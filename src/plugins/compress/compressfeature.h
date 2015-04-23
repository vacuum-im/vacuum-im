#ifndef COMPRESSFEATURE_H
#define COMPRESSFEATURE_H

#include <interfaces/ixmppstreammanager.h>

#ifdef USE_SYSTEM_ZLIB
#	include <zlib.h>
#else
#	include <thirdparty/zlib/zlib.h>
#endif

class CompressFeature :
	public QObject,
	public IXmppFeature,
	public IXmppDataHandler,
	public IXmppStanzaHadler
{
	Q_OBJECT;
	Q_INTERFACES(IXmppFeature IXmppDataHandler IXmppStanzaHadler);
public:
	CompressFeature(IXmppStream *AXmppStream);
	~CompressFeature();
	//IXmppDataHandler
	virtual bool xmppDataIn(IXmppStream *AXmppStream, QByteArray &AData, int AOrder);
	virtual bool xmppDataOut(IXmppStream *AXmppStream, QByteArray &AData, int AOrder);
	//IXmppStanzaHadler
	virtual bool xmppStanzaIn(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder);
	virtual bool xmppStanzaOut(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder);
	//IXmppFeature
	virtual QObject *instance() { return this; }
	virtual QString featureNS() const;
	virtual IXmppStream *xmppStream() const;
	virtual bool start(const QDomElement &AElem);
signals:
	void finished(bool ARestart);
	void error(const XmppError &AError);
	void featureDestroyed();
protected:
	bool startZlib();
	void stopZlib();
	void processData(QByteArray &AData, bool ADataOut);
private:
	IXmppStream *FXmppStream;
private:
	bool FZlibInited;
	z_stream FDefStruc;
	z_stream FInfStruc;
	QByteArray FOutBuffer;
};

#endif // COMPRESSFEATURE_H
