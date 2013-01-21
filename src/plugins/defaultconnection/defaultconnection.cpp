#include "defaultconnection.h"

#include <QNetworkProxy>

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
#if QT_VERSION >= 0x040600
	FSocket.setSocketOption(QAbstractSocket::KeepAliveOption, 1);
#endif
	connect(&FSocket, SIGNAL(connected()), SLOT(onSocketConnected()));
	connect(&FSocket, SIGNAL(encrypted()), SLOT(onSocketEncrypted()));
	connect(&FSocket, SIGNAL(readyRead()), SLOT(onSocketReadyRead()));
	connect(&FSocket, SIGNAL(modeChanged(QSslSocket::SslMode)), SIGNAL(modeChanged(QSslSocket::SslMode)));
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

		QString host = option(IDefaultConnection::COR_HOST).toString();
		quint16 port = option(IDefaultConnection::COR_PORT).toInt();
		QString domain = option(IDefaultConnection::COR_DOMAINE).toString();
		FUseLegacySSL = option(IDefaultConnection::COR_USE_LEGACY_SSL).toBool();

		QJDns::Record record;
		record.name = !host.isEmpty() ? host.toLatin1() : domain.toLatin1();
		record.port = port;
		record.priority = 0;
		record.weight = 0;
		FRecords.append(record);

		if (host.isEmpty() && FDns.init(QJDns::Unicast, QHostAddress::Any))
		{
			FDns.setNameServers(QJDns::systemInfo().nameServers);
			FSrvQueryId = FDns.queryStart(QString("_xmpp-client._tcp.%1.").arg(domain).toLatin1(),QJDns::Srv);
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

QSslCertificate DefaultConnection::peerCertificate() const
{
	return FSocket.peerCertificate();
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
		QJDns::Record record = FRecords.takeFirst();

		while (record.name.endsWith('.'))
			record.name.chop(1);

		if (FUseLegacySSL)
			FSocket.connectToHostEncrypted(record.name, record.port);
		else
			FSocket.connectToHost(record.name, record.port);
	}
}

void DefaultConnection::onDnsResultsReady(int AId, const QJDns::Response &AResults)
{
	if (FSrvQueryId == AId)
	{
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
		FDns.shutdown();
}

void DefaultConnection::onDnsShutdownFinished()
{
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
	emit encrypted();
	if (FUseLegacySSL)
	{
		FRecords.clear();
		emit connected();
	}
}

void DefaultConnection::onSocketReadyRead()
{
	emit readyRead(FSocket.bytesAvailable());
}

void DefaultConnection::onSocketSSLErrors(const QList<QSslError> &AErrors)
{
	FSSLError = true;
	emit sslErrorsOccured(AErrors);
}

void DefaultConnection::onSocketError(QAbstractSocket::SocketError)
{
	if (FRecords.isEmpty())
	{
		if (FSocket.state()!=QSslSocket::ConnectedState || FSSLError)
		{
			emit error(XmppError(IERR_CONNECTIONS_CONNECT_ERROR,FSocket.errorString()));
			emit disconnected();
		}
		else
		{
			emit error(XmppError(IERR_CONNECTIONS_CONNECT_ERROR,FSocket.errorString()));
		}
	}
	else
	{
		connectToNextHost();
	}
}

void DefaultConnection::onSocketDisconnected()
{
	emit disconnected();
}
