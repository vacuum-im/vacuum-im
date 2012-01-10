#ifndef MESSAGEARCHIVER_H
#define MESSAGEARCHIVER_H

#include <QPair>
#include <QList>
#include <QUuid>
#include <QMultiMap>
#include <definitions/namespaces.h>
#include <definitions/actiongroups.h>
#include <definitions/toolbargroups.h>
#include <definitions/optionvalues.h>
#include <definitions/optionnodes.h>
#include <definitions/optionnodeorders.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/rosterindextyperole.h>
#include <definitions/rosterlabelorders.h>
#include <definitions/stanzahandlerorders.h>
#include <definitions/sessionnegotiatororders.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/shortcuts.h>
#include <definitions/shortcutgrouporders.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imessagearchiver.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/imessagestyles.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/iprivatestorage.h>
#include <interfaces/iaccountmanager.h>
#include <interfaces/irostersview.h>
#include <interfaces/imultiuserchat.h>
#include <interfaces/iservicediscovery.h>
#include <interfaces/idataforms.h>
#include <interfaces/isessionnegotiation.h>
#include <interfaces/iroster.h>
#include <utils/options.h>
#include <utils/shortcuts.h>
#include <utils/errorhandler.h>
#include <utils/widgetmanager.h>
#include "archiveoptions.h"
#include "chatwindowmenu.h"

struct StanzaSession {
	QString sessionId;
	bool defaultPrefs;
	QString saveMode;
	QString requestId;
	QString error;
};

class MessageArchiver :
	public QObject,
	public IPlugin,
	public IMessageArchiver,
	public IStanzaHandler,
	public IStanzaRequestOwner,
	public IOptionsHolder,
	public ISessionNegotiator
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IMessageArchiver IStanzaHandler IStanzaRequestOwner IOptionsHolder ISessionNegotiator);
public:
	MessageArchiver();
	~MessageArchiver();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return MESSAGEARCHIVER_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin()  { return true; }
	//IStanzaHandler
	virtual bool stanzaReadWrite(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept);
	//IStanzaRequestOwner
	virtual void stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza);
	//IOptionsHolder
	virtual QMultiMap<int, IOptionsWidget *> optionsWidgets(const QString &ANodeId, QWidget *AParent);
	//SessionNegotiator
	virtual int sessionInit(const IStanzaSession &ASession, IDataForm &ARequest);
	virtual int sessionAccept(const IStanzaSession &ASession, const IDataForm &ARequest, IDataForm &ASubmit);
	virtual int sessionApply(const IStanzaSession &ASession);
	virtual void sessionLocalize(const IStanzaSession &ASession, IDataForm &AForm);
	//IMessageArchiver
	virtual bool isReady(const Jid &AStreamJid) const;
	virtual bool isArchivePrefsEnabled(const Jid &AStreamJid) const;
	virtual bool isSupported(const Jid &AStreamJid, const QString &AFeatureNS) const;
	virtual bool isAutoArchiving(const Jid &AStreamJid) const;
	virtual bool isArchivingAllowed(const Jid &AStreamJid, const Jid &AItemJid, const QString &AThreadId) const;
	virtual QString expireName(int AExpire) const;
	virtual QString methodName(const QString &AMethod) const;
	virtual QString otrModeName(const QString &AOTRMode) const;
	virtual QString saveModeName(const QString &ASaveMode) const;
	virtual IArchiveStreamPrefs archivePrefs(const Jid &AStreamJid) const;
	virtual IArchiveItemPrefs archiveItemPrefs(const Jid &AStreamJid, const Jid &AItemJid, const QString &AThreadId = QString::null) const;
	virtual QString setArchiveAutoSave(const Jid &AStreamJid, bool AAuto);
	virtual QString setArchivePrefs(const Jid &AStreamJid, const IArchiveStreamPrefs &APrefs);
	virtual QString removeArchiveItemPrefs(const Jid &AStreamJid, const Jid &AItemJid);
	virtual QString removeArchiveSessionPrefs(const Jid &AStreamJid, const QString &AThreadId);
	//Direct Archiving
	virtual bool saveMessage(const Jid &AStreamJid, const Jid &AItemJid, const Message &AMessage);
	virtual bool saveNote(const Jid &AStreamJid, const Jid &AItemJid, const QString &ANote, const QString &AThreadId = QString::null);
	//Archive Utilities
	virtual void elementToCollection(const QDomElement &AChatElem, IArchiveCollection &ACollection) const;
	virtual void collectionToElement(const IArchiveCollection &ACollection, QDomElement &AChatElem, const QString &ASaveMode) const;
	//Archive Handlers
	virtual void insertArchiveHandler(int AOrder, IArchiveHandler *AHandler);
	virtual void removeArchiveHandler(int AOrder, IArchiveHandler *AHandler);
	//Archive Engines
	virtual QList<IArchiveEngine *> archiveEngines() const;
	virtual bool isArchiveEngineEnabled(const QUuid &AId) const;
	virtual IArchiveEngine *findArchiveEngine(const QUuid &AId) const;
	virtual void registerArchiveEngine(IArchiveEngine *AEngine);
signals:
	void archivePrefsOpened(const Jid &AStreamJid);
	void archivePrefsChanged(const Jid &AStreamJid);
	void archivePrefsClosed(const Jid &AStreamJid);
	void requestCompleted(const QString &AId);
	void requestFailed(const QString &AId, const QString &AError);
