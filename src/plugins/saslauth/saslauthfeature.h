#ifndef SASLAUTHFEATURE_H
#define SASLAUTHFEATURE_H

#include <interfaces/ixmppstreammanager.h>
#include <interfaces/iconnectionmanager.h>

class SASLAuthFeature :
	public QObject,
	public IXmppFeature,
	public IXmppStanzaHadler
{
	Q_OBJECT;
	Q_INTERFACES(IXmppFeature IXmppStanzaHadler);
public:
	SASLAuthFeature(IXmppStream *AXmppStream);
	~SASLAuthFeature();
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
	void sendAuthRequest(const QStringList &AMechanisms);
protected slots:
	void onXmppStreamPasswordProvided(const QString &APassword);
private:
	IXmppStream *FXmppStream;
	QStringList FMechanisms;
};

#endif // SASLAUTHFEATURE_H
