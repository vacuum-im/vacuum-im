#ifndef MULTIUSERCHATWINDOW_H
#define MULTIUSERCHATWINDOW_H

#include <QStandardItemModel>
#include <definitions/messagedataroles.h>
#include <definitions/messagehandlerorders.h>
#include <definitions/multiuserdataroles.h>
#include <definitions/namespaces.h>
#include <definitions/actiongroups.h>
#include <definitions/notificationtypes.h>
#include <definitions/notificationdataroles.h>
#include <definitions/soundfiles.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/shortcuts.h>
#include <definitions/optionvalues.h>
#include <definitions/toolbargroups.h>
#include <interfaces/imultiuserchat.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/imessageprocessor.h>
#include <interfaces/imessagestyles.h>
#include <interfaces/imessagearchiver.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/irostersview.h>
#include <interfaces/istatusicons.h>
#include <interfaces/istatuschanger.h>
#include <interfaces/iaccountmanager.h>
#include <interfaces/iroster.h>
#include <interfaces/ipresence.h>
#include <utils/widgetmanager.h>
#include <utils/options.h>
#include <utils/widgetmanager.h>
#include "edituserslistdialog.h"
#include "usercontextmenu.h"
#include "inputtextdialog.h"
#include "usersproxymodel.h"
#include "ui_multiuserchatwindow.h"

struct WindowStatus
{
	QDateTime startTime;
	QDateTime createTime;
};

struct UserStatus
{
	QString lastStatusShow;
};


