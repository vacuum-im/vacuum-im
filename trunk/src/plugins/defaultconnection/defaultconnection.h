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
  void aboutToConnect();
  void connected();
  void encrypted();
  void readyRead(qint64 ABytes);
  void sslErrors(const QList<QSslError> &AErrors);
  void error(const QString &AMessage);
  void aboutToDisconnect();
  void disconnected();
  void modeChanged(QSslSocket::SslMode AMode);
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
  QHash<int, QVariant> FOptions;
};

#endif // DEFAULTCONNECTION_H
