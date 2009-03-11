#include "defaultconnection.h"

#include <qendian.h>
#include <QNetworkProxy>

#define CONNECT_TIMEOUT       30000
#define DISCONNECT_TIMEOUT    5000

DefaultConnection::DefaultConnection(IConnectionPlugin *APlugin, QObject *AParent) : QObject(AParent)
{
  FPlugin = APlugin;
  FConnected = false;
  FDisconnect = true; 
  FDisconnected = true;
  FSocket.setProtocol(QSsl::AnyProtocol);

  connect(&FSocket, SIGNAL(connected()), SLOT(onSocketConnected()));
  connect(&FSocket, SIGNAL(readyRead()), SLOT(onSocketReadyRead()));
  connect(&FSocket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(onSocketError(QAbstractSocket::SocketError)));
  connect(&FSocket, SIGNAL(disconnected()), SLOT(onSocketDisconnected()));
  connect(&FSocket, SIGNAL(encrypted()), SLOT(onSocketEncrypted()));
  connect(&FSocket, SIGNAL(sslErrors(const QList<QSslError> &)), SLOT(onSocketSSLErrors(const QList<QSslError> &)));
  connect(&FSocket, SIGNAL(modeChanged(QSslSocket::SslMode)), SIGNAL(modeChanged(QSslSocket::SslMode)));
  
  FConnectTimer.setSingleShot(true);
  connect(&FConnectTimer,SIGNAL(timeout()),SLOT(onConnectionTimeout()));
}

DefaultConnection::~DefaultConnection()
{
  disconnect();
}

bool DefaultConnection::isOpen() const
{
  return FSocket.state() == QAbstractSocket::ConnectedState;
}

bool DefaultConnection::isEncrypted() const
{
  return FSocket.isEncrypted();
}

void DefaultConnection::connectToHost()
{
  FDisconnect = false;
  FDisconnected = false;
  if (FSocket.state() == QAbstractSocket::UnconnectedState)
  {
    emit aboutToConnect();

    QString host  = option(IDefaultConnection::CO_HOST).toString();
    quint16 port = option(IDefaultConnection::CO_PORT).toInt();
    FUseSSL = option(IDefaultConnection::CO_USE_SSL).toBool();
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

    if (FUseSSL)
      FSocket.connectToHostEncrypted(host,port);
    else
      FSocket.connectToHost(host, port);

    FConnectTimer.start(CONNECT_TIMEOUT);
  }
  else
    connectionError(tr("Socket not ready"));
}

void DefaultConnection::disconnect()
{
  if (!FDisconnect)
  {
    FDisconnect = true;
    if (FConnected)
      FSocket.flush();
    FSocket.disconnectFromHost();
    if (FConnected && FSocket.state()!=QSslSocket::UnconnectedState)
      FSocket.waitForDisconnected(DISCONNECT_TIMEOUT);
  }
  if (!FDisconnected)
  {
    onSocketDisconnected();
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
  FSocket.ignoreSslErrors();
}

QList<QSslError> DefaultConnection::sslErrors() const
{
  return FSocket.sslErrors();
}

void DefaultConnection::connectionReady()
{
  FConnectTimer.stop();
  emit connected();
}

void DefaultConnection::connectionError(const QString &AError)
{
  emit error(AError);
  disconnect();
}

void DefaultConnection::onSocketConnected()
{
  FConnected = true;
  if (!FUseSSL)
    connectionReady();
}

void DefaultConnection::onSocketEncrypted()
{
  emit encrypted();
  if (FUseSSL)
    connectionReady();
}

void DefaultConnection::onSocketReadyRead()
{
  emit readyRead(FSocket.bytesAvailable()); 
}

void DefaultConnection::onSocketSSLErrors(const QList<QSslError> &AErrors)
{
  if (!FIgnoreSSLErrors)
    emit sslErrors(AErrors);
  else
    FSocket.ignoreSslErrors();
}

void DefaultConnection::onSocketError(QAbstractSocket::SocketError)
{
  FDisconnect = true;
  connectionError(FSocket.errorString());
}

void DefaultConnection::onSocketDisconnected()
{
  FConnected = false;
  FDisconnect = true;
  FDisconnected = true;
  FConnectTimer.stop();
  emit disconnected();
}

void DefaultConnection::onConnectionTimeout()
{
  connectionError(tr("Connection timed out"));
}

