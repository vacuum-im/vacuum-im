#ifndef SASLBINDFEATURE_H
#define SASLBINDFEATURE_H

#include <interfaces/ixmppstreammanager.h>

class SASLBindFeature :
	public QObject,
	public IXmppFeature,
	public IXmppStanzaHadler
{
	Q_OBJECT;
	Q_INTERFACES(IXmppFeature IXmppStanzaHadler);
public:
	SASLBindFeature(IXmppStream *AXmppStream);
	~SASLBindFeature();
	virtual QObject *instance() { return this; }
	//IXmppStanzaHadler
	virtual bool xmppStanzaIn(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder);
	virtual bool xmppStanzaOut(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder);
	//IXmppFeature
	virtual QString featureNS() const;
	virtual IXmppStream *xmppStream() const;
	virtual bool start(const QDomElement &AElem);
signals:
	void finished(bool ARestart);
	void error(const XmppError &AError);
	void featureDestroyed();
private:
	IXmppStream *FXmppStream;
};

#endif // SASLBINDFEATURE_H