class MultiUserChatWindow :
			public QMainWindow,
			public IMultiUserChatWindow,
			public IMessageHandler
{
	Q_OBJECT;
	Q_INTERFACES(IMultiUserChatWindow ITabWindowPage IMessageHandler);
public:
	MultiUserChatWindow(IMultiUserChatPlugin *AChatPlugin, IMultiUserChat *AMultiChat);
	~MultiUserChatWindow();
	virtual QMainWindow *instance() { return this; }
	//ITabWindowPage
	virtual QString tabPageId() const;
	virtual bool isActive() const;
	virtual void showWindow();
	virtual void closeWindow();
	//IMessageHandler
	virtual bool checkMessage(int AOrder, const Message &AMessage);
	virtual bool receiveMessage(int AMessageId);
	virtual bool showMessage(int AMessageId);
	virtual INotification notification(INotifications *ANotifications, const Message &AMessage);
	virtual bool openWindow(int AOrder, const Jid &AStreamJid, const Jid &AContactJid, Message::MessageType AType);
	//IMultiUserChatWindow
	virtual Jid streamJid() const;
	virtual Jid roomJid() const;
	virtual IViewWidget *viewWidget() const;
	virtual IEditWidget *editWidget() const;
	virtual IMenuBarWidget *menuBarWidget() const;
	virtual IToolBarWidget *toolBarWidget() const;
	virtual IStatusBarWidget *statusBarWidget() const;
	virtual IMultiUserChat *multiUserChat() const;
	virtual IChatWindow *openChatWindow(const Jid &AContactJid);
	virtual IChatWindow *findChatWindow(const Jid &AContactJid) const;
	virtual void contextMenuForUser(IMultiUser *AUser, Menu *AMenu);
	virtual void exitAndDestroy(const QString &AStatus, int AWaitClose = 5000);
signals:
	//ITabWindowPage
	void windowShow();
	void windowClose();
	void windowChanged();
	void windowActivated();
	void windowDeactivated();
	void windowDestroyed();
	//IMultiUserChatWindow
	void windowClosed();
	void chatWindowCreated(IChatWindow *AWindow);
	void chatWindowDestroyed(IChatWindow *AWindow);
	void multiUserContextMenu(IMultiUser *AUser, Menu *AMenu);
protected:
	void initialize();
	void connectMultiChat();
	void createMessageWidgets();
	void createStaticRoomActions();
	void updateStaticRoomActions();
	void createStaticUserContextActions();
	void insertStaticUserContextActions(Menu *AMenu, IMultiUser *AUser);
	void saveWindowState();
	void loadWindowState();
	void saveWindowGeometry();
	void loadWindowGeometry();
	bool showStatusCodes(const QString &ANick, const QList<int> &ACodes);
	void highlightUserRole(IMultiUser *AUser);
	void highlightUserAffiliation(IMultiUser *AUser);
	void setToolTipForUser(IMultiUser *AUser);
	bool execShortcutCommand(const QString &AText);
protected:
	bool isMentionMessage(const Message &AMessage) const;
	void setMessageStyle();
	void showTopic(const QString &ATopic);
	void showStatusMessage(const QString &AMessage, int AContentType=0);
	void showUserMessage(const Message &AMessage, const QString &ANick);
	void showHistory();
	void updateWindow();
	void updateListItem(const Jid &AContactJid);
	void removeActiveMessages();
protected:
	void setChatMessageStyle(IChatWindow *AWindow);
	void fillChatContentOptions(IChatWindow *AWindow, IMessageContentOptions &AOptions) const;
	void showChatStatus(IChatWindow *AWindow, const QString &AMessage);
	void showChatMessage(IChatWindow *AWindow, const Message &AMessage);
	void showChatHistory(IChatWindow *AWindow);
	IChatWindow *getChatWindow(const Jid &AContactJid);
	void removeActiveChatMessages(IChatWindow *AWindow);
	void updateChatWindow(IChatWindow *AWindow);
protected:
	virtual bool event(QEvent *AEvent);
	virtual void showEvent(QShowEvent *AEvent);
	virtual void closeEvent(QCloseEvent *AEvent);
	virtual bool eventFilter(QObject *AObject, QEvent *AEvent);
protected slots:
	void onChatOpened();
	void onChatNotify(const QString &ANotify);
	void onChatError(const QString &AMessage);
	void onChatClosed();
	void onStreamJidChanged(const Jid &ABefore, const Jid &AAfter);
	//Occupant
	void onUserPresence(IMultiUser *AUser, int AShow, const QString &AStatus);
	void onUserDataChanged(IMultiUser *AUser, int ARole, const QVariant &ABefore, const QVariant &AAfter);
	void onUserNickChanged(IMultiUser *AUser, const QString &AOldNick, const QString &ANewNick);
	void onPresenceChanged(int AShow, const QString &AStatus);
	void onSubjectChanged(const QString &ANick, const QString &ASubject);
	void onServiceMessageReceived(const Message &AMessage);
	void onMessageReceived(const QString &ANick, const Message &AMessage);
	void onInviteDeclined(const Jid &AContactJid, const QString &AReason);
	//Moderator
	void onUserKicked(const QString &ANick, const QString &AReason, const QString &AByUser);
	//Administrator
	void onUserBanned(const QString &ANick, const QString &AReason, const QString &AByUser);
	void onAffiliationListReceived(const QString &AAffiliation, const QList<IMultiUserListItem> &AList);
	//Owner
	void onConfigFormReceived(const IDataForm &AForm);
	void onRoomDestroyed(const QString &AReason);
protected slots:
	void onMessageReady();
	void onMessageAboutToBeSend();
	void onEditWidgetKeyEvent(QKeyEvent *AKeyEvent, bool &AHooked);
	void onWindowActivated();
	void onChatMessageReady();
	void onChatWindowActivated();
	void onChatWindowClosed();
	void onChatWindowDestroyed();
	void onHorizontalSplitterMoved(int APos, int AIndex);
	void onStyleOptionsChanged(const IMessageStyleOptions &AOptions, int AMessageType, const QString &AContext);
protected slots:
	void onNickMenuActionTriggered(bool);
	void onToolBarActionTriggered(bool);
	void onRoomUtilsActionTriggered(bool);
	void onDataFormMessageDialogAccepted();
	void onAffiliationListDialogAccepted();
	void onConfigFormDialogAccepted();
protected slots:
	void onUserItemActivated(const QModelIndex &AIndex);
	void onStatusIconsChanged();
	void onAccountOptionsChanged(const OptionsNode &ANode);
private:
	Ui::MultiUserChatWindowClass ui;
private:
	IMessageWidgets *FMessageWidgets;
	IMessageProcessor *FMessageProcessor;
	IMessageStyles *FMessageStyles;
	IMessageArchiver *FMessageArchiver;
	IDataForms *FDataForms;
	IStatusIcons *FStatusIcons;
	IStatusChanger *FStatusChanger;
	IMultiUserChat *FMultiChat;
	IMultiUserChatPlugin *FChatPlugin;
private:
	IViewWidget *FViewWidget;
	IEditWidget *FEditWidget;
	IMenuBarWidget *FMenuBarWidget;
	IToolBarWidget *FToolBarWidget;
	IStatusBarWidget *FStatusBarWidget;
private:
	Menu *FToolsMenu;
	Action *FChangeNick;
	Action *FInviteContact;
	Action *FRequestVoice;
	Action *FClearChat;
	Action *FChangeSubject;
	Action *FBanList;
	Action *FMembersList;
	Action *FAdminsList;
	Action *FOwnersList;
	Action *FConfigRoom;
	Action *FDestroyRoom;
	Action *FEnterRoom;
	Menu *FModeratorUtilsMenu;
	Action *FSetRoleNode;
	Action *FSetAffilOutcast;
	Menu *FChangeRole;
	Action *FSetRoleVisitor;
	Action *FSetRoleParticipant;
	Action *FSetRoleModerator;
	Menu *FChangeAffiliation;
	Action *FSetAffilNone;
	Action *FSetAffilMember;
	Action *FSetAffilAdmin;
	Action *FSetAffilOwner;
	Action *FExitRoom;
private:
	int FUsersListWidth;
	bool FShownDetached;
	bool FDestroyOnChatClosed;
	QList<int> FActiveMessages;
	QList<IChatWindow *> FChatWindows;
	QMultiMap<IChatWindow *,int> FActiveChatMessages;
	QMap<int, IDataDialogWidget *> FDataFormMessages;
	QHash<IMultiUser *, UserStatus> FUserStatus;
	QMap<IViewWidget *, WindowStatus> FWindowStatus;
private:
	UsersProxyModel *FUsersProxy;
	QStandardItemModel *FUsersModel;
	QHash<IMultiUser *, QStandardItem *> FUsers;
};

#endif // MULTIUSERCHATWINDOW_H
