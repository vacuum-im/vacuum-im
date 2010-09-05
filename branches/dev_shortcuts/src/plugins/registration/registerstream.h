#ifndef REGISTERSTREAM_H
#define REGISTERSTREAM_H

#include <definitions/namespaces.h>
#include <definitions/xmppstanzahandlerorders.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/iregistraton.h>
#include <utils/errorhandler.h>
#include <utils/stanza.h>

class RegisterStream :
			public QObject,
			public IXmppFeature,
			public IXmppStanzaHadler
{
	Q_OBJECT;
	Q_INTERFACES(IXmppFeature IXmppStanzaHadler);
public:
	RegisterStream(IXmppStream *AXmppStream);
	~RegisterStream();
	virtual QObject *instance() { return this; }
	//IXmppStanzaHadler
	virtual bool xmppStanzaIn(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder);
	virtual bool xmppStanzaOut(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder);
	//IXmppFeature
	virtual QString featureNS() const { return NS_FEATURE_REGISTER; }
	virtual IXmppStream *xmppStream() const { return FXmppStream; }
	virtual bool start(const QDomElement &AElem);
signals:
	void finished(bool ARestart);
	void error(const QString &AMessage);
	void featureDestroyed();
private:
	IXmppStream *FXmppStream;
};

#endif // REGISTERSTREAM_H
