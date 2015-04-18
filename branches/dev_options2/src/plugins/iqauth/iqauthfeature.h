#ifndef IQAUTHFEATURE_H
#define IQAUTHFEATURE_H

#include <interfaces/ixmppstreammanager.h>
#include <interfaces/iconnectionmanager.h>

class IqAuthFeature :
	public QObject,
	public IXmppFeature,
	public IXmppStanzaHadler
{
	Q_OBJECT;
	Q_INTERFACES(IXmppFeature IXmppStanzaHadler);
public:
	IqAuthFeature(IXmppStream *AXmppStream);
	~IqAuthFeature();
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
protected:
	void sendAuthRequest();
protected slots:
	void onXmppStreamPasswordProvided(const QString &APassword);
private:
	bool FPasswordRequested;
	IXmppStream *FXmppStream;
};

#endif // IQAUTHFEATURE_H
