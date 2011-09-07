#ifndef NORMALMESSAGEHANDLER_H
#define NORMALMESSAGEHANDLER_H

#define NORMALMESSAGEHANDLER_UUID "{8592e3c3-ef5e-42a9-91c9-faf1ed9a91cc}"

#include <QMultiMap>
#include <definitions/messagehandlerorders.h>
#include <definitions/rosterindextyperole.h>
#include <definitions/rosterlabelorders.h>
#include <definitions/rosternotifyorders.h>
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
#include <definitions/xmppurihandlerorders.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imessageprocessor.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/inotifications.h>
#include <interfaces/istatusicons.h>
#include <interfaces/irostersview.h>
#include <interfaces/ipresence.h>
#include <interfaces/ixmppuriqueries.h>
#include <utils/widgetmanager.h>
#include <utils/errorhandler.h>
#include <utils/textmanager.h>
#include <utils/shortcuts.h>

class NormalMessageHandler :
	public QObject,
	public IPlugin,
	public IMessageHandler,
	public IXmppUriHandler
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IMessageHandler IXmppUriHandler);
public:
	NormalMessageHandler();
	~NormalMessageHandler();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return NORMALMESSAGEHANDLER_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings() { return true; }
	virtual bool startPlugin() { return true; }
	//IXmppUriHandler
	virtual bool xmppUriOpen(const Jid &AStreamJid, const Jid &AContactJid, const QString &AAction, const QMultiMap<QString, QString> &AParams);
	//IMessageHandler
	virtual bool checkMessage(int AOrder, const Message &AMessage);
	virtual bool showMessage(int AMessageId);
	virtual bool receiveMessage(int AMessageId);
	virtual INotification notifyMessage(INotifications *ANotifications, const Message &AMessage);
	virtual bool createMessageWindow(int AOrder, const Jid &AStreamJid, const Jid &AContactJid, Message::MessageType AType, int AShowMode);
protected:
	IMessageWindow *getWindow(const Jid &AStreamJid, const Jid &AContactJid, IMessageWindow::Mode AMode);
	IMessageWindow *findWindow(const Jid &AStreamJid, const Jid &AContactJid);
	void showNextMessage(IMessageWindow *AWindow);
	void loadActiveMessages(IMessageWindow *AWindow);
	void updateWindow(IMessageWindow *AWindow);
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
	void onWindowDestroyed();
	void onStatusIconsChanged();
	void onShowWindowAction(bool);
	void onShortcutActivated(const QString &AId, QWidget *AWidget);
	void onRosterIndexMultiSelection(const QList<IRosterIndex *> &ASelected, bool &AAccepted);
	void onRosterIndexContextMenu(const QList<IRosterIndex *> &AIndexes, int ALabelId, Menu *AMenu);
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
private:
	QList<IMessageWindow *> FWindows;
	QMap<IMessageWindow *, Message> FLastMessages;
	QMultiMap<IMessageWindow *, int> FActiveMessages;
};

#endif // NORMALMESSAGEHANDLER_H
