#ifndef NORMALMESSAGEHANDLER_H
#define NORMALMESSAGEHANDLER_H

#define NORMALMESSAGEHANDLER_UUID "{8592e3c3-ef5e-42a9-91c9-faf1ed9a91cc}"

#include <QQueue>
#include <QMultiMap>
#include <definitions/messagehandlerorders.h>
#include <definitions/rosterindextyperole.h>
#include <definitions/rosternotifyorders.h>
#include <definitions/recentitemtypes.h>
#include <definitions/notificationtypes.h>
#include <definitions/notificationdataroles.h>
#include <definitions/notificationtypeorders.h>
#include <definitions/tabpagenotifypriorities.h>
#include <definitions/messagedataroles.h>
#include <definitions/actiongroups.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/soundfiles.h>
#include <definitions/shortcuts.h>
#include <definitions/optionvalues.h>
#include <definitions/optionnodes.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/xmppurihandlerorders.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imessageprocessor.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/inotifications.h>
#include <interfaces/istatusicons.h>
#include <interfaces/irostersview.h>
#include <interfaces/ipresence.h>
#include <interfaces/iroster.h>
#include <interfaces/ixmppuriqueries.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/irecentcontacts.h>
#include <utils/widgetmanager.h>
#include <utils/xmpperror.h>
#include <utils/textmanager.h>
#include <utils/shortcuts.h>
#include <utils/options.h>

class NormalMessageHandler :
	public QObject,
	public IPlugin,
	public IMessageHandler,
	public IXmppUriHandler,
	public IOptionsHolder
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IMessageHandler IXmppUriHandler IOptionsHolder);
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
	//IMessageHandler
	virtual bool messageCheck(int AOrder, const Message &AMessage, int ADirection);
	virtual bool messageDisplay(const Message &AMessage, int ADirection);
	virtual INotification messageNotify(INotifications *ANotifications, const Message &AMessage, int ADirection);
	virtual bool messageShowWindow(int AMessageId);
	virtual bool messageShowWindow(int AOrder, const Jid &AStreamJid, const Jid &AContactJid, Message::MessageType AType, int AShowMode);
	//IOptionsHolder
	virtual QMultiMap<int, IOptionsWidget *> optionsWidgets(const QString &ANodeId, QWidget *AParent);
protected:
	IMessageWindow *getWindow(const Jid &AStreamJid, const Jid &AContactJid, IMessageWindow::Mode AMode);
	IMessageWindow *findWindow(const Jid &AStreamJid, const Jid &AContactJid);
	bool showNextMessage(IMessageWindow *AWindow);
	void updateWindow(IMessageWindow *AWindow);
	void removeCurrentMessageNotify(IMessageWindow *AWindow);
	void removeNotifiedMessages(IMessageWindow *AWindow, int AMessageId = -1);
	void setMessageStyle(IMessageWindow *AWindow);
	void fillContentOptions(IMessageWindow *AWindow, IMessageContentOptions &AOptions) const;
	void showStyledMessage(IMessageWindow *AWindow, const Message &AMessage);
	bool isSelectionAccepted(const QList<IRosterIndex *> &ASelected) const;
protected slots:
	void onMessageReady();
	void onShowNextMessage();
	void onReplyMessage();
	void onForwardMessage();
	void onShowChatWindow();
	void onWindowActivated();
	void onWindowDestroyed();
	void onWindowNotifierActiveNotifyChanged(int ANotifyId);
	void onStatusIconsChanged();
	void onShowWindowAction(bool);
	void onShortcutActivated(const QString &AId, QWidget *AWidget);
	void onRosterIndexMultiSelection(const QList<IRosterIndex *> &ASelected, bool &AAccepted);
	void onRosterIndexContextMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu);
	void onPresenceItemReceived(IPresence *APresence, const IPresenceItem &AItem, const IPresenceItem &ABefore);
	void onStyleOptionsChanged(const IMessageStyleOptions &AOptions, int AMessageType, const QString &AContext);
private:
	IMessageWidgets *FMessageWidgets;
	IMessageProcessor *FMessageProcessor;
	IMessageStyles *FMessageStyles;
	IStatusIcons *FStatusIcons;
	IPresencePlugin *FPresencePlugin;
	IRostersView *FRostersView;
	IXmppUriQueries *FXmppUriQueries;
	IOptionsManager *FOptionsManager;
	IRecentContacts *FRecentContacts;
private:
	QList<IMessageWindow *> FWindows;
	QMultiMap<IMessageWindow *, int> FNotifiedMessages;
	QMap<IMessageWindow *, QQueue<Message> > FMessageQueue;
};

#endif // NORMALMESSAGEHANDLER_H
