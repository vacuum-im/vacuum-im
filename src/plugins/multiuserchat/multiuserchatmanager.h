#ifndef MULTIUSERCHATMANAGER_H
#define MULTIUSERCHATMANAGER_H

#include <QMessageBox>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imultiuserchat.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/imessageprocessor.h>
#include <interfaces/imessagearchiver.h>
#include <interfaces/ixmppstreammanager.h>
#include <interfaces/ipresencemanager.h>
#include <interfaces/iservicediscovery.h>
#include <interfaces/inotifications.h>
#include <interfaces/idataforms.h>
#include <interfaces/ivcardmanager.h>
#include <interfaces/iregistraton.h>
#include <interfaces/ixmppuriqueries.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/irostersmodel.h>
#include <interfaces/irostersview.h>
#include <interfaces/istatusicons.h>
#include <interfaces/irecentcontacts.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/imainwindow.h>
#include <utils/pluginhelper.h>
#include "multiuserchat.h"
#include "multiuserchatwindow.h"
#include "createmultichatwizard.h"
#include "inviteusersmenu.h"

struct ChatInvite {
	QString id;
	Jid streamJid;
	Jid roomJid;
	Jid fromJid;
	QString reason;
	QString thread;
	bool isContinue;
	QString password;
};

struct ChatConvert {
	Jid streamJid;
	Jid contactJid;
	Jid roomJid;
	QString reason;
	QString threadId;
	QList<Jid> members;
};

class MultiUserChatManager :
	public QObject,
	public IPlugin,
	public IMultiUserChatManager,
	public IXmppUriHandler,
	public IDiscoFeatureHandler,
	public IDataLocalizer,
	public IOptionsDialogHolder,
	public IRostersClickHooker,
	public IRecentItemHandler,
	public IStanzaHandler,
	public IStanzaRequestOwner
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IMultiUserChatManager IXmppUriHandler IDiscoFeatureHandler IDataLocalizer IOptionsDialogHolder IRostersClickHooker IRecentItemHandler IStanzaHandler IStanzaRequestOwner);
	Q_PLUGIN_METADATA(IID "org.vacuum-im.plugins.MultiUserChat");
public:
	MultiUserChatManager();
	~MultiUserChatManager();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return MULTIUSERCHAT_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin() { return true; }
	//IOptionsHolder
	virtual QMultiMap<int, IOptionsDialogWidget *> optionsDialogWidgets(const QString &ANodeId, QWidget *AParent);
	//IStanzaHandler
	virtual bool stanzaReadWrite(int AHandleId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept);
	//IStanzaRequestOwner
	virtual void stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza);
	//IRostersClickHooker
	virtual bool rosterIndexSingleClicked(int AOrder, IRosterIndex *AIndex, const QMouseEvent *AEvent);
	virtual bool rosterIndexDoubleClicked(int AOrder, IRosterIndex *AIndex, const QMouseEvent *AEvent);
	//IXmppUriHandler
	virtual bool xmppUriOpen(const Jid &AStreamJid, const Jid &AContactJid, const QString &AAction, const QMultiMap<QString, QString> &AParams);
	//IDiscoFeatureHandler
	virtual bool execDiscoFeature(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo);
	virtual Action *createDiscoFeatureAction(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo, QWidget *AParent);
	//IDataLocalizer
	virtual IDataFormLocale dataFormLocale(const QString &AFormType);
	//IRecentItemHandler
	virtual bool recentItemValid(const IRecentItem &AItem) const;
	virtual bool recentItemCanShow(const IRecentItem &AItem) const;
	virtual QIcon recentItemIcon(const IRecentItem &AItem) const;
	virtual QString recentItemName(const IRecentItem &AItem) const;
	virtual IRecentItem recentItemForIndex(const IRosterIndex *AIndex) const;
	virtual QList<IRosterIndex *> recentItemProxyIndexes(const IRecentItem &AItem) const;
	//IMultiUserChatManager
	virtual bool isReady(const Jid &AStreamJid) const;
	virtual QList<IMultiUserChat *> multiUserChats() const;
	virtual IMultiUserChat *findMultiUserChat(const Jid &AStreamJid, const Jid &ARoomJid) const;
	virtual IMultiUserChat *getMultiUserChat(const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANick,const QString &APassword, bool AIsolated);
	virtual QList<IMultiUserChatWindow *> multiChatWindows() const;
	virtual IMultiUserChatWindow *findMultiChatWindow(const Jid &AStreamJid, const Jid &ARoomJid) const;
	virtual IMultiUserChatWindow *getMultiChatWindow(const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANick, const QString &APassword);
	virtual QList<IRosterIndex *> multiChatRosterIndexes() const;
	virtual IRosterIndex *findMultiChatRosterIndex(const Jid &AStreamJid, const Jid &ARoomJid) const;
	virtual IRosterIndex *getMultiChatRosterIndex(const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANick, const QString &APassword);
	virtual QDialog *showMultiChatWizard(QWidget *AParent = NULL);
	virtual QDialog *showJoinMultiChatWizard(const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANick, const QString &APassword, QWidget *AParent = NULL);
	virtual QDialog *showCreateMultiChatWizard(const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANick, const QString &APassword, QWidget *AParent = NULL);
	virtual QDialog *showManualMultiChatWizard(const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANick, const QString &APassword, QWidget *AParent = NULL);
	virtual QString requestRegisteredNick(const Jid &AStreamJid, const Jid &ARoomJid);
