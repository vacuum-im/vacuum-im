#include "defaultconnection.h"

#include <QNetworkProxy>

#define DISCONNECT_TIMEOUT    5000

DefaultConnection::DefaultConnection(IConnectionPlugin *APlugin, QObject *AParent) : QObject(AParent)
{
  FPlugin = APlugin;
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
  if (FSocket.state() == QAbstractSocket::UnconnectedState)
  {
    emit aboutToConnect();

    FSSLError = false;
    QString host  = option(IDefaultConnection::CO_HOST).toString();
    quint16 port = option(IDefaultConnection::CO_PORT).toInt();
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

    if (FSSLConnection)
      FSocket.connectToHostEncrypted(host,port);
    else
      FSocket.connectToHost(host, port);
    
    return true;
  }
  return false;
}

void DefaultConnection::disconnectFromHost()
{
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

void DefaultConnection::onSocketConnected()
{
  if (!FSSLConnection)
    emit connected();
}

void DefaultConnection::onSocketEncrypted()
{
  emit encrypted();
  if (FSSLConnection)
    emit connected();
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
  if (FSocket.state()!=QSslSocket::ConnectedState || FSSLError)
  {
    emit error(FSocket.errorString());
    emit disconnected();
  }
  else
    emit error(FSocket.errorString());
}

void DefaultConnection::onSocketDisconnected()
{
  emit disconnected();
}
