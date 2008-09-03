#include "defaultconnection.h"

#include <qendian.h>

#define CONNECT_TIMEOUT       30000
#define DISCONNECT_TIMEOUT    5000

DefaultConnection::DefaultConnection(IConnectionPlugin *APlugin, QObject *AParent) : QObject(AParent)
{
  FPlugin = APlugin;
  FConnected = false;
  FDisconnect = true; 
  FDisconnected = true;
  FProxyState = ProxyUnconnected;
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
    FHost = option(IDefaultConnection::CO_Host).toString();
    FPort = option(IDefaultConnection::CO_Port).toInt();
    FUseSSL = option(IDefaultConnection::CO_UseSSL).toBool();
    FIgnoreSSLErrors = option(IDefaultConnection::CO_IgnoreSSLErrors).toBool();
    FProxyType = option(IDefaultConnection::CO_ProxyType).toInt();
    FProxyHost = option(IDefaultConnection::CO_ProxyHost).toString();
    FProxyPort = option(IDefaultConnection::CO_ProxyPort).toInt();
    FProxyUser = option(IDefaultConnection::CO_ProxyUserName).toString();
    FProxyPassword = option(IDefaultConnection::CO_ProxyPassword).toString();

    if (FProxyType == 0)
    {
      FProxyState = ProxyReady;
      if (FUseSSL)
        FSocket.connectToHostEncrypted(FHost,FPort);
      else
        FSocket.connectToHost(FHost, FPort);
    }
    else 
    {
      FProxyState = ProxyUnconnected;
      FSocket.connectToHost(FProxyHost, FProxyPort);
    }
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

void DefaultConnection::proxyConnection()
{
  switch (FProxyType)
  {
  case 1: socket5Connection();break;
  case 2: httpsConnection();break;
  }
}

void DefaultConnection::socket5Connection()
{
  if (FProxyState == ProxyUnconnected)
  {
    QByteArray buf;
    buf += 5;
    buf += 1;
    if (FProxyUser.isEmpty())
      buf +='\0';
    else 
      buf += 2;
    FProxyState = ProxyAuthType;
    write(buf); 
  }
  else if (FProxyState == ProxyAuthType)
  {
    QByteArray buf = read(FSocket.bytesAvailable());
    if (buf[2] == '\2') //Authentication process
    {
      buf.clear();
      buf += 1;       //version of subnegotiation
      buf += (const char)FProxyUser.toUtf8().size();
      buf += FProxyUser.toUtf8();
      buf += (const char)FProxyPassword.toUtf8().size();
      buf += FProxyPassword.toUtf8(); 
      FProxyState = ProxyAuthResult;
      write(buf);
    }
    else
      FProxyState = ProxyConnect;
  }
  else if (FProxyState == ProxyAuthResult)
  {
    QByteArray buf = read(FSocket.bytesAvailable());
    if (buf[1] != '\0')
      connectionError(tr("Socket5 authentication error"));
    else
      FProxyState = ProxyConnect;
  }

  if (FProxyState == ProxyConnect)
  {
    QByteArray buf;
    buf += 5;
    buf += 1;
    buf += '\0';
    buf += 3;
    buf += (const char)FHost.toAscii().size();
    buf += FHost.toAscii();
    quint16 port = qToBigEndian<quint16>(FPort);
    buf += QByteArray((const char *)&port,2);
    FProxyState = ProxyConnectResult;
    write(buf);
  }
  else if (FProxyState == ProxyConnectResult)
  {
    QByteArray buf = read(FSocket.bytesAvailable());
    if (buf[1] != '\0')
    {
      QString err;
      switch (buf[1])
      {
        case 1: err = tr("General SOCKS server failure.");break;
        case 2: err = tr("Connection not allowed by rule set.");break;
        case 3: err = tr("Network unreachable.");break;
        case 4: err = tr("Host unreachable.");break;
        case 5: err = tr("Connection refused.");break;
        case 6: err = tr("TTL expired.");break;
        case 7: err = tr("Command not supported.");break;
        case 8: err = tr("Address type not supported.");break;
        default: err = tr("Unknown socks error.");break;
      }
      connectionError(err);
    }
    else
      proxyReady();
  }
}

void DefaultConnection::httpsConnection()
{
  if (FProxyState == ProxyUnconnected)
  {
    QByteArray buf = "CONNECT " + FHost.trimmed().toAscii()+":"+QString("%1").arg(FPort).toAscii()+" HTTP/1.1\r\n";
    if (!FProxyUser.isEmpty())
      buf += "Proxy-Authorization: Basic "+FProxyUser.toUtf8()+":"+FProxyPassword.toUtf8()+"\r\n";
    buf += "Connection: Keep-Alive\r\n";
    buf += "Proxy-Connection: Keep-Alive\r\n";
    buf += "Host: "+FHost.toAscii()+":"+QString("%1").arg(FPort).toAscii()+"\r\n";  
    buf += "\r\n";
    FProxyState = ProxyConnectResult;
    write(buf);
  }
  else if (FProxyState == ProxyConnectResult)
  {
    QList<QByteArray> result = read(FSocket.bytesAvailable()).split(' ');
    if (result[1].toInt() != 200)
      connectionError(result[2]);
    else
      proxyReady();
  }
}

void DefaultConnection::proxyReady()
{
  FProxyState = ProxyReady;
  if (FUseSSL)
    FSocket.startClientEncryption();
  else
    connectionReady();
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
  if (FProxyState != ProxyReady)
    proxyConnection();
  else if (!FUseSSL)
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
  if (FProxyState != ProxyReady)
    proxyConnection();
  else
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
  FSocket.disconnectFromHost();
  emit disconnected();
}

void DefaultConnection::onConnectionTimeout()
{
  connectionError(tr("Connection timed out"));
}

