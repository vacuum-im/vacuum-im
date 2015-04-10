#ifndef SESSIONNEGOTIATION_H
#define SESSIONNEGOTIATION_H

#include <interfaces/ipluginmanager.h>
#include <interfaces/isessionnegotiation.h>
#include <interfaces/idataforms.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/ixmppstreammanager.h>
#include <interfaces/iservicediscovery.h>
#include <interfaces/ipresencemanager.h>
#include <interfaces/inotifications.h>

class SessionNegotiation :
	public QObject,
	public IPlugin,
	public ISessionNegotiation,
	public IStanzaHandler,
	public IDiscoFeatureHandler,
	public ISessionNegotiator,
	public IDataLocalizer
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin ISessionNegotiation IStanzaHandler IDiscoFeatureHandler ISessionNegotiator IDataLocalizer);
public:
	SessionNegotiation();
	~SessionNegotiation();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return SESSIONNEGOTIATION_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings() { return true; }
	virtual bool startPlugin() { return true; }
	//IStanzaHandler
	virtual bool stanzaReadWrite(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept);
	//IDiscoFeatureHandler
	virtual bool execDiscoFeature(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo);
	virtual Action *createDiscoFeatureAction(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo, QWidget *AParent);
	//IDataLocaliser
	virtual IDataFormLocale dataFormLocale(const QString &AFormType);
	//ISessionNegotiator
	virtual int sessionInit(const IStanzaSession &ASession, IDataForm &ARequest);
	virtual int sessionAccept(const IStanzaSession &ASession, const IDataForm &ARequest, IDataForm &ASubmit);
	virtual int sessionApply(const IStanzaSession &ASession);
	virtual void sessionLocalize(const IStanzaSession &ASession, IDataForm &AForm);
	//ISessionNegotiation
	virtual bool isReady(const Jid &AStreamJid) const;
	virtual IStanzaSession findSession(const QString &ASessionId) const;
	virtual IStanzaSession findSession(const Jid &AStreamJid, const Jid &AContactJid) const;
	virtual QList<IStanzaSession> findSessions(const Jid &AStreamJid, int AStatus = IStanzaSession::Active) const;
	virtual int initSession(const Jid &AStreamJid, const Jid &AContactJid);
	virtual void resumeSession(const Jid &AStreamJid, const Jid &AContactJid);
	virtual void terminateSession(const Jid &AStreamJid, const Jid &AContactJid);
	virtual void showSessionParams(const Jid &AStreamJid, const Jid &AContactJid);
	virtual void insertNegotiator(ISessionNegotiator *ANegotiator, int AOrder);
	virtual void removeNegotiator(ISessionNegotiator *ANegotiator, int AOrder);
signals:
	void sessionsOpened(const Jid &AStreamJid);
	void sessionsClosed(const Jid &AStreamJid);
	void sessionActivated(const IStanzaSession &ASession);
	void sessionTerminated(const IStanzaSession &ASession);
protected:
	bool sendSessionData(const IStanzaSession &ASession, const IDataForm &AForm) const;
	bool sendSessionError(const IStanzaSession &ASession, const IDataForm &ARequest) const;
	void processAccept(IStanzaSession &ASession, const IDataForm &ARequest);
	void processApply(IStanzaSession &ASession, const IDataForm &ASubmit);
	void processRenegotiate(IStanzaSession &ASession, const IDataForm &ARequest);
	void processContinue(IStanzaSession &ASession, const IDataForm &ARequest);
	void processTerminate(IStanzaSession &ASession, const IDataForm &ARequest);
	void showAcceptDialog(const IStanzaSession &ASession, const IDataForm &ARequest);
	void closeAcceptDialog(const IStanzaSession &ASession);
	void localizeSession(IStanzaSession &ASession, IDataForm &AForm) const;
	void removeSession(const IStanzaSession &ASession);
protected:
	void registerDiscoFeatures();
	void updateFields(const IDataForm &ASourse, IDataForm &ADestination, bool AInsert, bool ARemove) const;
	IDataForm defaultForm(const QString &AActionVar, const QVariant &AValue = true) const;
	IDataForm clearForm(const IDataForm &AForm) const;
	QStringList unsubmitedFields(const IDataForm &ARequest, const IDataForm &ASubmit, bool ARequired) const;
	IStanzaSession &dialogSession(IDataDialogWidget *ADialog);
protected:
	bool eventFilter(QObject *AObject, QEvent *AEvent);
protected slots:
	void onXmppStreamOpened(IXmppStream *AXmppStream);
	void onXmppStreamAboutToClose(IXmppStream *AXmppStream);
	void onXmppStreamClosed(IXmppStream *AXmppStream);
	void onPresenceItemReceived(IPresence *APresence, const IPresenceItem &AItem, const IPresenceItem &ABefore);
	void onNotificationActivated(int ANotifyId);
	void onAcceptDialogAccepted();
	void onAcceptDialogRejected();
	void onAcceptDialogDestroyed(IDataDialogWidget *ADialog);
	void onSessionActionTriggered(bool);
	void onDiscoInfoRecieved(const IDiscoInfo &AInfo);
private:
	IDataForms *FDataForms;
	IStanzaProcessor *FStanzaProcessor;
	IServiceDiscovery *FDiscovery;
	IPresenceManager *FPresenceManager;
	INotifications *FNotifications;
private:
	QHash<Jid,int> FSHISession;
	QHash<QString, IDataForm> FSuspended;
	QHash<QString, IDataForm> FRenegotiate;
	QMultiMap<int, ISessionNegotiator *> FNegotiators;
	QHash<Jid, QHash<Jid, IStanzaSession> > FSessions;
	QHash<Jid, QHash<Jid, IDataDialogWidget *> > FDialogs;
	QHash<int, IDataDialogWidget *> FDialogByNotify;
};

#endif // SESSIONNEGOTIATION_H
