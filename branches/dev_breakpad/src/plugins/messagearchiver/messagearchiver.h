#ifndef MESSAGEARCHIVER_H
#define MESSAGEARCHIVER_H

#include <QPair>
#include <QList>
#include <QUuid>
#include <QMultiMap>
#include <definitions/namespaces.h>
#include <definitions/actiongroups.h>
#include <definitions/toolbargroups.h>
#include <definitions/messagedataroles.h>
#include <definitions/optionvalues.h>
#include <definitions/optionnodes.h>
#include <definitions/optionnodeorders.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/rosterindexkinds.h>
#include <definitions/rosterindexroles.h>
#include <definitions/stanzahandlerorders.h>
#include <definitions/sessionnegotiatororders.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/shortcuts.h>
#include <definitions/shortcutgrouporders.h>
#include <definitions/internalerrors.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imessagearchiver.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/imessagestyles.h>
#include <interfaces/imessageprocessor.h>
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
#include <utils/xmpperror.h>
#include <utils/widgetmanager.h>
#include "archivestreamoptions.h"
#include "chatwindowmenu.h"
#include "archiveviewwindow.h"
#include "archiveenginesoptions.h"

struct StanzaSession {
	QString sessionId;
	bool defaultPrefs;
	QString saveMode;
	QString requestId;
	XmppStanzaError error;
};

struct RemoveRequest {
	XmppError lastError;
	IArchiveRequest request;
	QList<IArchiveEngine *> engines;
};

struct MessagesRequest {
	Jid streamJid;
	XmppError lastError;
	IArchiveRequest request;
	QList<IArchiveHeader> headers;
	IArchiveCollectionBody body;
};

struct HeadersRequest {
	XmppError lastError;
	IArchiveRequest request;
	QList<IArchiveEngine *> engines;
	QMap<IArchiveEngine *,QList<IArchiveHeader> > headers;
};

