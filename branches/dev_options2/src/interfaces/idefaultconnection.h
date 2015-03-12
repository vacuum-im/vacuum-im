#ifndef IDEFAULTCONNECTION_H
#define IDEFAULTCONNECTION_H

#define DEFAULTCONNECTION_UUID "{68F9B5F2-5898-43f8-9DD1-19F37E9779AC}"

#include <QSslSocket>
#include <interfaces/iconnectionmanager.h>

class IDefaultConnection :
	public IConnection
{
public:
	enum OptionRole {
		Host,
		Port,
		Domain,
		UseLegacySsl,
		CertVerifyMode,
	};
	enum CertificateVerifyMode {
		Disabled,
		Manual,
		Forbid,
		TrustedOnly
	};
public:
	virtual QObject *instance() =0;
	virtual void ignoreSslErrors() =0;
	virtual QList<QSslError> sslErrors() const =0;
	virtual QSsl::SslProtocol protocol() const =0;
	virtual void setProtocol(QSsl::SslProtocol AProtocol) =0;
	virtual QSslKey privateKey() const =0;
	virtual void setPrivateKey(const QSslKey &AKey) =0;
	virtual QSslCertificate localCertificate() const =0;
	virtual void setLocalCertificate(const QSslCertificate &ACertificate) =0;
	virtual QList<QSslCertificate> caCertificates() const =0;
	virtual void addCaSertificates(const QList<QSslCertificate> &ACertificates) =0;
	virtual void setCaCertificates(const QList<QSslCertificate> &ACertificates) =0;
	virtual QNetworkProxy proxy() const =0;
	virtual void setProxy(const QNetworkProxy &AProxy) =0;
	virtual QVariant option(int ARole) const =0;
	virtual void setOption(int ARole, const QVariant &AValue) =0;
protected:
	virtual void proxyChanged(const QNetworkProxy &AProxy) =0;
	virtual void sslErrorsOccured(const QList<QSslError> &AErrors) =0;
};

class IDefaultConnectionEngine :
	public IConnectionEngine
{
public:
	virtual QObject *instance() =0;
};

Q_DECLARE_INTERFACE(IDefaultConnection,"Vacuum.Plugin.IDefaultConnection/1.2")
Q_DECLARE_INTERFACE(IDefaultConnectionEngine,"Vacuum.Plugin.IDefaultConnectionEngine/1.2")

#endif // IDEFAULTCONNECTION_H
