#ifndef DEFAULTCONNECTION_H
#define DEFAULTCONNECTION_H

#include <QSslSocket>
#include <QTimer>
#include "../../interfaces/iconnectionmanager.h"
#include "../../interfaces/idefaultconnection.h"

class DefaultConnection : 
  public QObject,
  public IConnection,
  public IDefaultConnection
{
  Q_OBJECT;
  Q_INTERFACES(IConnection IDefaultConnection);

public:
  DefaultConnection(IConnectionPlugin *APlugin, QObject *AParent);
  ~DefaultConnection();

  virtual QObject *instance() { return this; }
  virtual IConnectionPlugin *ownerPlugin() const { return FPlugin; }

  virtual bool isOpen() const;
  virtual void connectToHost();
  virtual void disconnect();
  virtual qint64 write(const QByteArray &AData);
  virtual QByteArray read(qint64 ABytes);
  virtual QVariant option(int ARole) const;
  virtual void setOption(int ARole, const QVariant &AValue);
signals:
  virtual void connected();
  virtual void readyRead(qint64 ABytes);
  virtual void disconnected();
  virtual void error(const QString &AMessage);
protected:
  void proxyConnection();
  void socket4Connection();
  void socket5Connection();
  void httpsConnection();
protected slots:
  void onSocketConnected();
  void onSocketReadyRead();
  void onSocketDisconnected();
  void onSocketError(QAbstractSocket::SocketError err);
  void onReadTimeout();
  void onKeepAliveTimeout();
private:
  IConnectionPlugin *FPlugin;  
private:
  QSslSocket FSocket;
  QTimer ReadTimer;
  QTimer KeepAliveTimer;
private:
  enum ProxyState {
    ProxyUnconnected,
    ProxyAuthType,
    ProxyAuthResult,
    ProxyConnect,
    ProxyConnectResult,
    ProxyReady
  };
  ProxyState FProxyState;
  QHash<int, QVariant> FOptions;
  QString FHost;
  quint16 FPort;
  int FProxyType;
  QString FProxyHost;
  qint16  FProxyPort;
  QString FProxyUser;
  QString FProxyPassword;
};

#endif // DEFAULTCONNECTION_H
