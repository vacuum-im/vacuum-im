#ifndef CHATMESSAGEHANDLER_H
#define CHATMESSAGEHANDLER_H

#define CHATMESSAGEHANDLER_UUID "{b60cc0e4-8006-4909-b926-fcb3cbc506f0}"

#include <QTimer>
#include <definitions/messagehandlerorders.h>
#include <definitions/rosterindexkinds.h>
#include <definitions/rosterindexroles.h>
#include <definitions/rosterclickhookerorders.h>
#include <definitions/rosternotifyorders.h>
#include <definitions/recentitemtypes.h>
#include <definitions/notificationtypes.h>
#include <definitions/notificationdataroles.h>
#include <definitions/notificationtypeorders.h>
#include <definitions/tabpagenotifypriorities.h>
#include <definitions/messagedataroles.h>
#include <definitions/vcardvaluenames.h>
#include <definitions/actiongroups.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/soundfiles.h>
#include <definitions/shortcuts.h>
#include <definitions/optionnodes.h>
#include <definitions/optionvalues.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/toolbargroups.h>
#include <definitions/xmppurihandlerorders.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imessageprocessor.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/imessagestyles.h>
#include <interfaces/imessagearchiver.h>
#include <interfaces/inotifications.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/istatusicons.h>
#include <interfaces/irostersview.h>
#include <interfaces/irostersmodel.h>
#include <interfaces/iroster.h>
#include <interfaces/ipresence.h>
#include <interfaces/ivcard.h>
#include <interfaces/iavatars.h>
#include <interfaces/iaccountmanager.h>
#include <interfaces/istatuschanger.h>
#include <interfaces/ixmppuriqueries.h>
#include <interfaces/irecentcontacts.h>
#include <utils/options.h>
#include <utils/shortcuts.h>
#include <utils/textmanager.h>
#include <utils/widgetmanager.h>

struct WindowStatus {
	QDateTime startTime;
	QDateTime createTime;
	QDate lastDateSeparator;
};

class ChatMessageHandler :
	public QObject,
	public IPlugin,
	public IMessageHandler,
	public IXmppUriHandler,
	public IRostersClickHooker,
	public IOptionsHolder
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IMessageHandler IRostersClickHooker IXmppUriHandler IOptionsHolder);
public:
	ChatMessageHandler();
	~ChatMessageHandler();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return CHATMESSAGEHANDLER_UUID; }
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
	IMessageChatWindow *getWindow(const Jid &AStreamJid, const Jid &AContactJid);
	IMessageChatWindow *findWindow(const Jid &AStreamJid, const Jid &AContactJid) const;
	void updateWindow(IMessageChatWindow *AWindow);
	void removeNotifiedMessages(IMessageChatWindow *AWindow);
	void showHistory(IMessageChatWindow *AWindow);
	void setMessageStyle(IMessageChatWindow *AWindow);
	void fillContentOptions(IMessageChatWindow *AWindow, IMessageContentOptions &AOptions) const;
	void showDateSeparator(IMessageChatWindow *AWindow, const QDateTime &ADateTime);
	void showStyledStatus(IMessageChatWindow *AWindow, const QString &AMessage, bool ADontSave=false, const QDateTime &ATime=QDateTime::currentDateTime());
	void showStyledMessage(IMessageChatWindow *AWindow, const Message &AMessage);
	bool isSelectionAccepted(const QList<IRosterIndex *> &ASelected) const;
	QMap<Jid, QList<Jid> > getSortedAddresses(const QMultiMap<Jid,Jid> &AAddresses) const;
protected slots:
	void onWindowMessageReady();
	void onWindowActivated();
	void onWindowClosed();
	void onWindowDestroyed();
	void onWindowAddressChanged();
	void onWindowAvailAddressesChanged();
	void onWindowAddressMenuRequested(Menu *AMenu);
	void onWindowContextMenuRequested(Menu *AMenu);
	void onWindowToolTipsRequested(QMap<int,QString> &AToolTips);
	void onWindowNotifierActiveNotifyChanged(int ANotifyId);
protected slots:
	void onStatusIconsChanged();
	void onAvatarChanged(const Jid &AContactJid);
	void onRosterItemReceived(IRoster *ARoster, const IRosterItem &AItem, const IRosterItem &ABefore);
	void onPresenceItemReceived(IPresence *APresence, const IPresenceItem &AItem, const IPresenceItem &ABefore);
protected slots:
	void onShowWindowAction(bool);
	void onClearWindowAction(bool);
	void onChangeWindowAddressAction();
	void onActiveStreamRemoved(const Jid &AStreamJid);
	void onShortcutActivated(const QString &AId, QWidget *AWidget);
	void onArchiveMessagesLoaded(const QString &AId, const IArchiveCollectionBody &ABody);
	void onArchiveRequestFailed(const QString &AId, const XmppError &AError);
	void onRostersViewIndexContextMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu);
	void onStyleOptionsChanged(const IMessageStyleOptions &AOptions, int AMessageType, const QString &AContext);
private:
	IAvatars *FAvatars;
	IMessageWidgets *FMessageWidgets;
	IMessageProcessor *FMessageProcessor;
	IMessageStyles *FMessageStyles;
	IRosterPlugin *FRosterPlugin;
	IPresencePlugin *FPresencePlugin;
	IMessageArchiver *FMessageArchiver;
	IRostersView *FRostersView;
	IRostersModel *FRostersModel;
	IStatusIcons *FStatusIcons;
	IStatusChanger *FStatusChanger;
	INotifications *FNotifications;
	IAccountManager *FAccountManager;
	IXmppUriQueries *FXmppUriQueries;
	IOptionsManager *FOptionsManager;
	IRecentContacts *FRecentContacts;
private:
	QList<IMessageChatWindow *> FWindows;
	QMap<IMessageChatWindow *, QTimer *> FDestroyTimers;
	QMultiMap<IMessageChatWindow *, int> FNotifiedMessages;
	QMap<IMessageChatWindow *, WindowStatus> FWindowStatus;
private:
	QMap<QString, IMessageChatWindow *> FHistoryRequests;
	QMap<IMessageChatWindow *, QList<Message> > FPendingMessages;
};

#endif // CHATMESSAGEHANDLER_H
