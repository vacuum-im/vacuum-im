#ifndef DEFAULTCONNECTION_H
#define DEFAULTCONNECTION_H

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
  //IConnection
  virtual QObject *instance() { return this; }
  virtual IConnectionPlugin *ownerPlugin() const { return FPlugin; }
  virtual bool isOpen() const;
  virtual bool isEncrypted() const;
  virtual void connectToHost();
  virtual void disconnect();
  virtual qint64 write(const QByteArray &AData);
  virtual QByteArray read(qint64 ABytes);
  virtual QVariant option(int ARole) const;
  virtual void setOption(int ARole, const QVariant &AValue);
  //DefaultConnection
  virtual void startClientEncryption();
  virtual QSsl::SslProtocol protocol() const;
  virtual void setProtocol(QSsl::SslProtocol AProtocol);
  virtual void addCaCertificate(const QSslCertificate &ACertificate);
  virtual QList<QSslCertificate> caCertificates() const;
  virtual QSslCertificate peerCertificate() const;
  virtual void ignoreSslErrors();
  virtual QList<QSslError> sslErrors() const;
signals:
  virtual void connected();
  virtual void encrypted();
  virtual void readyRead(qint64 ABytes);
  virtual void disconnected();
  virtual void error(const QString &AMessage);
  virtual void modeChanged(QSslSocket::SslMode AMode);
  virtual void sslErrors(const QList<QSslError> &AErrors);
protected:
  void proxyConnection();
  void socket5Connection();
  void httpsConnection();
  void proxyReady();
  void connectionReady();
  void connectionError(const QString &AError);
protected slots:
  void onSocketConnected();
  void onSocketEncrypted();
  void onSocketReadyRead();
  void onSocketSSLErrors(const QList<QSslError> &AErrors);
  void onSocketError(QAbstractSocket::SocketError AError);
  void onSocketDisconnected();
  void onConnectionTimeout();
private:
  IConnectionPlugin *FPlugin;  
private:
  QSslSocket FSocket;
  QTimer FConnectTimer;
private:
  enum ProxyState {
    ProxyUnconnected,
    ProxyAuthType,
    ProxyAuthResult,
    ProxyConnect,
    ProxyConnectResult,
    ProxyReady
  };
  bool FDisconnect;
  bool FDisconnected;
  ProxyState FProxyState;
  QHash<int, QVariant> FOptions;
  QString FHost;
  quint16 FPort;
  bool FUseSSL;
  bool FIgnoreSSLErrors;
  int FProxyType;
  QString FProxyHost;
  qint16  FProxyPort;
  QString FProxyUser;
  QString FProxyPassword;
};

#endif // DEFAULTCONNECTION_H
