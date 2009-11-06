#ifndef MESSAGEARCHIVER_H
#define MESSAGEARCHIVER_H

#include <QMultiMap>
#include <definations/namespaces.h>
#include <definations/actiongroups.h>
#include <definations/toolbargroups.h>
#include <definations/menubargroups.h>
#include <definations/optionnodes.h>
#include <definations/optionnodeorders.h>
#include <definations/optionwidgetorders.h>
#include <definations/rosterindextyperole.h>
#include <definations/stanzahandlerorders.h>
#include <definations/sessionnegotiatororders.h>
#include <definations/resources.h>
#include <definations/menuicons.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imessagearchiver.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/imessagestyles.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/isettings.h>
#include <interfaces/iprivatestorage.h>
#include <interfaces/iaccountmanager.h>
#include <interfaces/irostersview.h>
#include <interfaces/imultiuserchat.h>
#include <interfaces/iservicediscovery.h>
#include <interfaces/idataforms.h>
#include <interfaces/isessionnegotiation.h>
#include <interfaces/iroster.h>
#include <utils/errorhandler.h>
#include "collectionwriter.h"
#include "archiveoptions.h"
#include "replicator.h"
#include "viewhistorywindow.h"
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
  virtual bool stanzaEdit(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept);
  virtual bool stanzaRead(int AHandlerId, const Jid &AStreamJid, const Stanza &AStanza, bool &AAccept);
  //IStanzaRequestOwner
  virtual void stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza);
  virtual void stanzaRequestTimeout(const Jid &AStreamJid, const QString &AStanzaId);
  //IOptionsHolder
  virtual QWidget *optionsWidget(const QString &ANode, int &AOrder);
  //SessionNegotiator
  virtual int sessionInit(const IStanzaSession &ASession, IDataForm &ARequest);
  virtual int sessionAccept(const IStanzaSession &ASession, const IDataForm &ARequest, IDataForm &ASubmit);
  virtual int sessionApply(const IStanzaSession &ASession);
  virtual void sessionLocalize(const IStanzaSession &ASession, IDataForm &AForm);
  //IMessageArchiver
  virtual IPluginManager *pluginManager() const { return FPluginManager; }
  virtual bool isReady(const Jid &AStreamJid) const;
  virtual bool isSupported(const Jid &AStreamJid) const;
  virtual bool isAutoArchiving(const Jid &AStreamJid) const;
  virtual bool isManualArchiving(const Jid &AStreamJid) const;
  virtual bool isLocalArchiving(const Jid &AStreamJid) const;
  virtual bool isArchivingAllowed(const Jid &AStreamJid, const Jid &AItemJid, int AMessageType) const;
  virtual QString methodName(const QString &AMethod) const;
  virtual QString otrModeName(const QString &AOTRMode) const;
  virtual QString saveModeName(const QString &ASaveMode) const;
  virtual QString expireName(int AExpire) const;
  virtual IArchiveStreamPrefs archivePrefs(const Jid &AStreamJid) const;
  virtual IArchiveItemPrefs archiveItemPrefs(const Jid &AStreamJid, const Jid &AItemJid) const;
  virtual QString setArchiveAutoSave(const Jid &AStreamJid, bool AAuto);
  virtual QString setArchivePrefs(const Jid &AStreamJid, const IArchiveStreamPrefs &APrefs);
  virtual QString removeArchiveItemPrefs(const Jid &AStreamJid, const Jid &AItemJid);
  virtual IArchiveWindow *showArchiveWindow(const Jid &AStreamJid, const IArchiveFilter &AFilter, int AGroupKind, QWidget *AParent = NULL);
  //Archive Handlers
  virtual void insertArchiveHandler(IArchiveHandler *AHandler, int AOrder);
  virtual void removeArchiveHandler(IArchiveHandler *AHandler, int AOrder);
  //Direct Archiving
  virtual bool saveMessage(const Jid &AStreamJid, const Jid &AItemJid, const Message &AMessage);
  virtual bool saveNote(const Jid &AStreamJid, const Jid &AItemJid, const QString &ANote, const QString &AThreadId = "");
  //Local Archive
  virtual Jid gateJid(const Jid &AContactJid) const;
  virtual QString gateNick(const Jid &AStreamJid, const Jid &AContactJid) const;
  virtual QList<Message> findLocalMessages(const Jid &AStreamJid, const IArchiveRequest &ARequest) const;
  virtual bool hasLocalCollection(const Jid &AStreamJid, const IArchiveHeader &AHeader) const;
  virtual bool saveLocalCollection(const Jid &AStreamJid, const IArchiveCollection &ACollection, bool AAppend = true);
  virtual IArchiveHeader loadLocalHeader(const Jid &AStreamJid, const Jid &AWith, const QDateTime &AStart) const;
  virtual QList<IArchiveHeader> loadLocalHeaders(const Jid &AStreamJid, const IArchiveRequest &ARequest) const;
  virtual IArchiveCollection loadLocalCollection(const Jid &AStreamJid, const IArchiveHeader &AHeader) const;
  virtual bool removeLocalCollection(const Jid &AStreamJid, const IArchiveHeader &AHeader);
  virtual IArchiveModifications loadLocalModifications(const Jid &AStreamJid, const QDateTime &AStart, int ACount) const;
  //Server Archive
  virtual QDateTime replicationPoint(const Jid &AStreamJid) const;
  virtual bool replicationEnabled(const Jid &AStreamJid) const;
  virtual void setReplicationEnabled(const Jid &AStreamJid, bool AEnabled);
  virtual QString saveServerCollection(const Jid &AStreamJid, const IArchiveCollection &ACollection);
  virtual QString loadServerHeaders(const Jid AStreamJid, const IArchiveRequest &ARequest, const QString &AAfter = "");
  virtual QString loadServerCollection(const Jid AStreamJid, const IArchiveHeader &AHeader, const QString &AAfter = "");
  virtual QString removeServerCollections(const Jid &AStreamJid, const IArchiveRequest &ARequest, bool AOpened = false);
  virtual QString loadServerModifications(const Jid &AStreamJid, const QDateTime &AStart, int ACount, const QString &AAfter = "");
