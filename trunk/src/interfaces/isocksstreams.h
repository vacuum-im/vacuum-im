#ifndef ISOCKSSTREAMS_H
#define ISOCKSSTREAMS_H

#include <QIODevice>
#include <QTcpSocket>
#include <QNetworkProxy>
#include <interfaces/idatastreamsmanager.h>

#define SOCKSSTREAMS_UUID "{6c7cb01e-64c5-4644-97ba-1d00b8c417c2}"

class ISocksStream :
	public IDataStreamSocket
{
public:
	virtual QIODevice *instance() =0;
	virtual int connectTimeout() const =0;
	virtual void setConnectTimeout(int ATimeout) =0;
	virtual bool isDirectConnectionEnabled() const =0;
	virtual void setDirectConnectionEnabled(bool AEnabled) =0;
	virtual bool isDerectConnectionForwardEnabled() const =0;
	virtual void setDirectConnectionForwardEnabled(bool AEnabled) =0;
	virtual QString directConnectionForwardHost() const =0;
	virtual quint16 directConnectionForwardPort() const =0;
	virtual void setDirectConnectionForwardAddress(const QString &AHost, quint16 APort) =0;
	virtual QNetworkProxy networkProxy() const =0;
	virtual void setNetworkProxy(const QNetworkProxy &AProxy) =0;
	virtual QList<QString> streamProxyList() const =0;
	virtual void setStreamProxyList(const QList<QString> &AProxyList) =0;
protected:
	virtual void propertiesChanged() =0;
};

class ISocksStreams :
	public IDataStreamMethod
{
public:
	virtual QObject *instance() =0;
	virtual quint16 listeningPort() const =0;
	virtual QString accountStreamProxy(const Jid &AStreamJid) const =0;
	virtual QNetworkProxy accountNetworkProxy(const Jid &AStreamJid) const =0;
	virtual QString connectionKey(const QString &ASessionId, const Jid &AInitiator, const Jid &ATarget) const =0;
	virtual bool appendLocalConnection(const QString &AKey) =0;
	virtual void removeLocalConnection(const QString &AKey) =0;
protected:
	virtual void localConnectionAccepted(const QString &AKey, QTcpSocket *ATcpSocket) =0;
};

Q_DECLARE_INTERFACE(ISocksStream,"Vacuum.Plugin.ISocksStream/1.2")
Q_DECLARE_INTERFACE(ISocksStreams,"Vacuum.Plugin.ISocksStreams/1.1")

#endif // ISOCKSSTREAMS_H
