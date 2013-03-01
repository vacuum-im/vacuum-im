#ifndef SOCKSSTREAM_H
#define SOCKSSTREAM_H

#include <QTimer>
#include <QWaitCondition>
#include <QReadWriteLock>
#include <definitions/namespaces.h>
#include <definitions/internalerrors.h>
#include <interfaces/isocksstreams.h>
#include <interfaces/idatastreamsmanager.h>
#include <interfaces/istanzaprocessor.h>
#include <utils/stanza.h>
#include <utils/xmpperror.h>
#include <utils/ringbuffer.h>

struct HostInfo {
	Jid jid;
	QString name;
	quint16 port;
};

class SocksStream :
	public QIODevice,
	public ISocksStream,
	public IStanzaHandler,
	public IStanzaRequestOwner
{
	Q_OBJECT;
	Q_INTERFACES(ISocksStream IDataStreamSocket IStanzaHandler IStanzaRequestOwner);
public:
	SocksStream(ISocksStreams *ASocksStreams, IStanzaProcessor *AStanzaProcessor, const QString &AStreamId, const Jid &AStreamJid, const Jid &AContactJid, int AKind, QObject *AParent=NULL);
	~SocksStream();
	virtual QIODevice *instance() { return this; }
	//IStanzaHandler
	virtual bool stanzaReadWrite(int AHandleId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept);
	//IStanzaRequestOwner
	virtual void stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza);
	//QIODevice
	virtual bool isSequential() const;
	virtual qint64 bytesAvailable() const;
	virtual qint64 bytesToWrite() const;
	virtual bool waitForBytesWritten(int AMsecs);
	virtual bool waitForReadyRead(int AMsecs);
	//IDataStreamSocket
	virtual QString methodNS() const;
	virtual QString streamId() const;
	virtual Jid streamJid() const;
	virtual Jid contactJid() const;
	virtual int streamKind() const;
	virtual int streamState() const;
	virtual XmppError error() const;
	virtual bool isOpen() const;
	virtual bool open(QIODevice::OpenMode AMode);
	virtual bool flush();
	virtual void close();
	virtual void abort(const XmppError &AError);
	//ISocksStream
	virtual int connectTimeout() const;
	virtual void setConnectTimeout(int ATimeout);
	virtual bool isDirectConnectionsDisabled() const;
	virtual void setDirectConnectionsDisabled(bool ADisabled);
	virtual QString forwardHost() const;
	virtual quint16 forwardPort() const;
	virtual void setForwardAddress(const QString &AHost, quint16 APort);
	virtual QNetworkProxy networkProxy() const;
	virtual void setNetworkProxy(const QNetworkProxy &AProxy);
	virtual QList<QString> proxyList() const;
	virtual void setProxyList(const QList<QString> &AProxyList);
signals:
	void stateChanged(int AState);
	void propertiesChanged();
protected:
	virtual qint64 readData(char *AData, qint64 AMaxSize);
	virtual qint64 writeData(const char *AData, qint64 AMaxSize);
	void setOpenMode(OpenMode AMode);
	virtual bool event(QEvent *AEvent);
protected:
	void setStreamState(int AState);
	void setStreamError(const XmppError &AError);
	void setTcpSocket(QTcpSocket *ASocket);
	void writeBufferedData(bool AFlush);
	void readBufferedData(bool AFlush);
	int insertStanzaHandle(const QString &ACondition);
	void removeStanzaHandle(int &AHandleId);
protected:
	bool negotiateConnection(int ACommand);
	bool requestProxyAddress();
	bool sendAvailHosts();
	bool connectToHost();
	bool sendUsedHost();
	bool sendFailedHosts();
	bool activateStream();
protected slots:
	void onHostSocketProxyAuthenticationRequired(const QNetworkProxy &AProxy, QAuthenticator *AAuth);
	void onHostSocketConnected();
	void onHostSocketReadyRead();
	void onHostSocketError(QAbstractSocket::SocketError AError);
	void onHostSocketDisconnected();
	void onTcpSocketReadyRead();
	void onTcpSocketBytesWritten(qint64 ABytes);
	void onTcpSocketError(QAbstractSocket::SocketError AError);
	void onTcpSocketDisconnected();
	void onLocalConnectionAccepted(const QString &AKey, QTcpSocket *ATcpSocket);
	void onCloseTimerTimeout();
private:
	ISocksStreams *FSocksStreams;
	IStanzaProcessor *FStanzaProcessor;
private:
	Jid FStreamJid;
	Jid FContactJid;
	int FStreamKind;
	int FStreamState;
	XmppError FError;
	QString FStreamId;
private:
	int FConnectTimeout;
	bool FDirectConnectDisabled;
	QString FForwardHost;
	quint16 FForwardPort;
	QList<QString> FProxyList;
	QNetworkProxy FNetworkProxy;
private:
	int FSHIHosts;
	QString FHostRequest;
	QString FActivateRequest;
	QList<QString> FProxyRequests;
private:
	int FHostIndex;
	QTimer FCloseTimer;
	QString FConnectKey;
	QTcpSocket *FTcpSocket;
	QList<HostInfo> FHosts;
private:
	RingBuffer FReadBuffer;
	RingBuffer FWriteBuffer;
	mutable QReadWriteLock FThreadLock;
	QWaitCondition FReadyReadCondition;
	QWaitCondition FBytesWrittenCondition;
};

#endif // SOCKSSTREAM_H
