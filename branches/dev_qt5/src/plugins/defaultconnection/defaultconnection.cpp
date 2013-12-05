#include "defaultconnection.h"

#include <QNetworkProxy>
#include <QAuthenticator>
#include <definitions/internalerrors.h>

#define START_QUERY_ID        0
#define STOP_QUERY_ID         -1

#define DISCONNECT_TIMEOUT    5000

DefaultConnection::DefaultConnection(IConnectionPlugin *APlugin, QObject *AParent) : QObject(AParent)
{
	FPlugin = APlugin;
	FDisconnecting = false;
	
	FDnsLookup.setType(QDnsLookup::SRV);
	connect(&FDnsLookup,SIGNAL(finished()),SLOT(onDnsLookupFinished()));

	FSocket.setProtocol(QSsl::AnyProtocol);
	FSocket.setSocketOption(QAbstractSocket::KeepAliveOption,1);
	connect(&FSocket, SIGNAL(proxyAuthenticationRequired(const QNetworkProxy &, QAuthenticator *)),
		SLOT(onSocketProxyAuthenticationRequired(const QNetworkProxy &, QAuthenticator *)));
	connect(&FSocket, SIGNAL(connected()), SLOT(onSocketConnected()));
	connect(&FSocket, SIGNAL(encrypted()), SLOT(onSocketEncrypted()));
	connect(&FSocket, SIGNAL(readyRead()), SLOT(onSocketReadyRead()));
	connect(&FSocket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(onSocketError(QAbstractSocket::SocketError)));
	connect(&FSocket, SIGNAL(sslErrors(const QList<QSslError> &)), SLOT(onSocketSSLErrors(const QList<QSslError> &)));
	connect(&FSocket, SIGNAL(disconnected()), SLOT(onSocketDisconnected()));

	// Make FDnsLookup.isFinished to be true
	FDnsLookup.lookup();
	FDnsLookup.abort();
}

DefaultConnection::~DefaultConnection()
{
	disconnectFromHost();
	emit connectionDestroyed();
}

bool DefaultConnection::isOpen() const
{
	return FSocket.state() == QAbstractSocket::ConnectedState;
}

bool DefaultConnection::isEncrypted() const
{
	return FSocket.isEncrypted();
}

bool DefaultConnection::connectToHost()
{
	if (FDnsLookup.isFinished() && FSocket.state()==QAbstractSocket::UnconnectedState)
	{
		emit aboutToConnect();

		FRecords.clear();
		FSSLError = false;

		QString host = option(IDefaultConnection::Host).toString();
		quint16 port = option(IDefaultConnection::Port).toInt();
		QString domain = option(IDefaultConnection::Domain).toString();
		FUseLegacySSL = option(IDefaultConnection::UseLegacySsl).toBool();
		FVerifyMode = (CertificateVerifyMode)option(IDefaultConnection::CertVerifyMode).toInt();

		SrvRecord record;
		record.target = !host.isEmpty() ? host : domain;
		record.port = port;
		FRecords.append(record);

		if (host.isEmpty())
		{
			FDnsLookup.setName(QString("_xmpp-client._tcp.%1.").arg(domain));
			FDnsLookup.lookup();
		}
		else
		{
			connectToNextHost();
		}
		return true;
	}
	return false;
}

void DefaultConnection::disconnectFromHost()
{
	if (!FDisconnecting)
	{
		FRecords.clear();
		FDisconnecting = true;

		if (FSocket.state() != QSslSocket::UnconnectedState)
		{
			if (FSocket.state() == QSslSocket::ConnectedState)
			{
				emit aboutToDisconnect();
				FSocket.flush();
				FSocket.disconnectFromHost();
			}
			else if (FSocket.state() != QSslSocket::ClosingState)
			{
				FSocket.abort();
			}
			if (FSocket.state()!=QSslSocket::UnconnectedState && !FSocket.waitForDisconnected(DISCONNECT_TIMEOUT))
			{
				FSocket.abort();
			}
		}
		else if (!FDnsLookup.isFinished())
		{
			FDnsLookup.abort();
		}

		emit disconnected();
		FDisconnecting = false;
	}
}

void DefaultConnection::abortConnection(const XmppError &AError)
{
	if (!FDisconnecting && FSocket.state()!=QSslSocket::UnconnectedState)
	{
		emit error(AError);
		disconnectFromHost();
	}
}

qint64 DefaultConnection::write(const QByteArray &AData)
{
	return FSocket.write(AData);
}

QByteArray DefaultConnection::read(qint64 ABytes)
{
	return FSocket.read(ABytes);
}

IConnectionPlugin *DefaultConnection::ownerPlugin() const
{
	return FPlugin;
}

QSslCertificate DefaultConnection::hostCertificate() const
{
	return FSocket.peerCertificate();
}

void DefaultConnection::startClientEncryption()
{
	FSocket.startClientEncryption();
}

void DefaultConnection::ignoreSslErrors()
{
	FSSLError = false;
	FSocket.ignoreSslErrors();
}