signals:
	void multiUserChatCreated(IMultiUserChat *AMultiChat);
	void multiUserChatDestroyed(IMultiUserChat *AMultiChat);
	void multiChatWindowCreated(IMultiUserChatWindow *AWindow);
	void multiChatWindowDestroyed(IMultiUserChatWindow *AWindow);
	void multiChatRosterIndexCreated(IRosterIndex *AIndex);
	void multiChatRosterIndexDestroyed(IRosterIndex *AIndex);
	void multiChatContextMenu(IMultiUserChatWindow *AWindow, Menu *AMenu);
	void multiUserContextMenu(IMultiUserChatWindow *AWindow, IMultiUser *AUser, Menu *AMenu);
	void multiUserToolTips(IMultiUserChatWindow *AWindow, IMultiUser *AUser, QMap<int,QString> &AToolTips);
	void registeredNickReceived(const QString &AId, const QString &ANick);
	//IRecentItemHandler
	void recentItemUpdated(const IRecentItem &AItem);
protected:
	void registerDiscoFeatures();
	bool isSelectionAccepted(const QList<IRosterIndex *> &ASelected) const;
	IMultiUser *findMultiChatWindowUser(const Jid &AStreamJid, const Jid &AContactJid) const;
protected:
	IRosterIndex *getConferencesGroupIndex(const Jid &AStreamJid);
	void updateMultiChatRosterIndex(const Jid &AStreamJid, const Jid &ARoomJid);
	IMultiUserChatWindow *findMultiChatWindowForIndex(const IRosterIndex *AIndex) const;
	IMultiUserChatWindow *getMultiChatWindowForIndex(const IRosterIndex *AIndex);
protected:
	void updateMultiChatRecentItem(IRosterIndex *AChatIndex);
	void updateMultiChatRecentItemProperties(IMultiUserChat *AChat);
	void updateMultiUserRecentItems(IMultiUserChat *AChat, const QString &ANick = QString::null);
	QString multiChatRecentName(const Jid &AStreamJid, const Jid &ARoomJid) const;
	IRecentItem multiChatRecentItem(IMultiUserChat *AChat, const QString &ANick = QString::null) const;
protected:
	Action *createWizardAction(QWidget *AParent) const;
	Action *createJoinAction(const Jid &AStreamJid, const Jid &ARoomJid, QWidget *AParent) const;
	Menu *createInviteMenu(const QStringList &AStreams, const QStringList &AContacts, QWidget *AParent) const;
protected slots:
	void onWizardRoomActionTriggered(bool);
	void onJoinRoomActionTriggered(bool);
	void onOpenRoomActionTriggered(bool);
	void onEnterRoomActionTriggered(bool);
	void onExitRoomActionTriggered(bool);
	void onCopyToClipboardActionTriggered(bool);
protected slots:
	void onStatusIconsChanged();
	void onActiveXmppStreamRemoved(const Jid &AStreamJid);
	void onShortcutActivated(const QString &AId, QWidget *AWidget);
