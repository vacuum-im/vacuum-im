#ifndef STARTTLS_H
#define STARTTLS_H

#include <QObject>
#include <definitions/namespaces.h>
#include <definitions/internalerrors.h>
#include <definitions/xmppstanzahandlerorders.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/iconnectionmanager.h>
#include <interfaces/idefaultconnection.h>
#include <utils/stanza.h>
#include <utils/xmpperror.h>

class StartTLS :
	public QObject,
	public IXmppFeature,
	public IXmppStanzaHadler
{
	Q_OBJECT;
	Q_INTERFACES(IXmppFeature IXmppStanzaHadler);
public:
	StartTLS(IXmppStream *AXmppStream);
	~StartTLS();
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
	IDefaultConnection *FConnection;
};

#endif // STARTTLS_H