protected:
	void registerDiscoFeatures();
	QString loadServerPrefs(const Jid &AStreamJid);
	QString loadStoragePrefs(const Jid &AStreamJid);
	void applyArchivePrefs(const Jid &AStreamJid, const QDomElement &AElem);
	bool prepareMessage(const Jid &AStreamJid, Message &AMessage, bool ADirectionIn);
	bool processMessage(const Jid &AStreamJid, const Message &AMessage, bool ADirectionIn);
	IArchiveEngine *findEngineByCapability(quint32 ACapability, const Jid &AStreamJid) const;
	QMultiMap<int, IArchiveEngine *> engineOrderByCapability(quint32 ACapability, const Jid &AStreamJid) const;
	void openHistoryOptionsNode(const Jid &AStreamJid);
	void closeHistoryOptionsNode(const Jid &AStreamJid);
	Menu *createContextMenu(const Jid &AStreamJid, const QStringList &AContacts, QWidget *AParent) const;
	void notifyInChatWindow(const Jid &AStreamJid, const Jid &AContactJid, const QString &AMessage) const;
	bool hasStanzaSession(const Jid &AStreamJid, const Jid &AContactJid) const;
	bool isOTRStanzaSession(const IStanzaSession &ASession) const;
	bool isOTRStanzaSession(const Jid &AStreamJid, const Jid &AContactJid) const;
	QString stanzaSessionDirPath(const Jid &AStreamJid) const;
	void saveStanzaSessionContext(const Jid &AStreamJid, const Jid &AContactJid) const;
	void restoreStanzaSessionContext(const Jid &AStreamJid, const QString &ASessionId = QString::null);
	void removeStanzaSessionContext(const Jid &AStreamJid, const QString &ASessionId) const;
	void startSuspendedStanzaSession(const Jid &AStreamJid, const QString &ARequestId);
	void cancelSuspendedStanzaSession(const Jid &AStreamJid, const QString &ARequestId, const QString &AError);
	void renegotiateStanzaSessions(const Jid &AStreamJid) const;
	bool isSelectionAccepted(const QList<IRosterIndex *> &ASelected) const;
protected slots:
	void onStreamOpened(IXmppStream *AXmppStream);
	void onStreamClosed(IXmppStream *AXmppStream);
	void onPrivateDataChanged(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement);
	void onPrivateDataError(const QString &AId, const QString &AError);
	void onShortcutActivated(const QString &AId, QWidget *AWidget);
	void onRosterIndexMultiSelection(const QList<IRosterIndex *> &ASelected, bool &AAccepted);
	void onRosterIndexContextMenu(const QList<IRosterIndex *> &AIndexes, int ALabelId, Menu *AMenu);
	void onMultiUserContextMenu(IMultiUserChatWindow *AWindow, IMultiUser *AUser, Menu *AMenu);
	void onSetMethodByAction(bool);
	void onSetItemPrefsByAction(bool);
	void onShowArchiveWindowByAction(bool);
	void onShowArchiveWindowByToolBarAction(bool);
	void onShowHistoryOptionsDialogByAction(bool);
	void onRemoveItemPrefsByAction(bool);
	void onDiscoInfoReceived(const IDiscoInfo &AInfo);
	void onStanzaSessionActivated(const IStanzaSession &ASession);
	void onStanzaSessionTerminated(const IStanzaSession &ASession);
	void onToolBarWidgetCreated(IToolBarWidget *AWidget);
	void onToolBarSettingsMenuAboutToShow();
private:
	IPluginManager *FPluginManager;
	IXmppStreams *FXmppStreams;
	IStanzaProcessor *FStanzaProcessor;
	IOptionsManager *FOptionsManager;
	IPrivateStorage *FPrivateStorage;
	IAccountManager *FAccountManager;
	IRosterPlugin *FRosterPlugin;
	IRostersViewPlugin *FRostersViewPlugin;
	IServiceDiscovery *FDiscovery;
	IDataForms *FDataForms;
	IMessageWidgets *FMessageWidgets;
	ISessionNegotiation *FSessionNegotiation;
	IMultiUserChatPlugin *FMultiUserChatPlugin;
private:
	QMap<Jid,int> FSHIPrefs;
	QMap<Jid,int> FSHIMessageIn;
	QMap<Jid,int> FSHIMessageOut;
	QMap<Jid,int> FSHIMessageBlocks;
private:
	QMap<QString,Jid> FPrefsSaveRequests;
	QMap<QString,Jid> FPrefsLoadRequests;
	QMap<QString,bool> FPrefsAutoRequests;
	QMap<QString,Jid> FPrefsRemoveItemRequests;
	QMap<QString,QString> FPrefsRemoveSessionRequests;
private:
	QList<Jid> FInStoragePrefs;
	QMap<Jid,QString> FNamespaces;
	QMap<Jid,QList<QString> > FFeatures;
	QMap<Jid,IArchiveStreamPrefs> FArchivePrefs;
	QMap<Jid,QList< QPair<Message,bool> > > FPendingMessages;
private:
	QMap<QString,QString> FRestoreRequests;
	QMap<Jid,QMap<Jid,StanzaSession> > FSessions;
private:
	QMap<QUuid,IArchiveEngine *> FArchiveEngines;
	QMultiMap<int,IArchiveHandler *> FArchiveHandlers;
};

#endif // MESSAGEARCHIVER_H
