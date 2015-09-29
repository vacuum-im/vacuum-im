#ifndef MULTIUSERCHATMANAGER_H
#define MULTIUSERCHATMANAGER_H

#include <QMessageBox>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imultiuserchat.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/imessageprocessor.h>
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
#include "multiuserchat.h"
#include "multiuserchatwindow.h"
#include "createmultichatwizard.h"

struct InviteFields {
	Jid streamJid;
	Jid roomJid;
	Jid fromJid;
	QString password;
};

class MultiUserChatManager :
	public QObject,
	public IPlugin,
	public IMultiUserChatManager,
	public IXmppUriHandler,
	public IDiscoFeatureHandler,
	public IMessageHandler,
	public IDataLocalizer,
	public IOptionsDialogHolder,
	public IRostersClickHooker,
	public IRecentItemHandler,
	public IStanzaRequestOwner
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IMultiUserChatManager IXmppUriHandler IDiscoFeatureHandler IMessageHandler IDataLocalizer IOptionsDialogHolder IRostersClickHooker IRecentItemHandler IStanzaRequestOwner);
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
	//IMessageHandler
	virtual bool messageCheck(int AOrder, const Message &AMessage, int ADirection);
	virtual bool messageDisplay(const Message &AMessage, int ADirection);
	virtual INotification messageNotify(INotifications *ANotifications, const Message &AMessage, int ADirection);
	virtual bool messageShowWindow(int AMessageId);
	virtual bool messageShowWindow(int AOrder, const Jid &AStreamJid, const Jid &AContactJid, Message::MessageType AType, int AShowMode);
	//IRecentItemHandler
	virtual bool recentItemValid(const IRecentItem &AItem) const;
	virtual bool recentItemCanShow(const IRecentItem &AItem) const;
	virtual QIcon recentItemIcon(const IRecentItem &AItem) const;
	virtual QString recentItemName(const IRecentItem &AItem) const;
	virtual IRecentItem recentItemForIndex(const IRosterIndex *AIndex) const;
	virtual QList<IRosterIndex *> recentItemProxyIndexes(const IRecentItem &AItem) const;
	//IMultiUserChatManager
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
	bool isReady(const Jid &AStreamJid) const;
	void updateRecentChatItem(IRosterIndex *AIndex);
	void updateRecentChatItemProperties(IRosterIndex *AIndex);
	void updateChatRosterIndex(IMultiUserChatWindow *AWindow);
	void updateRecentUserItem(IMultiUserChat *AChat, const QString &ANick);
	bool isSelectionAccepted(const QList<IRosterIndex *> &ASelected) const;
	Action *createWizardAction(QWidget *AParent) const;
	Menu *createInviteMenu(const Jid &AContactJid, QWidget *AParent) const;
	Action *createJoinAction(const Jid &AStreamJid, const Jid &ARoomJid, QWidget *AParent) const;
	IRosterIndex *getConferencesGroupIndex(const Jid &AStreamJid) const;
	IMultiUserChatWindow *findMultiChatWindowForIndex(const IRosterIndex *AIndex) const;
	IMultiUserChatWindow *getMultiChatWindowForIndex(const IRosterIndex *AIndex);
	IMultiUser *findMultiChatWindowUser(const Jid &AStreamJid, const Jid &AContactJid) const;
	QString getMultiChatName(const Jid &AStreamJid, const Jid &ARoomJid) const;
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
	void onMultiChatWindowDestroyed();
	void onMultiChatTitleChanged(const QString &ATitle);
	void onMultiChatPresenceChanged(const IPresenceItem &APresence);
	void onMultiChatContextMenu(Menu *AMenu);
	void onMultiUserContextMenu(IMultiUser *AUser, Menu *AMenu);
	void onMultiUserToolTips(IMultiUser *AUser, QMap<int,QString> &AToolTips);
	void onMultiUserPrivateChatWindowChanged(IMessageChatWindow *AWindow);
	void onMultiUserChanged(IMultiUser *AUser, int AData, const QVariant &ABefore);
protected slots:
	void onMultiChatWindowInfoContextMenu(Menu *AMenu);
	void onMultiChatWindowInfoToolTips(QMap<int,QString> &AToolTips);
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
	void onInviteDialogFinished(int AResult);
	void onInviteActionTriggered(bool);
private:
	IMessageWidgets *FMessageWidgets;
	IMessageProcessor *FMessageProcessor;
	IRostersViewPlugin *FRostersViewPlugin;
	IRostersModel *FRostersModel;
	IXmppStreamManager *FXmppStreamManager;
	IServiceDiscovery *FDiscovery;
	INotifications *FNotifications;
	IDataForms *FDataForms;
	IXmppUriQueries *FXmppUriQueries;
	IOptionsManager *FOptionsManager;
	IStatusIcons *FStatusIcons;
	IRecentContacts *FRecentContacts;
	IStanzaProcessor *FStanzaProcessor;
private:
	QList<IMultiUserChat *> FChats;
	QMap<int, Message> FActiveInvites;
	QList<IRosterIndex *> FChatIndexes;
	QList<QString> FDiscoNickRequests;
	QMap<QString,QString> FRegisterNickRequests;
	QList<IMultiUserChatWindow *> FChatWindows;
	QMap<QMessageBox *,InviteFields> FInviteDialogs;
};

#endif // MULTIUSERCHATMANAGER_H
