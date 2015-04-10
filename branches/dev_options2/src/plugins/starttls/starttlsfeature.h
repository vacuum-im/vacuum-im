#ifndef STARTTLSFEATURE_H
#define STARTTLSFEATURE_H

#include <QObject>
#include <interfaces/ixmppstreammanager.h>
#include <interfaces/iconnectionmanager.h>
#include <interfaces/idefaultconnection.h>

class StartTLSFeature :
	public QObject,
	public IXmppFeature,
	public IXmppStanzaHadler
{
	Q_OBJECT;
	Q_INTERFACES(IXmppFeature IXmppStanzaHadler);
public:
	StartTLSFeature(IXmppStream *AXmppStream);
	~StartTLSFeature();
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
protected slots:
	void onConnectionEncrypted();
private:
	IXmppStream *FXmppStream;
};

#endif // STARTTLSFEATURE_H