protected slots:
	void onMultiChatDestroyed();
	void onMultiChatPropertiesChanged();
	void onMultiChatUserChanged(IMultiUser *AUser, int AData, const QVariant &ABefore);
protected slots:
	void onMultiChatWindowDestroyed();
	void onMultiChatWindowContextMenu(Menu *AMenu);
	void onMultiChatWindowUserContextMenu(IMultiUser *AUser, Menu *AMenu);
	void onMultiChatWindowUserToolTips(IMultiUser *AUser, QMap<int,QString> &AToolTips);
	void onMultiChatWindowPrivateWindowChanged(IMessageChatWindow *AWindow);
	void onMultiChatWindowInfoContextMenu(Menu *AMenu);
	void onMultiChatWindowInfoToolTips(QMap<int,QString> &AToolTips);
protected slots:
	void onXmppStreamOpened(IXmppStream *AXmppStream);
	void onXmppStreamClosed(IXmppStream *AXmppStream);
protected slots:
	void onRostersModelStreamsLayoutChanged(int ABefore);
	void onRostersModelIndexDestroyed(IRosterIndex *AIndex);
	void onRostersModelIndexDataChanged(IRosterIndex *AIndex, int ARole);
protected slots:
	void onRostersViewIndexMultiSelection(const QList<IRosterIndex *> &ASelected, bool &AAccepted);
	void onRostersViewIndexContextMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu);
	void onRostersViewIndexToolTips(IRosterIndex *AIndex, quint32 ALabelId, QMap<int,QString> &AToolTips);
	void onRostersViewIndexClipboardMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu);
protected slots:
	void onInviteActionTriggered(bool);
	void onInviteDialogFinished(int AResult);
	void onNotificationActivated(int ANotifyId);
	void onNotificationRemoved(int ANotifyId);
protected slots:
	void onMessageChatWindowCreated(IMessageChatWindow *AWindow);
	void onConvertMessageChatWindowStart(const QMultiMap<Jid, Jid> &AAddresses);
	void onConvertMessageChatWindowWizardAccetped(IMultiUserChatWindow *AWindow);
	void onConvertMessageChatWindowWizardRejected();
	void onConvertMessageChatWindowFinish(const ChatConvert &AConvert);
protected slots:
	void onMessageArchiverRequestFailed(const QString &AId, const XmppError &AError);
	void onMessageArchiverHeadersLoaded(const QString &AId, const QList<IArchiveHeader> &AHeaders);
	void onMessageArchiverCollectionLoaded(const QString &AId, const IArchiveCollection &ACollection);
private:
	PluginPointer<IDataForms> FDataForms;
	PluginPointer<IStatusIcons> FStatusIcons;
	PluginPointer<IRostersModel> FRostersModel;
	PluginPointer<IServiceDiscovery> FDiscovery;
	PluginPointer<INotifications> FNotifications;
	PluginPointer<IXmppUriQueries> FXmppUriQueries;
	PluginPointer<IOptionsManager> FOptionsManager;
	PluginPointer<IRecentContacts> FRecentContacts;
	PluginPointer<IMessageWidgets> FMessageWidgets;
	PluginPointer<IMessageArchiver> FMessageArchiver;
	PluginPointer<IStanzaProcessor> FStanzaProcessor;
	PluginPointer<IMainWindowPlugin> FMainWindowPlugin;
	PluginPointer<IMessageProcessor> FMessageProcessor;
	PluginPointer<IRostersViewPlugin> FRostersViewPlugin;
	PluginPointer<IXmppStreamManager> FXmppStreamManager;
private:
	QList<IMultiUserChat *> FChats;
	QList<IRosterIndex *> FChatIndexes;
	QList<IMultiUserChatWindow *> FChatWindows;
private:
	QList<QString> FDiscoNickRequests;
	QMap<QString, QString> FRegisterNickRequests;
private:
	QMap<Jid, int> FSHIInvite;
	QMap<int, ChatInvite> FInviteNotify;
	QMap<QMessageBox *, ChatInvite> FInviteDialogs;
private:
	QMap<QString, ChatConvert> FHistoryConvert;
	QMap<CreateMultiChatWizard *, ChatConvert> FWizardConvert;
};

#endif // MULTIUSERCHATMANAGER_H
