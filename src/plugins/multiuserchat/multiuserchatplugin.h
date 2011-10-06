#ifndef MULTIUSERCHATPLUGIN_H
#define MULTIUSERCHATPLUGIN_H

#include <QMessageBox>
#include <definitions/actiongroups.h>
#include <definitions/toolbargroups.h>
#include <definitions/dataformtypes.h>
#include <definitions/rosterindextyperole.h>
#include <definitions/rosterlabelorders.h>
#include <definitions/rosternotifyorders.h>
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
#include <interfaces/istanzaprocessor.h>
#include <interfaces/ipresence.h>
#include <interfaces/imainwindow.h>
#include <interfaces/itraymanager.h>
#include <interfaces/iservicediscovery.h>
#include <interfaces/inotifications.h>
#include <interfaces/idataforms.h>
#include <interfaces/ivcard.h>
#include <interfaces/iregistraton.h>
#include <interfaces/ixmppuriqueries.h>
#include <interfaces/ioptionsmanager.h>
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
	public IOptionsHolder
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IMultiUserChatPlugin IXmppUriHandler IDiscoFeatureHandler IMessageHandler IDataLocalizer IOptionsHolder);
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
	//IMultiUserChatPlugin
	virtual IPluginManager *pluginManager() const { return FPluginManager; }
	virtual bool requestRoomNick(const Jid &AStreamJid, const Jid &ARoomJid);
	virtual IMultiUserChat *getMultiUserChat(const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANick,const QString &APassword);
	virtual QList<IMultiUserChat *> multiUserChats() const { return FChats; }
	virtual IMultiUserChat *multiUserChat(const Jid &AStreamJid, const Jid &ARoomJid) const;
	virtual IMultiUserChatWindow *getMultiChatWindow(const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANick, const QString &APassword);
	virtual QList<IMultiUserChatWindow *> multiChatWindows() const { return FChatWindows; }
	virtual IMultiUserChatWindow *multiChatWindow(const Jid &AStreamJid, const Jid &ARoomJid) const;
	virtual void showJoinMultiChatDialog(const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANick, const QString &APassword);
signals:
	void roomNickReceived(const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANick);
	void multiUserChatCreated(IMultiUserChat *AMultiChat);
	void multiUserChatDestroyed(IMultiUserChat *AMultiChat);
	void multiChatWindowCreated(IMultiUserChatWindow *AWindow);
	void multiChatWindowDestroyed(IMultiUserChatWindow *AWindow);
	void multiUserContextMenu(IMultiUserChatWindow *AWindow, IMultiUser *AUser, Menu *AMenu);
protected:
	void insertChatAction(IMultiUserChatWindow *AWindow);
	void removeChatAction(IMultiUserChatWindow *AWindow);
	void registerDiscoFeatures();
	QString streamVCardNick(const Jid &AStreamJid) const;
	Menu *createInviteMenu(const Jid &AContactJid, QWidget *AParent) const;
	Action *createJoinAction(const Jid &AStreamJid, const Jid &ARoomJid, QObject *AParent) const;
protected slots:
	void onMultiUserContextMenu(IMultiUser *AUser, Menu *AMenu);
	void onMultiUserChatDestroyed();
	void onMultiChatWindowDestroyed();
	void onStreamRemoved(IXmppStream *AXmppStream);
	void onJoinActionTriggered(bool);
	void onShowAllRoomsTriggered(bool);
	void onLeaveHiddenRoomsTriggered(bool);
	void onRosterIndexContextMenu(const QList<IRosterIndex *> &AIndexes, int ALabelId, Menu *AMenu);
	void onChatActionTriggered(bool);
	void onDiscoInfoReceived(const IDiscoInfo &ADiscoInfo);
	void onRegisterFieldsReceived(const QString &AId, const IRegisterFields &AFields);
	void onRegisterErrorReceived(const QString &AId, const QString &AError);
	void onInviteDialogFinished(int AResult);
	void onInviteActionTriggered(bool);
private:
	IPluginManager *FPluginManager;
	IMessageWidgets *FMessageWidgets;
	IMessageProcessor *FMessageProcessor;
	IRostersViewPlugin *FRostersViewPlugin;
	IMainWindowPlugin *FMainWindowPlugin;
	ITrayManager *FTrayManager;
	IXmppStreams *FXmppStreams;
	IServiceDiscovery *FDiscovery;
	INotifications *FNotifications;
	IDataForms *FDataForms;
	IVCardPlugin *FVCardPlugin;
	IRegistration *FRegistration;
	IXmppUriQueries *FXmppUriQueries;
	IOptionsManager *FOptionsManager;
private:
	Menu *FChatMenu;
private:
	QList<IMultiUserChat *> FChats;
	QMap<int, Message> FActiveInvites;
	QList<IMultiUserChatWindow *> FChatWindows;
	QMap<IMultiUserChatWindow *, Action *> FChatActions;
	QMap<QMessageBox *,InviteFields> FInviteDialogs;
	QMap<QString, QPair<Jid,Jid> > FNickRequests;
};

#endif // MULTIUSERCHATPLUGIN_H
