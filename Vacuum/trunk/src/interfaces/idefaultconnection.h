#ifndef IDEFAULTCONNECTION_H
#define IDEFAULTCONNECTION_H

#define DEFAULTCONNECTION_UUID "{68F9B5F2-5898-43f8-9DD1-19F37E9779AC}"

#include <QSslSocket>

class IDefaultConnection
{
public:
  enum Options {
    CO_Host,
    CO_Port,
    CO_UseSSL,
    CO_IgnoreSSLErrors,
    CO_ProxyType,
    CO_ProxyHost,
    CO_ProxyPort,
    CO_ProxyUserName,
    CO_ProxyPassword,
    CO_UserOptions = 100
  };
  virtual QObject *instance() =0;
  virtual void startClientEncryption() =0;
  virtual QSsl::SslProtocol protocol() const =0;
  virtual void setProtocol(QSsl::SslProtocol AProtocol) =0;
  virtual void addCaCertificate(const QSslCertificate &ACertificate) =0;
  virtual QList<QSslCertificate> caCertificates() const =0;
  virtual QSslCertificate peerCertificate() const =0;
  virtual void ignoreSslErrors() =0;
  virtual QList<QSslError> sslErrors() const =0;
signals:
  virtual void modeChanged(QSslSocket::SslMode AMode) =0;
  virtual void sslErrors(const QList<QSslError> &AErrors) =0;
};

class IDefaultConnectionPlugin
{
public:
  virtual QObject *instance() =0;
  virtual QStringList proxyTypeNames() const =0;
};

Q_DECLARE_INTERFACE(IDefaultConnection,"Vacuum.Plugin.IDefaultConnection/1.0")
Q_DECLARE_INTERFACE(IDefaultConnectionPlugin,"Vacuum.Plugin.IDefaultConnectionPlugin/1.0")

#endif