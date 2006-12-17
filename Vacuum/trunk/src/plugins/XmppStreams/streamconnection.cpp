#include <QtDebug>
#include "streamconnection.h"
#include <qendian.h>

#define READ_TIMEOUT 15000
#define KEEP_ALIVE_TIMEOUT 30000

StreamConnection::StreamConnection(QObject *parent)
  : QObject(parent), 
    FSocket(this)
{
  FProxyType = 0;
  FProxyPort = 0;
  FBytesWriten = 0;
  FBytesReaded = 0;
  FProxyState = ProxyUnconnected;
  connect(&FSocket, SIGNAL(connected()), SLOT(onSocketConnected()));
  connect(&FSocket, SIGNAL(readyRead()), SLOT(onSocketReadyRead()));
  connect(&FSocket, SIGNAL(error(QAbstractSocket::SocketError)),
    SLOT(onSocketError(QAbstractSocket::SocketError)));
  connect(&FSocket, SIGNAL(disconnected()), SLOT(onSocketDisconnected()));
  connect(&ReadTimer,SIGNAL(timeout()),SLOT(onReadTimeout()));
  connect(&KeepAliveTimer,SIGNAL(timeout()),SLOT(onKeepAliveTimeout()));
}

StreamConnection::~StreamConnection()
{
  qDebug() << "~StreamConnection";
  close();
}

void StreamConnection::connectToHost(const QString &AHost, qint16 APort)
{
  FHost = AHost;
  FPort = APort;
  if (FProxyType == 0)
  {
    FProxyState = ProxyReady;
    FSocket.connectToHost(AHost, APort);
  }
  else 
  {
    FProxyState = ProxyUnconnected;
    FSocket.connectToHost(FProxyHost, FProxyPort);
  }
}

qint64 StreamConnection::write(const QByteArray &AData) 
{
  KeepAliveTimer.start(KEEP_ALIVE_TIMEOUT); 
  //qDebug() << "Out:"<<AData.size()<<"bytes";
  //qDebug() << AData.trimmed();
  qint64 bytes = FSocket.write(AData);
  if (bytes > 0)
    FBytesWriten += bytes;
  return bytes;
}

QByteArray StreamConnection::read(qint64 ABytes) 
{
  KeepAliveTimer.start(KEEP_ALIVE_TIMEOUT); 
  QByteArray data(ABytes,' ');
  qint64 readBytes = FSocket.read(data.data(),ABytes);
  FBytesReaded += readBytes;
  //qDebug() << "In:" << readBytes <<"bytes";
  //qDebug() << data.trimmed();
  return data;
}

bool StreamConnection::isValid() const
{
  return FSocket.state() == QAbstractSocket::ConnectedState;
}

bool StreamConnection::isOpen() const
{
  return FSocket.isOpen();
}

void StreamConnection::close()
{
  if (isOpen())
  {
    FSocket.disconnectFromHost();
    FSocket.flush(); 
  }
}

QStringList StreamConnection::proxyTypes() const
{
  return QStringList()	<< "Direct connection"     //0
                        << "SOCKS4 Proxy"          //1
                        << "SOCKS4A proxy"         //2
                        << "SOCKS5 Proxy"          //3
                        << "HTTPS Proxy"           //4
                        << "HTTP Polling";         //5
}

void StreamConnection::onSocketConnected()
{
  ReadTimer.start(READ_TIMEOUT); 
  KeepAliveTimer.start(KEEP_ALIVE_TIMEOUT); 
  if (FProxyState != ProxyReady)
    proxyConnection();
  else
    emit connected();
}

void StreamConnection::onSocketReadyRead()
{
  ReadTimer.stop();
  if (FProxyState != ProxyReady)
    proxyConnection();
  else
    emit readyRead(FSocket.bytesAvailable()); 
}

void StreamConnection::onSocketDisconnected()
{
  ReadTimer.stop(); 
  KeepAliveTimer.stop(); 
  emit disconnected();
}

void StreamConnection::onSocketError(QAbstractSocket::SocketError err)
{
  Q_UNUSED(err);
  emit error(FSocket.errorString());
}

void StreamConnection::onReadTimeout()
{
  if (isOpen())
  {
    ReadTimer.stop(); 
    emit error(tr("Socket timeout"));
    FSocket.disconnectFromHost();
  }
}

void StreamConnection::onKeepAliveTimeout()
{
  if (isValid())
    FSocket.write(" "); 
}

void StreamConnection::proxyConnection()
{
  switch (FProxyType)
  {
    case 1: socket4Connection();break;
    case 2: socket4Connection();break;
    case 3: socket5Connection();break;
    case 4: httpsConnection();break;
  }
}

void StreamConnection::socket4Connection()
{
  FProxyState = ProxyReady;
  emit connected();
}

void StreamConnection::socket5Connection()
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
    ReadTimer.start(READ_TIMEOUT); 
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
      ReadTimer.start(READ_TIMEOUT); 
      write(buf);
    }
    else
      FProxyState = ProxyConnect;
  }
  else if (FProxyState == ProxyAuthResult)
  {
    QByteArray buf = read(FSocket.bytesAvailable());
    if (buf[1]  != '\0')
    {
      emit error(tr("Socket5 authentication error"));
      close();
    }
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
    ReadTimer.start(READ_TIMEOUT); 
    write(buf);
  }
  else if (FProxyState == ProxyConnectResult)
  {
    QByteArray buf = read(FSocket.bytesAvailable());
    if (buf[1] == '\0')
    {
      FProxyState = ProxyReady;
      emit connected();
    }
    else
    {
      QString err;
      switch (buf[1])
      {
        case 1: err = tr("General SOCKS server failure.");break;
        case 2: err = tr("Connection not allowed by ruleset.");break;
        case 3: err = tr("Network unreachable.");break;
        case 4: err = tr("Host unreachable.");break;
        case 5: err = tr("Connection refused.");break;
        case 6: err = tr("TTL expired.");break;
        case 7: err = tr("Command not supported.");break;
        case 8: err = tr("Address type not supported.");break;
        default: err = tr("Unknown socks error.");break;
      }
      emit error(err);
      close();
    }
  }
}

void StreamConnection::httpsConnection()
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
    ReadTimer.start(READ_TIMEOUT); 
    write(buf);
  }
  else if (FProxyState == ProxyConnectResult)
  {
    QList<QByteArray> result = read(FSocket.bytesAvailable()).split(' ');
    if (result[1].toInt() == 200)
    {
      FProxyState = ProxyReady;
      emit connected();
    }
    else
    {
      emit error(result[2]);
      close();
    }
  }
}
