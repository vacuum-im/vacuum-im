#ifndef SASLSESSIONFEATURE_H
#define SASLSESSIONFEATURE_H

#include <interfaces/ipluginmanager.h>
#include <interfaces/ixmppstreammanager.h>

class SASLSessionFeature :
	public QObject,
	public IXmppFeature,
	public IXmppStanzaHadler
{
	Q_OBJECT;
	Q_INTERFACES(IXmppFeature IXmppStanzaHadler);
public:
	SASLSessionFeature(IXmppStream *AXmppStream);
	~SASLSessionFeature();
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
private:
	IXmppStream *FXmppStream;
};

#endif // SASLSESSIONFEATURE_H
