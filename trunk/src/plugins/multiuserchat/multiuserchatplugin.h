#ifndef MULTIUSERCHATPLUGIN_H
#define MULTIUSERCHATPLUGIN_H

#include <QMessageBox>
#include <definitions/actiongroups.h>
#include <definitions/toolbargroups.h>
#include <definitions/dataformtypes.h>
#include <definitions/recentitemtypes.h>
#include <definitions/recentitemproperties.h>
#include <definitions/rosterindexkinds.h>
#include <definitions/rosterindexroles.h>
#include <definitions/rosternotifyorders.h>
#include <definitions/rosterindexkindorders.h>
#include <definitions/rosterclickhookerorders.h>
#include <definitions/discofeaturehandlerorders.h>
#include <definitions/messagehandlerorders.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/soundfiles.h>
#include <definitions/shortcuts.h>
#include <definitions/shortcutgrouporders.h>
#include <definitions/vcardvaluenames.h>
#include <definitions/optionvalues.h>
#include <definitions/optionnodes.h>
#include <definitions/optionnodeorders.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/xmppurihandlerorders.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imultiuserchat.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/imessageprocessor.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/ipresence.h>
#include <interfaces/iservicediscovery.h>
#include <interfaces/inotifications.h>
#include <interfaces/idataforms.h>
#include <interfaces/ivcard.h>
#include <interfaces/iregistraton.h>
#include <interfaces/ixmppuriqueries.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/irostersmodel.h>
#include <interfaces/irostersview.h>
#include <interfaces/istatusicons.h>
#include <interfaces/irecentcontacts.h>
#include <utils/widgetmanager.h>
#include <utils/shortcuts.h>
#include <utils/message.h>
#include <utils/action.h>
#include <utils/options.h>
#include "multiuserchat.h"
#include "multiuserchatwindow.h"
#include "joinmultichatdialog.h"

struct InviteFields {
	Jid streamJid;
	Jid roomJid;
	Jid fromJid;
	QString password;
};

class MultiUserChatPlugin :
	public QObject,
	public IPlugin,
	public IMultiUserChatPlugin,
	public IXmppUriHandler,
	public IDiscoFeatureHandler,
	public IMessageHandler,
	public IDataLocalizer,
	public IOptionsHolder,
	public IRostersClickHooker,
	public IRecentItemHandler
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IMultiUserChatPlugin IXmppUriHandler IDiscoFeatureHandler IMessageHandler IDataLocalizer IOptionsHolder IRostersClickHooker IRecentItemHandler);
public:
	MultiUserChatPlugin();
	~MultiUserChatPlugin();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return MULTIUSERCHAT_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin() { return true; }
	//IOptionsHolder
	virtual QMultiMap<int, IOptionsWidget *> optionsWidgets(const QString &ANodeId, QWidget *AParent);
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
	//IMultiUserChatPlugin
	virtual IPluginManager *pluginManager() const;
	virtual bool requestRoomNick(const Jid &AStreamJid, const Jid &ARoomJid);
	virtual QList<IMultiUserChat *> multiUserChats() const;
	virtual IMultiUserChat *findMultiUserChat(const Jid &AStreamJid, const Jid &ARoomJid) const;
	virtual IMultiUserChat *getMultiUserChat(const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANick,const QString &APassword);
	virtual QList<IMultiUserChatWindow *> multiChatWindows() const;
	virtual IMultiUserChatWindow *findMultiChatWindow(const Jid &AStreamJid, const Jid &ARoomJid) const;
	virtual IMultiUserChatWindow *getMultiChatWindow(const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANick, const QString &APassword);
	virtual QList<IRosterIndex *> multiChatRosterIndexes() const;
	virtual IRosterIndex *findMultiChatRosterIndex(const Jid &AStreamJid, const Jid &ARoomJid) const;
	virtual IRosterIndex *getMultiChatRosterIndex(const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANick, const QString &APassword);
	virtual void showJoinMultiChatDialog(const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANick, const QString &APassword);
