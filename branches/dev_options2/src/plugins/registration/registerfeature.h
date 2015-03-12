#ifndef REGISTERFEATURE_H
#define REGISTERFEATURE_H

#include <interfaces/idataforms.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/iregistraton.h>
#include <interfaces/iconnectionmanager.h>

class Registration;

class RegisterFeature :
	public QObject,
	public IXmppFeature,
	public IXmppStanzaHadler
{
	Q_OBJECT;
	Q_INTERFACES(IXmppFeature IXmppStanzaHadler);
public:
	RegisterFeature(IXmppStream *AXmppStream);
	~RegisterFeature();
	virtual QObject *instance() { return this; }
	//IXmppStanzaHadler
	virtual bool xmppStanzaIn(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder);
	virtual bool xmppStanzaOut(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder);
	//IXmppFeature
	virtual QString featureNS() const;
	virtual IXmppStream *xmppStream() const;
	virtual bool start(const QDomElement &AElem);
	//RegisterFeature
	bool isFinished() const;
	IRegisterSubmit sentSubmit() const;
	bool sendSubmit(const IRegisterSubmit &ASubmit);
signals:
	//IXmppFeature
	void finished(bool ARestart);
	void error(const XmppError &AError);
	void featureDestroyed();
	//RegisterFeature
	void registerFields(const IRegisterFields &AFields);
private:
	IXmppStream *FXmppStream;
	Registration *FRegistration;
private:
	bool FFinished;
	IRegisterSubmit FSubmit;
};

#endif // REGISTERFEATURE_H
