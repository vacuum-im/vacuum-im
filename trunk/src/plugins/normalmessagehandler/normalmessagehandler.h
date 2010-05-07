#ifndef NORMALMESSAGEHANDLER_H
#define NORMALMESSAGEHANDLER_H

#define NORMALMESSAGEHANDLER_UUID "{8592e3c3-ef5e-42a9-91c9-faf1ed9a91cc}"

#include <QMultiMap>
#include <definations/messagehandlerorders.h>
#include <definations/rosterindextyperole.h>
#include <definations/rosterlabelorders.h>
#include <definations/notificationdataroles.h>
#include <definations/actiongroups.h>
#include <definations/resources.h>
#include <definations/menuicons.h>
#include <definations/soundfiles.h>
#include <definations/xmppurihandlerorders.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imessageprocessor.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/inotifications.h>
#include <interfaces/istatusicons.h>
#include <interfaces/irostersview.h>
#include <interfaces/ipresence.h>
#include <interfaces/ixmppuriqueries.h>
#include <utils/errorhandler.h>

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
	virtual void showMessage(int AMessageId);
	virtual void receiveMessage(int AMessageId);
	virtual INotification notification(INotifications *ANotifications, const Message &AMessage);
	virtual bool openWindow(int AOrder, const Jid &AStreamJid, const Jid &AContactJid, Message::MessageType AType);
protected:
	IMessageWindow *getWindow(const Jid &AStreamJid, const Jid &AContactJid, IMessageWindow::Mode AMode);
	IMessageWindow *findWindow(const Jid &AStreamJid, const Jid &AContactJid);
	void showWindow(IMessageWindow *AWindow);
	void showNextMessage(IMessageWindow *AWindow);
	void loadActiveMessages(IMessageWindow *AWindow);
	void updateWindow(IMessageWindow *AWindow);
	void setMessageStyle(IMessageWindow *AWindow);
	void fillContentOptions(IMessageWindow *AWindow, IMessageContentOptions &AOptions) const;
	void showStyledMessage(IMessageWindow *AWindow, const Message &AMessage);
protected slots:
	void onMessageReady();
	void onShowNextMessage();
	void onReplyMessage();
	void onForwardMessage();
	void onShowChatWindow();
	void onWindowDestroyed();
	void onStatusIconsChanged();
	void onShowWindowAction(bool);
	void onRosterIndexContextMenu(IRosterIndex *AIndex, Menu *AMenu);
	void onPresenceReceived(IPresence *APresence, const IPresenceItem &APresenceItem);
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