signals:
	void roomNickReceived(const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANick);
	void multiUserChatCreated(IMultiUserChat *AMultiChat);
	void multiUserChatDestroyed(IMultiUserChat *AMultiChat);
	void multiChatWindowCreated(IMultiUserChatWindow *AWindow);
	void multiChatWindowDestroyed(IMultiUserChatWindow *AWindow);
	void multiChatRosterIndexCreated(IRosterIndex *AIndex);
	void multiChatRosterIndexDestroyed(IRosterIndex *AIndex);
	void multiChatContextMenu(IMultiUserChatWindow *AWindow, Menu *AMenu);
	void multiUserContextMenu(IMultiUserChatWindow *AWindow, IMultiUser *AUser, Menu *AMenu);
	void multiUserToolTips(IMultiUserChatWindow *AWindow, IMultiUser *AUser, QMap<int,QString> &AToolTips);
	//IRecentItemHandler
	void recentItemUpdated(const IRecentItem &AItem);
protected:
	void registerDiscoFeatures();
	QString streamVCardNick(const Jid &AStreamJid) const;
	void updateRecentItemProxy(IRosterIndex *AIndex);
	void updateRecentItemProperties(IRosterIndex *AIndex);
	void updateChatRosterIndex(IMultiUserChatWindow *AWindow);
	bool isSelectionAccepted(const QList<IRosterIndex *> &ASelected) const;
	Menu *createInviteMenu(const Jid &AContactJid, QWidget *AParent) const;
	Action *createJoinAction(const Jid &AStreamJid, const Jid &ARoomJid, QObject *AParent) const;
	IMultiUserChatWindow *getMultiChatWindowForIndex(const IRosterIndex *AIndex);
	QString getRoomName(const Jid &AStreamJid, const Jid &ARoomJid) const;
protected slots:
	void onMultiChatContextMenu(Menu *AMenu);
	void onMultiUserContextMenu(IMultiUser *AUser, Menu *AMenu);
	void onMultiUserToolTips(IMultiUser *AUser, QMap<int,QString> &AToolTips);
	void onMultiUserChatChanged();
	void onMultiUserChatDestroyed();
	void onMultiChatWindowDestroyed();
protected slots:
	void onMultiChatWindowInfoContextMenu(Menu *AMenu);
	void onMultiChatWindowInfoToolTips(QMap<int,QString> &AToolTips);
protected slots:
	void onStatusIconsChanged();
	void onJoinRoomActionTriggered(bool);
	void onOpenRoomActionTriggered(bool);
	void onEnterRoomActionTriggered(bool);
	void onExitRoomActionTriggered(bool);
	void onCopyToClipboardActionTriggered(bool);
	void onRosterIndexDestroyed(IRosterIndex *AIndex);
	void onActiveStreamRemoved(const Jid &AStreamJid);
	void onShortcutActivated(const QString &AId, QWidget *AWidget);
	void onRostersViewIndexMultiSelection(const QList<IRosterIndex *> &ASelected, bool &AAccepted);
	void onRostersViewIndexContextMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu);
	void onRostersViewClipboardMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu);
	void onDiscoInfoReceived(const IDiscoInfo &ADiscoInfo);
	void onRegisterFieldsReceived(const QString &AId, const IRegisterFields &AFields);
	void onRegisterErrorReceived(const QString &AId, const XmppError &AError);
	void onInviteDialogFinished(int AResult);
	void onInviteActionTriggered(bool);
private:
	IPluginManager *FPluginManager;
	IMessageWidgets *FMessageWidgets;
	IMessageProcessor *FMessageProcessor;
	IRostersViewPlugin *FRostersViewPlugin;
	IRostersModel *FRostersModel;
	IXmppStreams *FXmppStreams;
	IServiceDiscovery *FDiscovery;
	INotifications *FNotifications;
	IDataForms *FDataForms;
	IVCardPlugin *FVCardPlugin;
	IRegistration *FRegistration;
	IXmppUriQueries *FXmppUriQueries;
	IOptionsManager *FOptionsManager;
	IStatusIcons *FStatusIcons;
	IRecentContacts *FRecentContacts;
private:
	QList<IMultiUserChat *> FChats;
	QMap<int, Message> FActiveInvites;
	QList<IRosterIndex *> FChatIndexes;
	QList<IMultiUserChatWindow *> FChatWindows;
	QMap<QMessageBox *,InviteFields> FInviteDialogs;
	QMap<QString, QPair<Jid,Jid> > FNickRequests;
};

#endif // MULTIUSERCHATPLUGIN_H
