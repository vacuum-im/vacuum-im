#ifndef STREAMCONNECTION_H
#define STREAMCONNECTION_H

#include <QObject>
#include <QTimer>
#include "../../interfaces/ixmppstreams.h"

class StreamConnection : 
  public QObject,
  public IStreamConnection
{
  Q_OBJECT

public:
  StreamConnection(QObject *parent);
  ~StreamConnection();

  //IStreamConnection;
  virtual QObject *instance() { return this; }
  virtual const QTcpSocket &socket() { return FSocket; }
  virtual void connectToHost(const QString &AHost, qint16 APort);
  virtual qint64 write(const QByteArray &AData);
  virtual QByteArray read(qint64 ABytes);
  virtual bool isValid() const;
  virtual bool isOpen() const;
  virtual void close();
  virtual QStringList proxyTypes() const;
  virtual void setProxyType(int AProxyType) { FProxyType = AProxyType; }
  virtual int proxyType() const { return FProxyType; }
  virtual void setProxyHost(const QString &AProxyHost) { FProxyHost = AProxyHost; }
  virtual const QString &proxyHost() const { return FProxyHost; }
  virtual void setProxyPort(qint16 AProxyPort) { FProxyPort = AProxyPort; }
  virtual qint16 proxyPort() const { return FProxyPort; }
  virtual void setProxyUsername(const QString &AProxyUser) { FProxyUser = AProxyUser; }
  virtual const QString &proxyUsername() const { return FProxyUser; }
  virtual void setProxyPassword(const QString &AProxyPassword) { FProxyPassword = AProxyPassword; }
  virtual const QString &proxyPassword() const { return FProxyPassword; }
  virtual void setPollServer(const QString &APollServer) { FPollServer = APollServer; }
  virtual const QString &pollServer() const { return FPollServer; }
  virtual qint64 bytesWriten() const { return FBytesWriten; }
  virtual qint64 bytesReaded() const { return FBytesReaded; }
signals:
  void connected();
  void readyRead(qint64 bytes);
  void disconnected();
  void error(const QString &errString);
protected slots:
  void onSocketConnected();
  void onSocketReadyRead();
  void onSocketDisconnected();
  void onSocketError(QAbstractSocket::SocketError err);
  void onReadTimeout();
  void onKeepAliveTimeout();
protected:
  void proxyConnection();
  void socket4Connection();
  void socket5Connection();
  void httpsConnection();
private:
  enum ProxyState {
    ProxyUnconnected,
    ProxyAuthType,
    ProxyAuthResult,
    ProxyConnect,
    ProxyConnectResult,
    ProxyReady
  }           FProxyState;
  QString     FHost;
  quint16     FPort;
  QTimer      ReadTimer;
  QTimer      KeepAliveTimer;
  QTcpSocket  FSocket;
  int         FProxyType;
  QString     FProxyHost;
  qint16      FProxyPort;
  QString     FProxyUser;
  QString     FProxyPassword;
  QString     FPollServer;
  qint64      FBytesWriten;
  qint64      FBytesReaded;
};

#endif // STREAMCONNECTION_H
