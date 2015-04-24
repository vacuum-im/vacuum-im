#ifndef CHATMESSAGEHANDLER_H
#define CHATMESSAGEHANDLER_H

#define CHATMESSAGEHANDLER_UUID "{b60cc0e4-8006-4909-b926-fcb3cbc506f0}"

#include <QTimer>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imessageprocessor.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/imessagestylemanager.h>
#include <interfaces/imessagearchiver.h>
#include <interfaces/inotifications.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/istatusicons.h>
#include <interfaces/irostersview.h>
#include <interfaces/irostersmodel.h>
#include <interfaces/irostermanager.h>
#include <interfaces/ipresencemanager.h>
#include <interfaces/ivcardmanager.h>
#include <interfaces/iavatars.h>
#include <interfaces/iaccountmanager.h>
#include <interfaces/istatuschanger.h>
#include <interfaces/ixmppuriqueries.h>
#include <interfaces/irecentcontacts.h>

struct WindowStatus {
	QDateTime startTime;
	QDateTime createTime;
	QDate lastDateSeparator;
};

struct WindowContent {
	QString html;
	IMessageStyleContentOptions options;
};

class ChatMessageHandler :
	public QObject,
	public IPlugin,
	public IXmppUriHandler,
	public IMessageHandler,
	public IRostersClickHooker,
	public IMessageEditSendHandler
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IXmppUriHandler IMessageHandler IRostersClickHooker IMessageEditSendHandler);
	Q_PLUGIN_METADATA(IID "org.vacuum-im.plugins.ChatMessageHandler");
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
	//IMessageEditSendHandler
	virtual bool messageEditSendPrepare(int AOrder, IMessageEditWidget *AWidget);
	virtual bool messageEditSendProcesse(int AOrder, IMessageEditWidget *AWidget);
	//IMessageHandler
	virtual bool messageCheck(int AOrder, const Message &AMessage, int ADirection);
	virtual bool messageDisplay(const Message &AMessage, int ADirection);
	virtual INotification messageNotify(INotifications *ANotifications, const Message &AMessage, int ADirection);
	virtual bool messageShowWindow(int AMessageId);
	virtual bool messageShowWindow(int AOrder, const Jid &AStreamJid, const Jid &AContactJid, Message::MessageType AType, int AShowMode);
	//IRostersClickHooker
	virtual bool rosterIndexSingleClicked(int AOrder, IRosterIndex *AIndex, const QMouseEvent *AEvent);
	virtual bool rosterIndexDoubleClicked(int AOrder, IRosterIndex *AIndex, const QMouseEvent *AEvent);
	//IXmppUriHandler
	virtual bool xmppUriOpen(const Jid &AStreamJid, const Jid &AContactJid, const QString &AAction, const QMultiMap<QString, QString> &AParams);
protected:
	IMessageChatWindow *getWindow(const Jid &AStreamJid, const Jid &AContactJid);
	IMessageChatWindow *findWindow(const Jid &AStreamJid, const Jid &AContactJid) const;
	void updateWindow(IMessageChatWindow *AWindow);
	void removeNotifiedMessages(IMessageChatWindow *AWindow);
	void showHistory(IMessageChatWindow *AWindow);
	void requestHistory(IMessageChatWindow *AWindow);
	void setMessageStyle(IMessageChatWindow *AWindow);
	void showDateSeparator(IMessageChatWindow *AWindow, const QDateTime &ADateTime);
	void fillContentOptions(const Jid &AStreamJid, const Jid &AContactJid, IMessageStyleContentOptions &AOptions) const;
	void showStyledStatus(IMessageChatWindow *AWindow, const QString &AMessage, bool ADontSave=false, const QDateTime &ATime=QDateTime::currentDateTime());
	void showStyledMessage(IMessageChatWindow *AWindow, const Message &AMessage);
	bool isSelectionAccepted(const QList<IRosterIndex *> &ASelected) const;
	QMap<Jid, QList<Jid> > getSortedAddresses(const QMultiMap<Jid,Jid> &AAddresses) const;
protected slots:
	void onWindowActivated();
	void onWindowClosed();
	void onWindowDestroyed();
	void onWindowAddressChanged();
	void onWindowAvailAddressesChanged();
	void onWindowAddressMenuRequested(Menu *AMenu);
	void onWindowContextMenuRequested(Menu *AMenu);
	void onWindowToolTipsRequested(QMap<int,QString> &AToolTips);
	void onWindowNotifierActiveNotifyChanged(int ANotifyId);
	void onWindowContentAppended(const QString &AHtml, const IMessageStyleContentOptions &AOptions);
	void onWindowMessageStyleOptionsChanged(const IMessageStyleOptions &AOptions, bool ACleared);
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
	void onArchiveRequestFailed(const QString &AId, const XmppError &AError);
	void onArchiveMessagesLoaded(const QString &AId, const IArchiveCollectionBody &ABody);
	void onRostersViewIndexContextMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu);
	void onStyleOptionsChanged(const IMessageStyleOptions &AOptions, int AMessageType, const QString &AContext);
private:
	IAvatars *FAvatars;
	IMessageWidgets *FMessageWidgets;
	IMessageProcessor *FMessageProcessor;
	IMessageStyleManager *FMessageStyleManager;
	IRosterManager *FRosterManager;
	IPresenceManager *FPresenceManager;
	IMessageArchiver *FMessageArchiver;
	IRostersView *FRostersView;
	IRostersModel *FRostersModel;
	IStatusIcons *FStatusIcons;
	IStatusChanger *FStatusChanger;
	INotifications *FNotifications;
	IAccountManager *FAccountManager;
	IXmppUriQueries *FXmppUriQueries;
	IRecentContacts *FRecentContacts;
private:
	QList<IMessageChatWindow *> FWindows;
	QMap<IMessageChatWindow *, QTimer *> FDestroyTimers;
	QMultiMap<IMessageChatWindow *, int> FNotifiedMessages;
	QMap<IMessageChatWindow *, WindowStatus> FWindowStatus;
private:
	QMap<QString, IMessageChatWindow *> FHistoryRequests;
	QMap<IMessageChatWindow *, QList<Message> > FPendingMessages;
	QMap<IMessageChatWindow *, QList<WindowContent> > FPendingContent;
	QMap<IMessageChatWindow *, IArchiveCollectionBody> FHistoryMessages;
};

#endif // CHATMESSAGEHANDLER_H