signals:
  virtual void archiveAutoSaveChanged(const Jid &AStreamJid, bool AAuto);
  virtual void archivePrefsChanged(const Jid &AStreamJid, const IArchiveStreamPrefs &APrefs);
  virtual void archiveItemPrefsChanged(const Jid &AStreamJid, const Jid &AItemJid, const IArchiveItemPrefs &APrefs);
  virtual void archiveItemPrefsRemoved(const Jid &AStreamJid, const Jid &AItemJid);
  virtual void requestCompleted(const QString &AId);
  virtual void requestFailed(const QString &AId, const QString &AError);
  //Local Archive
  virtual void localCollectionOpened(const Jid &AStreamJid, const IArchiveHeader &AHeader);
  virtual void localCollectionSaved(const Jid &AStreamJid, const IArchiveHeader &AHeader);
  virtual void localCollectionRemoved(const Jid &AStreamJid, const IArchiveHeader &AHeader);
  //Server Archive
  virtual void serverCollectionSaved(const QString &AId, const IArchiveHeader &AHeader);
  virtual void serverHeadersLoaded(const QString &AId, const QList<IArchiveHeader> &AHeaders,  const IArchiveResultSet &AResult);
  virtual void serverCollectionLoaded(const QString &AId, const IArchiveCollection &ACollection,  const IArchiveResultSet &AResult);
  virtual void serverCollectionsRemoved(const QString &AId, const IArchiveRequest &ARequest);
  virtual void serverModificationsLoaded(const QString &AId, const IArchiveModifications &AModifs,  const IArchiveResultSet &AResult);
  //ArchiveWindow
  virtual void archiveWindowCreated(IArchiveWindow *AWindow);
  virtual void archiveWindowDestroyed(IArchiveWindow *AWindow);
  //IOptionsHolder
  virtual void optionsAccepted();
  virtual void optionsRejected();
protected:
  QString loadServerPrefs(const Jid &AStreamJid);
  QString loadStoragePrefs(const Jid &AStreamJid);
  void applyArchivePrefs(const Jid &AStreamJid, const QDomElement &AElem);
  QString collectionDirName(const Jid &AJid) const;
  QString collectionFileName(const DateTime &AStart) const;
  QString collectionDirPath(const Jid &AStreamJid, const Jid &AWith) const;
  QString collectionFilePath(const Jid &AStreamJid, const Jid &AWith, const DateTime &AStart) const;
  QMultiMap<QString,QString> filterCollectionFiles(const QStringList &AFiles, const IArchiveRequest &ARequest, const QString &APrefix) const;
  QStringList findCollectionFiles(const Jid &AStreamJid, const IArchiveRequest &ARequest) const;
  IArchiveHeader loadCollectionHeader(const QString &AFileName) const;
  CollectionWriter *findCollectionWriter(const Jid &AStreamJid, const Jid &AWith, const QString &AThreadId) const;
  CollectionWriter *newCollectionWriter(const Jid &AStreamJid, const IArchiveHeader &AHeader);
  void elementToCollection(const QDomElement &AChatElem, IArchiveCollection &ACollection) const;
  void collectionToElement(const IArchiveCollection &ACollection, QDomElement &AChatElem, const QString &ASaveMode) const;
  bool saveLocalModofication(const Jid &AStreamJid, const IArchiveHeader &AHeader, const QString &AAction) const;
  Replicator *insertReplicator(const Jid &AStreamJid);
  void removeReplicator(const Jid &AStreamJid);
  bool prepareMessage(const Jid &AStreamJid, Message &AMessage, bool ADirectionIn);
  bool processMessage(const Jid &AStreamJid, Message &AMessage, bool ADirectionIn);
  void openHistoryOptionsNode(const Jid &AStreamJid);
  void closeHistoryOptionsNode(const Jid &AStreamJid);
  Menu *createContextMenu(const Jid &AStreamJid, const Jid &AContactJid, QWidget *AParent) const;
  void registerDiscoFeatures();
  void notifyInChatWindow(const Jid &AStreamJid, const Jid &AContactJid, const QString &AMessage) const;
  bool hasStanzaSession(const Jid &AStreamJid, const Jid &AContactJid) const;
  bool isOTRStanzaSession(const IStanzaSession &ASession) const;
  bool isOTRStanzaSession(const Jid &AStreamJid, const Jid &AContactJid) const;
  void saveStanzaSessionContext(const Jid &AStreamJid, const Jid &AContactJid) const;
  void restoreStanzaSessionContext(const Jid &AStreamJid, const QString &ASessionId = QString::null);
  void removeStanzaSessionContext(const Jid &AStreamJid, const QString &ASessionId) const;
  void startSuspendedStanzaSession(const Jid &AStreamJid, const QString &ARequestId);
  void cancelSuspendedStanzaSession(const Jid &AStreamJid, const QString &ARequestId, const QString &AError);
  void renegotiateStanzaSessions(const Jid &AStreamJid) const;
