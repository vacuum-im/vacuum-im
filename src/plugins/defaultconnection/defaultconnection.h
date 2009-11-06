#ifndef DEFAULTCONNECTION_H
#define DEFAULTCONNECTION_H

#include <interfaces/iconnectionmanager.h>
#include <interfaces/idefaultconnection.h>

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
  virtual bool connectToHost();
  virtual void disconnectFromHost();
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
  virtual void aboutToConnect();
  virtual void connected();
  virtual void encrypted();
  virtual void readyRead(qint64 ABytes);
  virtual void sslErrors(const QList<QSslError> &AErrors);
  virtual void error(const QString &AMessage);
  virtual void aboutToDisconnect();
  virtual void disconnected();
  virtual void modeChanged(QSslSocket::SslMode AMode);
protected slots:
  void onSocketConnected();
  void onSocketEncrypted();
  void onSocketReadyRead();
  void onSocketSSLErrors(const QList<QSslError> &AErrors);
  void onSocketError(QAbstractSocket::SocketError AError);
  void onSocketDisconnected();
private:
  IConnectionPlugin *FPlugin;  
private:
  bool FSSLError;
  bool FSSLConnection;
  bool FIgnoreSSLErrors;
  QSslSocket FSocket;
  QHash<int, QVariant> FOptions;
};

#endif // DEFAULTCONNECTION_H