struct CollectionRequest {
	XmppError lastError;
	IArchiveCollection collection;
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
	virtual bool isArchiveAutoSave(const Jid &AStreamJid) const;
	virtual bool isArchivingAllowed(const Jid &AStreamJid, const Jid &AItemJid, const QString &AThreadId) const;
	virtual QWidget *showArchiveWindow(const Jid &AStreamJid, const Jid &AContactJid = Jid::null);
  //Preferences
	virtual QString prefsNamespace(const Jid &AStreamJid) const;
	virtual IArchiveStreamPrefs archivePrefs(const Jid &AStreamJid) const;
	virtual IArchiveItemPrefs archiveItemPrefs(const Jid &AStreamJid, const Jid &AItemJid, const QString &AThreadId = QString::null) const;
	virtual QString setArchiveAutoSave(const Jid &AStreamJid, bool AAuto);
	virtual QString setArchivePrefs(const Jid &AStreamJid, const IArchiveStreamPrefs &APrefs);
	virtual QString removeArchiveItemPrefs(const Jid &AStreamJid, const Jid &AItemJid);
	virtual QString removeArchiveSessionPrefs(const Jid &AStreamJid, const QString &AThreadId);
	//Direct Archiving
	virtual bool saveMessage(const Jid &AStreamJid, const Jid &AItemJid, const Message &AMessage);
	virtual bool saveNote(const Jid &AStreamJid, const Jid &AItemJid, const QString &ANote, const QString &AThreadId = QString::null);
	//Archive Management
	virtual QString loadMessages(const Jid &AStreamJid, const IArchiveRequest &ARequest);
	virtual QString loadHeaders(const Jid &AStreamJid, const IArchiveRequest &ARequest);
	virtual QString loadCollection(const Jid &AStreamJid, const IArchiveHeader &AHeader);
	virtual QString removeCollections(const Jid &AStreamJid, const IArchiveRequest &ARequest);
	//Utilities
	virtual void elementToCollection(const QDomElement &AChatElem, IArchiveCollection &ACollection) const;
	virtual void collectionToElement(const IArchiveCollection &ACollection, QDomElement &AChatElem, const QString &ASaveMode) const;
	//Handlers
	virtual void insertArchiveHandler(int AOrder, IArchiveHandler *AHandler);
	virtual void removeArchiveHandler(int AOrder, IArchiveHandler *AHandler);
	//Engines
	virtual quint32 totalCapabilities(const Jid &AStreamJid) const;
	virtual QList<IArchiveEngine *> archiveEngines() const;
	virtual IArchiveEngine *findArchiveEngine(const QUuid &AId) const;
	virtual bool isArchiveEngineEnabled(const QUuid &AId) const;
	virtual void setArchiveEngineEnabled(const QUuid &AId, bool AEnabled);
	virtual void registerArchiveEngine(IArchiveEngine *AEngine);
signals:
	//Common Requests
	void requestCompleted(const QString &AId);
	void requestFailed(const QString &AId, const XmppError &AError);
	//Archive Preferences
	void archivePrefsOpened(const Jid &AStreamJid);
	void archivePrefsChanged(const Jid &AStreamJid);
	void archivePrefsClosed(const Jid &AStreamJid);
	//Archive Management
	void messagesLoaded(const QString &AId, const IArchiveCollectionBody &ABody);
	void headersLoaded(const QString &AId, const QList<IArchiveHeader> &AHeaders);
	void collectionLoaded(const QString &AId, const IArchiveCollection &ACollection);
	void collectionsRemoved(const QString &AId, const IArchiveRequest &ARequest);
	//Engines
	void totalCapabilitiesChanged(const Jid &AStreamJid);
	void archiveEngineRegistered(IArchiveEngine *AEngine);
	void archiveEngineEnableChanged(const QUuid &AId, bool AEnabled);
protected:
	QString archiveStreamDirPath(const Jid &AStreamJid) const;
	QString archiveStreamFilePath(const Jid &AStreamJid, const QString &AFileName) const;
protected:
	QString loadServerPrefs(const Jid &AStreamJid);
	QString loadStoragePrefs(const Jid &AStreamJid);
	void applyArchivePrefs(const Jid &AStreamJid, const QDomElement &AElem);
protected:
	void loadPendingMessages(const Jid &AStreamJid);
	void savePendingMessages(const Jid &AStreamJid);
	void processPendingMessages(const Jid &AStreamJid);
	bool prepareMessage(const Jid &AStreamJid, Message &AMessage, bool ADirectionIn);
	bool processMessage(const Jid &AStreamJid, const Message &AMessage, bool ADirectionIn);
protected:
	IArchiveEngine *findEngineByCapability(quint32 ACapability, const Jid &AStreamJid) const;
	QMultiMap<int, IArchiveEngine *> engineOrderByCapability(quint32 ACapability, const Jid &AStreamJid) const;
protected:
	void processRemoveRequest(const QString &ALocalId, RemoveRequest &ARequest);
	void processHeadersRequest(const QString &ALocalId, HeadersRequest &ARequest);
	void processCollectionRequest(const QString &ALocalId, CollectionRequest &ARequest);
	void processMessagesRequest(const QString &ALocalId, MessagesRequest &ARequest);
protected:
	bool hasStanzaSession(const Jid &AStreamJid, const Jid &AContactJid) const;
	bool isOTRStanzaSession(const IStanzaSession &ASession) const;
	bool isOTRStanzaSession(const Jid &AStreamJid, const Jid &AContactJid) const;
	void saveStanzaSessionContext(const Jid &AStreamJid, const Jid &AContactJid) const;
	void restoreStanzaSessionContext(const Jid &AStreamJid, const QString &ASessionId = QString::null);
	void removeStanzaSessionContext(const Jid &AStreamJid, const QString &ASessionId) const;
	void startSuspendedStanzaSession(const Jid &AStreamJid, const QString &ARequestId);
	void cancelSuspendedStanzaSession(const Jid &AStreamJid, const QString &ARequestId, const XmppStanzaError &AError);
	void renegotiateStanzaSessions(const Jid &AStreamJid) const;
protected:
	void registerDiscoFeatures();
	void openHistoryOptionsNode(const Jid &AStreamJid);
	void closeHistoryOptionsNode(const Jid &AStreamJid);
	bool isSelectionAccepted(const QList<IRosterIndex *> &ASelected) const;
	Menu *createContextMenu(const QStringList &AStreams, const QStringList &AContacts, QWidget *AParent) const;
	void notifyInChatWindow(const Jid &AStreamJid, const Jid &AContactJid, const QString &AMessage) const;
protected slots:
	void onEngineCapabilitiesChanged(const Jid &AStreamJid);
	void onEngineRequestFailed(const QString &AId, const XmppError &AError);
	void onEngineCollectionsRemoved(const QString &AId, const IArchiveRequest &ARequest);
	void onEngineHeadersLoaded(const QString &AId, const QList<IArchiveHeader> &AHeaders);
	void onEngineCollectionLoaded(const QString &AId, const IArchiveCollection &ACollection);
	void onSelfRequestFailed(const QString &AId, const XmppError &AError);
	void onSelfHeadersLoaded(const QString &AId, const QList<IArchiveHeader> &AHeaders);
	void onSelfCollectionLoaded(const QString &AId, const IArchiveCollection &ACollection);
protected slots:
	void onStreamOpened(IXmppStream *AXmppStream);
	void onStreamClosed(IXmppStream *AXmppStream);
	void onPrivateDataLoadedSaved(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement);
	void onPrivateDataChanged(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace);
	void onShortcutActivated(const QString &AId, QWidget *AWidget);
	void onRostersViewIndexMultiSelection(const QList<IRosterIndex *> &ASelected, bool &AAccepted);
	void onRostersViewIndexContextMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu);
	void onMultiUserContextMenu(IMultiUserChatWindow *AWindow, IMultiUser *AUser, Menu *AMenu);
	void onSetItemPrefsByAction(bool);
	void onSetAutoArchivingByAction(bool);
	void onRemoveItemPrefsByAction(bool);
	void onShowArchiveWindowByAction(bool);
	void onShowArchiveWindowByToolBarAction(bool);
	void onShowHistoryOptionsDialogByAction(bool);
	void onDiscoInfoReceived(const IDiscoInfo &AInfo);
	void onStanzaSessionActivated(const IStanzaSession &ASession);
	void onStanzaSessionTerminated(const IStanzaSession &ASession);
	void onToolBarWidgetCreated(IMessageToolBarWidget *AWidget);
	void onOptionsChanged(const OptionsNode &ANode);
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
	QHash<QString,QString> FRequestId2LocalId;
	QMap<QString,RemoveRequest> FRemoveRequests;
	QMap<QString,HeadersRequest> FHeadersRequests;
	QMap<QString,CollectionRequest> FCollectionRequests;
	QMap<QString,MessagesRequest> FMesssagesRequests;
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
