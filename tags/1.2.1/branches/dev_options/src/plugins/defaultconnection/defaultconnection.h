#ifndef DEFAULTCONNECTION_H
#define DEFAULTCONNECTION_H

#include <interfaces/iconnectionmanager.h>
#include <interfaces/idefaultconnection.h>
#include <thirdparty/jdns/qjdns.h>

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
  //IDefaultConnection
  virtual void startClientEncryption();
  virtual QSsl::SslProtocol protocol() const;
  virtual void setProtocol(QSsl::SslProtocol AProtocol);
  virtual void addCaCertificate(const QSslCertificate &ACertificate);
  virtual QList<QSslCertificate> caCertificates() const;
  virtual QSslCertificate peerCertificate() const;
  virtual void ignoreSslErrors();
  virtual QList<QSslError> sslErrors() const;
  virtual QNetworkProxy proxy() const;
  virtual void setProxy(const QNetworkProxy &AProxy);
  virtual QVariant option(int ARole) const;
  virtual void setOption(int ARole, const QVariant &AValue);
signals:
  void aboutToConnect();
  void connected();
  void encrypted();
  void readyRead(qint64 ABytes);
  void error(const QString &AMessage);
  void aboutToDisconnect();
  void disconnected();
  void connectionDestroyed();
  //IDefaultConnection
  void modeChanged(QSslSocket::SslMode AMode);
  void sslErrors(const QList<QSslError> &AErrors);
  void proxyChanged(const QNetworkProxy &AProxy);
protected:
  void connectToNextHost();
protected slots:
  void onDnsResultsReady(int AId, const QJDns::Response &AResults);
  void onDnsError(int AId, QJDns::Error AError);
  void onDnsShutdownFinished();
  void onSocketConnected();
  void onSocketEncrypted();
  void onSocketReadyRead();
  void onSocketSSLErrors(const QList<QSslError> &AErrors);
  void onSocketError(QAbstractSocket::SocketError AError);
  void onSocketDisconnected();
private:
  IConnectionPlugin *FPlugin;  
private:
  int FSrvQueryId;
  QJDns FDns;
  QList<QJDns::Record> FRecords;
private:
  bool FSSLError;
  bool FSSLConnection;
  bool FIgnoreSSLErrors;
  QSslSocket FSocket;
private:
  QMap<int, QVariant> FOptions;
};

#endif // DEFAULTCONNECTION_H
