#ifndef DEFAULTCONNECTION_H
#define DEFAULTCONNECTION_H

#include <interfaces/idefaultconnection.h>
#include <thirdparty/jdns/qjdns.h>
#include <utils/xmpperror.h>

class DefaultConnection :
	public QObject,
	public IDefaultConnection
{
	Q_OBJECT;
	Q_INTERFACES(IConnection IDefaultConnection);
public:
	DefaultConnection(IConnectionEngine *AEngine, QObject *AParent);
	~DefaultConnection();
	//IConnection
	virtual QObject *instance() { return this; }
	virtual bool isOpen() const;
	virtual bool isEncrypted() const;
	virtual bool isEncryptionSupported() const;
	virtual bool connectToHost();
	virtual bool startEncryption();
	virtual void disconnectFromHost();
	virtual void abortConnection(const XmppError &AError);
	virtual qint64 write(const QByteArray &AData);
	virtual QByteArray read(qint64 ABytes);
	virtual IConnectionEngine *engine() const;
	virtual QSslCertificate hostCertificate() const;
	//IDefaultConnection
	virtual void ignoreSslErrors();
	virtual QList<QSslError> sslErrors() const;
	virtual QSsl::SslProtocol protocol() const;
	virtual void setProtocol(QSsl::SslProtocol AProtocol);
	virtual QSslKey privateKey() const;
	virtual void setPrivateKey(const QSslKey &AKey);
	virtual QSslCertificate localCertificate() const;
	virtual void setLocalCertificate(const QSslCertificate &ACertificate);
	virtual QList<QSslCertificate> caCertificates() const;
	virtual void addCaSertificates(const QList<QSslCertificate> &ACertificates);
	virtual void setCaCertificates(const QList<QSslCertificate> &ACertificates);
	virtual QNetworkProxy proxy() const;
	virtual void setProxy(const QNetworkProxy &AProxy);
	virtual QVariant option(int ARole) const;
	virtual void setOption(int ARole, const QVariant &AValue);
signals:
	//IConnection
	void aboutToConnect();
	void connected();
	void encrypted();
	void readyRead(qint64 ABytes);
	void error(const XmppError &AError);
	void aboutToDisconnect();
	void disconnected();
	void connectionDestroyed();
	//IDefaultConnection
	void proxyChanged(const QNetworkProxy &AProxy);
	void sslErrorsOccured(const QList<QSslError> &AErrors);
protected:
	void connectToNextHost();
protected slots:
	void onDnsResultsReady(int AId, const QJDns::Response &AResults);
	void onDnsError(int AId, QJDns::Error AError);
	void onDnsShutdownFinished();
	void onSocketProxyAuthenticationRequired(const QNetworkProxy &AProxy, QAuthenticator *AAuth);
	void onSocketConnected();
	void onSocketEncrypted();
	void onSocketReadyRead();
	void onSocketSSLErrors(const QList<QSslError> &AErrors);
	void onSocketError(QAbstractSocket::SocketError AError);
	void onSocketDisconnected();
private:
	IConnectionEngine *FEngine;
private:
	QJDns FDns;
	int FSrvQueryId;
	QList<QJDns::Record> FRecords;
private:
	bool FSSLError;
	bool FDisconnecting;
	QSslSocket FSocket;
private:
	bool FUseLegacySSL;
	QMap<int, QVariant> FOptions;
	CertificateVerifyMode FVerifyMode;
};

#endif // DEFAULTCONNECTION_H