QList<QSslError> DefaultConnection::sslErrors() const
{
	return FSocket.sslErrors();
}

QSsl::SslProtocol DefaultConnection::protocol() const
{
	return FSocket.protocol();
}

void DefaultConnection::setProtocol(QSsl::SslProtocol AProtocol)
{
	FSocket.setProtocol(AProtocol);
}

QSslKey DefaultConnection::privateKey() const
{
	return FSocket.privateKey();
}

void DefaultConnection::setPrivateKey(const QSslKey &AKey)
{
	FSocket.setPrivateKey(AKey);
}

QSslCertificate DefaultConnection::localCertificate() const
{
	return FSocket.localCertificate();
}

void DefaultConnection::setLocalCertificate(const QSslCertificate &ACertificate)
{
	FSocket.setLocalCertificate(ACertificate);
}

QList<QSslCertificate> DefaultConnection::caCertificates() const
{
	return FSocket.caCertificates();
}

void DefaultConnection::addCaSertificates(const QList<QSslCertificate> &ACertificates)
{
	QList<QSslCertificate> curSerts = caCertificates();
	foreach(const QSslCertificate &cert, ACertificates)
	{
		if (!cert.isNull() && !curSerts.contains(cert))
			FSocket.addCaCertificate(cert);
	}
}

void DefaultConnection::setCaCertificates(const QList<QSslCertificate> &ACertificates)
{
	FSocket.setCaCertificates(ACertificates);
}

QNetworkProxy DefaultConnection::proxy() const
{
	return FSocket.proxy();
}

void DefaultConnection::setProxy(const QNetworkProxy &AProxy)
{
	if (AProxy!= FSocket.proxy())
	{
		FSocket.setProxy(AProxy);
		emit proxyChanged(AProxy);
	}
}

QVariant DefaultConnection::option(int ARole) const
{
	return FOptions.value(ARole);
}

void DefaultConnection::setOption(int ARole, const QVariant &AValue)
{
	FOptions.insert(ARole, AValue);
}

void DefaultConnection::connectToNextHost()
{
	if (!FRecords.isEmpty())
	{
		SrvRecord record = FRecords.takeFirst();

		if (FUseLegacySSL)
			FSocket.connectToHostEncrypted(record.target, record.port);
		else
			FSocket.connectToHost(record.target, record.port);
	}
}

void DefaultConnection::onDnsLookupFinished()
{
	if (!FRecords.isEmpty())
	{
		QList<QDnsServiceRecord> dnsRecords = FDnsLookup.serviceRecords();
		if (!dnsRecords.isEmpty())
		{
			FRecords.clear();
			foreach (const QDnsServiceRecord &dnsRecord, dnsRecords)
			{
				SrvRecord srvRecord;
				srvRecord.target = dnsRecord.target();
				srvRecord.port = dnsRecord.port();
				FRecords.append(srvRecord);
			}
		}
		connectToNextHost();
	}
}

void DefaultConnection::onSocketProxyAuthenticationRequired(const QNetworkProxy &AProxy, QAuthenticator *AAuth)
{
	AAuth->setUser(AProxy.user());
	AAuth->setPassword(AProxy.password());
}

void DefaultConnection::onSocketConnected()
{
	if (!FUseLegacySSL)
	{
		FRecords.clear();
		emit connected();
	}
}

void DefaultConnection::onSocketEncrypted()
{
	if (FVerifyMode==IDefaultConnection::TrustedOnly && !caCertificates().contains(hostCertificate()))
	{
		abortConnection(XmppError(IERR_DEFAULTCONNECTION_CERT_NOT_TRUSTED));
	}
	else
	{
		emit encrypted();
		if (FUseLegacySSL)
		{
			FRecords.clear();
			emit connected();
		}
	}
}

void DefaultConnection::onSocketReadyRead()
{
	emit readyRead(FSocket.bytesAvailable());
}

void DefaultConnection::onSocketSSLErrors(const QList<QSslError> &AErrors)
{
	if (FVerifyMode == IDefaultConnection::Disabled)
	{
		ignoreSslErrors();
	}
	else if (FVerifyMode == IDefaultConnection::TrustedOnly)
	{
		ignoreSslErrors();
	}
	else
	{
		FSSLError = true;
		emit sslErrorsOccured(AErrors);
	}
}

void DefaultConnection::onSocketError(QAbstractSocket::SocketError)
{
	if (FRecords.isEmpty())
	{
		if (FSocket.state()!=QSslSocket::ConnectedState || FSSLError)
		{
			emit error(XmppError(IERR_CONNECTIONMANAGER_CONNECT_ERROR,FSocket.errorString()));
			emit disconnected();
		}
		else
		{
			emit error(XmppError(IERR_CONNECTIONMANAGER_CONNECT_ERROR,FSocket.errorString()));
		}
	}
	else
	{
		connectToNextHost();
	}
}

void DefaultConnection::onSocketDisconnected()
{
	FRecords.clear();
	emit disconnected();
}
