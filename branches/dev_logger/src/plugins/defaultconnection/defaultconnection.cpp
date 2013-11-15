#include "defaultconnection.h"

#include <QNetworkProxy>
#include <QAuthenticator>
#include <definitions/internalerrors.h>
#include <utils/logger.h>

#define START_QUERY_ID        0
#define STOP_QUERY_ID         -1

#define DISCONNECT_TIMEOUT    5000

DefaultConnection::DefaultConnection(IConnectionPlugin *APlugin, QObject *AParent) : QObject(AParent)
{
	FPlugin = APlugin;
	FDisconnecting = false;
	
	FSrvQueryId = START_QUERY_ID;
	connect(&FDns, SIGNAL(resultsReady(int, const QJDns::Response &)),SLOT(onDnsResultsReady(int, const QJDns::Response &)));
	connect(&FDns, SIGNAL(error(int, QJDns::Error)),SLOT(onDnsError(int, QJDns::Error)));
	connect(&FDns, SIGNAL(shutdownFinished()),SLOT(onDnsShutdownFinished()));

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
	if (FSrvQueryId==START_QUERY_ID && FSocket.state()==QAbstractSocket::UnconnectedState)
	{
		emit aboutToConnect();

		FRecords.clear();
		FSSLError = false;

		QString host = option(IDefaultConnection::Host).toString();
		quint16 port = option(IDefaultConnection::Port).toInt();
		QString domain = option(IDefaultConnection::Domain).toString();
		FUseLegacySSL = option(IDefaultConnection::UseLegacySsl).toBool();
		FVerifyMode = (CertificateVerifyMode)option(IDefaultConnection::CertVerifyMode).toInt();

		QJDns::Record record;
		record.name = !host.isEmpty() ? host.toLatin1() : domain.toLatin1();
		record.port = port;
		record.priority = 0;
		record.weight = 0;
		FRecords.append(record);

		if (!host.isEmpty())
		{
			connectToNextHost();
		}
		else if (FDns.init(QJDns::Unicast, QHostAddress::Any))
		{
			LOG_INFO(QString("Starting DNS SRV lookup, domain=%1").arg(domain));
			FDns.setNameServers(QJDns::systemInfo().nameServers);
			FSrvQueryId = FDns.queryStart(QString("_xmpp-client._tcp.%1.").arg(domain).toLatin1(),QJDns::Srv);
		}
		else
		{
			LOG_ERROR("Failed to init DNS SRV lookup");
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

		LOG_INFO(QString("Disconnecting from host=%1").arg(FSocket.peerName()));

		if (FSocket.state() != QSslSocket::UnconnectedState)
		{
			if (FSocket.state() == QSslSocket::ConnectedState)
			{
				emit aboutToDisconnect();
				FSocket.flush();
				FSocket.disconnectFromHost();
			}
			else
			{
				FSocket.abort();
				emit disconnected();
			}
		}
		else if (FSrvQueryId != START_QUERY_ID)
		{
			FSrvQueryId = STOP_QUERY_ID;
			FDns.shutdown();
		}

		if (FSocket.state()!=QSslSocket::UnconnectedState && !FSocket.waitForDisconnected(DISCONNECT_TIMEOUT))
		{
			FSocket.abort();
			emit disconnected();
		}

		FDisconnecting = false;
	}
}

void DefaultConnection::abortConnection(const XmppError &AError)
{
	if (!FDisconnecting && FSocket.state()!=QSslSocket::UnconnectedState)
	{
		LOG_INFO(QString("Aborting connection to host=%1: %2").arg(FSocket.peerName(),AError.errorMessage()));
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
		LOG_INFO(QString("Connection proxy changed, host=%1, port=%2").arg(AProxy.hostName()).arg(AProxy.port()));
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
		QJDns::Record record = FRecords.takeFirst();

		while (record.name.endsWith('.'))
			record.name.chop(1);

		if (FUseLegacySSL)
		{
			LOG_INFO(QString("Connecting to host with encryption, host=%1, port=%2").arg(QString::fromLatin1(record.name)).arg(record.port));
			FSocket.connectToHostEncrypted(record.name, record.port);
		}
		else
		{
			LOG_INFO(QString("Connecting to host=%1, port=%2").arg(QString::fromLatin1(record.name)).arg(record.port));
			FSocket.connectToHost(record.name, record.port);
		}
	}
}

void DefaultConnection::onDnsResultsReady(int AId, const QJDns::Response &AResults)
{
	if (FSrvQueryId == AId)
	{
		LOG_INFO(QString("SRV records received, count=%1").arg(AResults.answerRecords.count()));
		if (!AResults.answerRecords.isEmpty())
		{
			FUseLegacySSL = false;
			FRecords = AResults.answerRecords;
		}
		FDns.shutdown();
	}
}

void DefaultConnection::onDnsError(int AId, QJDns::Error AError)
{
	Q_UNUSED(AError);
	if (FSrvQueryId == AId)
	{
		LOG_WARNING(QString("Failed to lookup DNS SRV records: %1").arg(AError));
		FDns.shutdown();
	}
}

void DefaultConnection::onDnsShutdownFinished()
{
	LOG_INFO("DNS SRV lookup finished");
	if (FSrvQueryId != STOP_QUERY_ID)
	{
		FSrvQueryId = START_QUERY_ID;
		connectToNextHost();
	}
	else
	{
		FSrvQueryId = START_QUERY_ID;
		emit disconnected();
	}
}

void DefaultConnection::onSocketProxyAuthenticationRequired(const QNetworkProxy &AProxy, QAuthenticator *AAuth)
{
	LOG_INFO(QString("Proxy authentication requested, host=%1, proxy=%2, user=%3").arg(FSocket.peerName(),AProxy.hostName(),AProxy.user()));
	AAuth->setUser(AProxy.user());
	AAuth->setPassword(AProxy.password());
}

void DefaultConnection::onSocketConnected()
{
	LOG_INFO(QString("Socket connected, host=%1").arg(FSocket.peerName()));
	if (!FUseLegacySSL)
	{
		FRecords.clear();
		emit connected();
	}
}

void DefaultConnection::onSocketEncrypted()
{
	LOG_INFO(QString("Socket encrypted, host=%1").arg(FSocket.peerName()));
	if (FVerifyMode!=IDefaultConnection::TrustedOnly || caCertificates().contains(hostCertificate()))
	{
		emit encrypted();
		if (FUseLegacySSL)
		{
			FRecords.clear();
			emit connected();
		}
	}
	else
	{
		abortConnection(XmppError(IERR_DEFAULTCONNECTION_CERT_NOT_TRUSTED));
	}
}

void DefaultConnection::onSocketReadyRead()
{
	emit readyRead(FSocket.bytesAvailable());
}

void DefaultConnection::onSocketSSLErrors(const QList<QSslError> &AErrors)
{
	LOG_INFO(QString("Socket SSL errors occurred, host=%1, verify=%2").arg(FSocket.peerName()).arg(FVerifyMode));
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

void DefaultConnection::onSocketError(QAbstractSocket::SocketError AError)
{
	Q_UNUSED(AError);
	LOG_INFO(QString("Socket error, host=%1: %2").arg(FSocket.peerName(),FSocket.errorString()));
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
	LOG_INFO(QString("Socket disconnected, host=%1").arg(FSocket.peerName()));
	emit disconnected();
}
