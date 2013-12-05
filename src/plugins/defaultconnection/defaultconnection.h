#ifndef DEFAULTCONNECTION_H
#define DEFAULTCONNECTION_H

#include <QDnsLookup>
#include <definitions/internalerrors.h>
#include <interfaces/idefaultconnection.h>
#include <utils/xmpperror.h>

struct SrvRecord {
	QString target;
	quint16 port;
};

class DefaultConnection :
	public QObject,
	public IDefaultConnection
{
	Q_OBJECT;
	Q_INTERFACES(IConnection IDefaultConnection);
public:
	DefaultConnection(IConnectionPlugin *APlugin, QObject *AParent);
	~DefaultConnection();
	//IConnection
	virtual QObject *instance() { return this; }
	virtual bool isOpen() const;
	virtual bool isEncrypted() const;
	virtual bool connectToHost();
	virtual void disconnectFromHost();
	virtual void abortConnection(const XmppError &AError);
	virtual qint64 write(const QByteArray &AData);
	virtual QByteArray read(qint64 ABytes);
	virtual IConnectionPlugin *ownerPlugin() const;
	virtual QSslCertificate hostCertificate() const;
	//IDefaultConnection
	virtual void startClientEncryption();
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
	void onDnsLookupFinished();
	void onSocketProxyAuthenticationRequired(const QNetworkProxy &AProxy, QAuthenticator *AAuth);
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
	bool FUseLegacySSL;
	bool FDisconnecting;
	QSslSocket FSocket;
	CertificateVerifyMode FVerifyMode;
	QDnsLookup FDnsLookup;
	QList<SrvRecord> FRecords;
private:
	QMap<int, QVariant> FOptions;
};

#endif // DEFAULTCONNECTION_H
