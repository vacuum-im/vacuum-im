#ifndef NORMALMESSAGEHANDLER_H
#define NORMALMESSAGEHANDLER_H

#define NORMALMESSAGEHANDLER_UUID "{8592e3c3-ef5e-42a9-91c9-faf1ed9a91cc}"

#include <QQueue>
#include <QMultiMap>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/soundfiles.h>
#include <definitions/shortcuts.h>
#include <definitions/optionvalues.h>
#include <definitions/optionnodes.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/actiongroups.h>
#include <definitions/toolbargroups.h>
#include <definitions/messagedataroles.h>
#include <definitions/messagehandlerorders.h>
#include <definitions/xmppurihandlerorders.h>
#include <definitions/rosterindexkinds.h>
#include <definitions/rosterindexroles.h>
#include <definitions/rosternotifyorders.h>
#include <definitions/rosterclickhookerorders.h>
#include <definitions/recentitemtypes.h>
#include <definitions/notificationtypes.h>
#include <definitions/notificationdataroles.h>
#include <definitions/notificationtypeorders.h>
#include <definitions/tabpagenotifypriorities.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imessageprocessor.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/inotifications.h>
#include <interfaces/istatusicons.h>
#include <interfaces/irostersview.h>
#include <interfaces/ipresence.h>
#include <interfaces/iroster.h>
#include <interfaces/iavatars.h>
#include <interfaces/ixmppuriqueries.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/irecentcontacts.h>
#include <utils/widgetmanager.h>
#include <utils/xmpperror.h>
#include <utils/textmanager.h>
#include <utils/shortcuts.h>
#include <utils/options.h>

enum WindowMenuAction {
	NextAction,
	SendAction,
	ReplyAction,
	ForwardAction,
	OpenChatAction,
	SendChatAction
};

class NormalMessageHandler :
	public QObject,
	public IPlugin,
	public IMessageHandler,
	public IXmppUriHandler,
	public IRostersClickHooker,
	public IOptionsHolder
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IMessageHandler IXmppUriHandler IRostersClickHooker IOptionsHolder);
public:
	NormalMessageHandler();
	~NormalMessageHandler();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return NORMALMESSAGEHANDLER_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin() { return true; }
	//IXmppUriHandler
	virtual bool xmppUriOpen(const Jid &AStreamJid, const Jid &AContactJid, const QString &AAction, const QMultiMap<QString, QString> &AParams);
	//IRostersClickHooker
	virtual bool rosterIndexSingleClicked(int AOrder, IRosterIndex *AIndex, const QMouseEvent *AEvent);
	virtual bool rosterIndexDoubleClicked(int AOrder, IRosterIndex *AIndex, const QMouseEvent *AEvent);
	//IMessageHandler
	virtual bool messageCheck(int AOrder, const Message &AMessage, int ADirection);
	virtual bool messageDisplay(const Message &AMessage, int ADirection);
	virtual INotification messageNotify(INotifications *ANotifications, const Message &AMessage, int ADirection);
	virtual bool messageShowWindow(int AMessageId);
	virtual bool messageShowWindow(int AOrder, const Jid &AStreamJid, const Jid &AContactJid, Message::MessageType AType, int AShowMode);
	//IOptionsHolder
	virtual QMultiMap<int, IOptionsWidget *> optionsWidgets(const QString &ANodeId, QWidget *AParent);
protected:
	IMessageNormalWindow *getWindow(const Jid &AStreamJid, const Jid &AContactJid, IMessageNormalWindow::Mode AMode);
	IMessageNormalWindow *findWindow(const Jid &AStreamJid, const Jid &AContactJid) const;
	Menu *createWindowMenu(IMessageNormalWindow *AWindow) const;
	Action *findWindowMenuAction(IMessageNormalWindow *AWindow, WindowMenuAction AActionId) const;
	void setDefaultWindowMenuAction(IMessageNormalWindow *AWindow, WindowMenuAction AActionId) const;
	void setWindowMenuActionVisible(IMessageNormalWindow *AWindow, WindowMenuAction AActionId, bool AVisible) const;
	void setWindowMenuActionEnabled(IMessageNormalWindow *AWindow, WindowMenuAction AActionId, bool AEnabled) const;
	void updateWindowMenu(IMessageNormalWindow *AWindow) const;
	void updateWindow(IMessageNormalWindow *AWindow) const;
protected:
	bool sendMessage(IMessageNormalWindow *AWindow);
	bool showNextMessage(IMessageNormalWindow *AWindow);
	void removeCurrentMessageNotify(IMessageNormalWindow *AWindow);
	void removeNotifiedMessages(IMessageNormalWindow *AWindow, int AMessageId = -1);
protected:
	void setMessageStyle(IMessageNormalWindow *AWindow);
	void fillContentOptions(IMessageNormalWindow *AWindow, IMessageContentOptions &AOptions) const;
	void showStyledMessage(IMessageNormalWindow *AWindow, const Message &AMessage);
protected:
	bool isAnyPresenceOpened(const QStringList &AStreams) const;
	bool isSelectionAccepted(const QList<IRosterIndex *> &ASelected) const;
	QMap<int,QStringList> indexesRolesMap(const QList<IRosterIndex *> &AIndexes) const;
protected slots:
	void onWindowMessageReady();
	void onWindowActivated();
	void onWindowDestroyed();
	void onWindowAddressChanged();
	void onWindowAvailAddressesChanged();
	void onWindowSelectedReceiversChanged();
	void onWindowContextMenuRequested(Menu *AMenu);
	void onWindowToolTipsRequested(QMap<int,QString> &AToolTips);
	void onWindowNotifierActiveNotifyChanged(int ANotifyId);
protected slots:
	void onWindowMenuSendMessage();
	void onWindowMenuShowNextMessage();
	void onWindowMenuReplyMessage();
	void onWindowMenuForwardMessage();
	void onWindowMenuShowChatDialog();
	void onWindowMenuSendAsChatMessage();
protected slots:
	void onStatusIconsChanged();
	void onAvatarChanged(const Jid &AContactJid);
	void onPresenceItemReceived(IPresence *APresence, const IPresenceItem &AItem, const IPresenceItem &ABefore);
protected slots:
	void onShowWindowAction(bool);
	void onActiveStreamRemoved(const Jid &AStreamJid);
	void onShortcutActivated(const QString &AId, QWidget *AWidget);
	void onRostersViewIndexMultiSelection(const QList<IRosterIndex *> &ASelected, bool &AAccepted);
	void onRostersViewIndexContextMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu);
	void onStyleOptionsChanged(const IMessageStyleOptions &AOptions, int AMessageType, const QString &AContext);
private:
	IAvatars *FAvatars;
	IMessageWidgets *FMessageWidgets;
	IMessageProcessor *FMessageProcessor;
	IMessageStyles *FMessageStyles;
	IStatusIcons *FStatusIcons;
	INotifications *FNotifications;
	IPresencePlugin *FPresencePlugin;
	IRostersView *FRostersView;
	IRostersModel *FRostersModel;
	IXmppUriQueries *FXmppUriQueries;
	IOptionsManager *FOptionsManager;
	IRecentContacts *FRecentContacts;
private:
	QList<IMessageNormalWindow *> FWindows;
	QMultiMap<IMessageNormalWindow *, int> FNotifiedMessages;
	QMap<IMessageNormalWindow *, QQueue<Message> > FMessageQueue;
};

#endif // NORMALMESSAGEHANDLER_H
