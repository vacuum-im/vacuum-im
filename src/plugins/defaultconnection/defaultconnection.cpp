#include "defaultconnection.h"

#include <QNetworkProxy>

#define START_QUERY_ID        0
#define STOP_QUERY_ID         -1

#define DISCONNECT_TIMEOUT    5000

DefaultConnection::DefaultConnection(IConnectionPlugin *APlugin, QObject *AParent) : QObject(AParent)
{
  FPlugin = APlugin;

  FSrvQueryId = START_QUERY_ID;
  connect(&FDns, SIGNAL(resultsReady(int, const QJDns::Response &)),SLOT(onDnsResultsReady(int, const QJDns::Response &)));
  connect(&FDns, SIGNAL(error(int, QJDns::Error)),SLOT(onDnsError(int, QJDns::Error)));
  connect(&FDns, SIGNAL(shutdownFinished()),SLOT(onDnsShutdownFinished()));

  FSocket.setProtocol(QSsl::AnyProtocol);
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

    QString host = option(IDefaultConnection::CO_HOST).toString();
    quint16 port = option(IDefaultConnection::CO_PORT).toInt();
    QString domain = option(IDefaultConnection::CO_DOMAINE).toString();
    FSSLConnection = option(IDefaultConnection::CO_USE_SSL).toBool();
    FIgnoreSSLErrors = option(IDefaultConnection::CO_IGNORE_SSL_ERRORS).toBool();
    
    QNetworkProxy proxy;
    switch (option(IDefaultConnection::CO_PROXY_TYPE).toInt())
    {
    case PT_NO_PROXY: 
      proxy.setType(QNetworkProxy::NoProxy);
      break;
    case PT_SOCKET5_PROXY: 
      proxy.setType(QNetworkProxy::Socks5Proxy);
      break;
    case PT_HTTP_PROXY: 
      proxy.setType(QNetworkProxy::HttpProxy);
      break;
    default:
      proxy.setType(QNetworkProxy::DefaultProxy);
    }
    proxy.setHostName(option(IDefaultConnection::CO_PROXY_HOST).toString());
    proxy.setPort(option(IDefaultConnection::CO_PROXY_PORT).toInt());
    proxy.setUser(option(IDefaultConnection::CO_PROXY_USER_NAME).toString());
    proxy.setPassword(option(IDefaultConnection::CO_PROXY_PASSWORD).toString());
    FSocket.setProxy(proxy);

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
      connectToNextHost();

    return true;
  }
  return false;
}

void DefaultConnection::disconnectFromHost()
{
  FRecords.clear();

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
    emit error(tr("Disconnection timed out"));
    emit disconnected();
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

QVariant DefaultConnection::option(int ARole) const
{
  return FOptions.value(ARole);
}

void DefaultConnection::setOption(int ARole, const QVariant &AValue)
{
  FOptions.insert(ARole, AValue);
}

void DefaultConnection::startClientEncryption()
{
  FSocket.startClientEncryption();
}

QSsl::SslProtocol DefaultConnection::protocol() const
{
  return FSocket.protocol();
}

void DefaultConnection::setProtocol(QSsl::SslProtocol AProtocol)
{
  FSocket.setProtocol(AProtocol);
}

void DefaultConnection::addCaCertificate(const QSslCertificate &ACertificate)
{
  FSocket.addCaCertificate(ACertificate);
}

QList<QSslCertificate> DefaultConnection::caCertificates() const
{
  return FSocket.caCertificates();
}

QSslCertificate DefaultConnection::peerCertificate() const
{
  return FSocket.peerCertificate();
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

void DefaultConnection::connectToNextHost()
{
  if (!FRecords.isEmpty())
  {
    QJDns::Record record = FRecords.takeFirst();

    while (record.name.endsWith('.'))
      record.name.chop(1);

    if (FSSLConnection)
      FSocket.connectToHostEncrypted(record.name, record.port);
    else
      FSocket.connectToHost(record.name, record.port);
  }
}

void DefaultConnection::onDnsResultsReady(int AId, const QJDns::Response &AResults)
{
  if (FSrvQueryId == AId)
  {
    FSSLConnection = false;
    FRecords = AResults.answerRecords;
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
    connectToNextHost();
  else
    emit disconnected();
  FSrvQueryId = START_QUERY_ID;
}

void DefaultConnection::onSocketConnected()
{
  if (!FSSLConnection)
  {
    FRecords.clear();
    emit connected();
  }
}

void DefaultConnection::onSocketEncrypted()
{
  emit encrypted();
  if (FSSLConnection)
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
  if (!FIgnoreSSLErrors)
    emit sslErrors(AErrors);
  else
    ignoreSslErrors();
}

void DefaultConnection::onSocketError(QAbstractSocket::SocketError)
{
  if (FRecords.isEmpty())
  {
    if (FSocket.state()!=QSslSocket::ConnectedState || FSSLError)
    {
      emit error(FSocket.errorString());
      emit disconnected();
    }
    else
      emit error(FSocket.errorString());
  }
  else
    connectToNextHost();
}

void DefaultConnection::onSocketDisconnected()
{
  emit disconnected();
}
