#ifndef REGISTERSTREAM_H
#define REGISTERSTREAM_H

#include <definitions/namespaces.h>
#include <definitions/internalerrors.h>
#include <definitions/xmppstanzahandlerorders.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/iregistraton.h>
#include <interfaces/idataforms.h>
#include <interfaces/iconnectionmanager.h>
#include <utils/widgetmanager.h>
#include <utils/xmpperror.h>
#include <utils/stanza.h>

class RegisterStream :
	public QObject,
	public IXmppFeature,
	public IXmppStanzaHadler
{
	Q_OBJECT;
	Q_INTERFACES(IXmppFeature IXmppStanzaHadler);
public:
	RegisterStream(IDataForms *ADataForms, IXmppStream *AXmppStream);
	~RegisterStream();
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
	void onXmppStreamClosed();
	void onRegisterDialogAccepred();
	void onRegisterDialogRejected();
private:
	IDataForms *FDataForms;
	IXmppStream *FXmppStream;
private:
	IDataDialogWidget *FDialog;
};

#endif // REGISTERSTREAM_H