protected slots:
  void onStreamOpened(IXmppStream *AXmppStream);
  void onStreamClosed(IXmppStream *AXmppStream);
  void onAccountHidden(IAccount *AAccount);
  void onStreamJidChanged(IXmppStream *AXmppStream, const Jid &ABefour);
  void onPrivateDataChanged(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement);
  void onPrivateDataError(const QString &AId, const QString &AError);
  void onCollectionWriterDestroyed(const Jid &AStreamJid,  CollectionWriter *AWriter);
  void onRosterIndexContextMenu(IRosterIndex *AIndex, Menu *AMenu);
  void onMultiUserContextMenu(IMultiUserChatWindow *AWindow, IMultiUser *AUser, Menu *AMenu);
  void onMultiChatWindowMenuAboutToShow();
  void onSetMethodAction(bool);
  void onSetItemPrefsAction(bool);
  void onShowArchiveWindowAction(bool);
  void onOpenHistoryOptionsAction(bool);
  void onRemoveItemPrefsAction(bool);
  void onOptionsDialogAccepted();
  void onOptionsDialogRejected();
  void onArchiveHandlerDestroyed(QObject *AHandler);
  void onArchiveWindowDestroyed(IArchiveWindow *AWindow);
  void onDiscoInfoReceived(const IDiscoInfo &ADiscoInfo);
  void onStanzaSessionActivated(const IStanzaSession &ASession);
  void onStanzaSessionTerminated(const IStanzaSession &ASession);
  void onChatWindowCreated(IChatWindow *AWindow);
  void onMultiChatWindowCreated(IMultiUserChatWindow *AWindow);
private:
  IPluginManager *FPluginManager;
  IXmppStreams *FXmppStreams;
  IStanzaProcessor *FStanzaProcessor;    
  ISettingsPlugin *FSettingsPlugin;
  IPrivateStorage *FPrivateStorage;
  IAccountManager *FAccountManager;
  IRostersViewPlugin *FRostersViewPlugin;
  IServiceDiscovery *FDiscovery;
  IDataForms *FDataForms;
  IMessageWidgets *FMessageWidgets;
  ISessionNegotiation *FSessionNegotiation;
  IRosterPlugin *FRosterPlugin;
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
  QMap<QString,Jid> FPrefsRemoveRequests;
private:
  QMap<QString,IArchiveHeader> FSaveRequests;
  QMap<QString,IArchiveRequest> FListRequests;
  QMap<QString,IArchiveHeader> FRetrieveRequests;
  QMap<QString,IArchiveRequest> FRemoveRequests;
  QMap<QString,DateTime> FModifyRequests;
  QMap<QString,QString> FRestoreRequests;
private:
  QList<Jid> FInStoragePrefs;
  QMap<Jid,QString> FNamespaces;
  QMap<Jid,IArchiveStreamPrefs> FArchivePrefs;
  QMap<Jid,Replicator *> FReplicators;
  QMap<Jid,ViewHistoryWindow *> FArchiveWindows;
  QMap<Jid,QString> FGatewayTypes;
  QMultiMap<int, IArchiveHandler *> FArchiveHandlers;
  QMap<Jid,QMap<Jid,StanzaSession> > FSessions;
  QMap<Jid,QMultiMap<Jid,CollectionWriter *> > FCollectionWriters;
};

#endif // MESSAGEARCHIVER_H
