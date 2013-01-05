#ifndef MULTIUSERCHATWINDOW_H
#define MULTIUSERCHATWINDOW_H

#include <QStandardItemModel>
#include <definitions/messagedataroles.h>
#include <definitions/messagehandlerorders.h>
#include <definitions/multiuserdataroles.h>
#include <definitions/namespaces.h>
#include <definitions/actiongroups.h>
#include <definitions/recentitemtypes.h>
#include <definitions/rosternotifyorders.h>
#include <definitions/notificationtypes.h>
#include <definitions/notificationdataroles.h>
#include <definitions/notificationtypeorders.h>
#include <definitions/tabpagenotifypriorities.h>
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
#include <interfaces/irecentcontacts.h>
#include <utils/options.h>
#include <utils/shortcuts.h>
#include <utils/textmanager.h>
#include <utils/xmpperror.h>
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
	QDate lastDateSeparator;
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
	Q_INTERFACES(IMultiUserChatWindow ITabPage IMessageHandler);
public:
	MultiUserChatWindow(IMultiUserChatPlugin *AChatPlugin, IMultiUserChat *AMultiChat);
	~MultiUserChatWindow();
	virtual QMainWindow *instance() { return this; }
	//ITabWindowPage
	virtual QString tabPageId() const;
	virtual bool isVisibleTabPage() const;
	virtual bool isActiveTabPage() const;
	virtual void assignTabPage();
	virtual void showTabPage();
	virtual void showMinimizedTabPage();
	virtual void closeTabPage();
	virtual QIcon tabPageIcon() const;
	virtual QString tabPageCaption() const;
	virtual QString tabPageToolTip() const;
	virtual ITabPageNotifier *tabPageNotifier() const;
	virtual void setTabPageNotifier(ITabPageNotifier *ANotifier);
	//IMessageHandler
	virtual bool messageCheck(int AOrder, const Message &AMessage, int ADirection);
	virtual bool messageDisplay(const Message &AMessage, int ADirection);
	virtual INotification messageNotify(INotifications *ANotifications, const Message &AMessage, int ADirection);
	virtual bool messageShowWindow(int AMessageId);
	virtual bool messageShowWindow(int AOrder, const Jid &AStreamJid, const Jid &AContactJid, Message::MessageType AType, int AShowMode);
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
	virtual void contextMenuForWindow(Menu *AMenu);
	virtual void contextMenuForUser(IMultiUser *AUser, Menu *AMenu);
	virtual void exitAndDestroy(const QString &AStatus, int AWaitClose = 15000);
signals:
	//ITabWindowPage
	void tabPageAssign();
	void tabPageShow();
	void tabPageShowMinimized();
	void tabPageClose();
	void tabPageClosed();
	void tabPageChanged();
	void tabPageActivated();
	void tabPageDeactivated();
	void tabPageDestroyed();
	void tabPageNotifierChanged();
	//IMultiUserChatWindow
	void chatWindowCreated(IChatWindow *AWindow);
	void chatWindowDestroyed(IChatWindow *AWindow);
	void multiChatWindowContextMenu(Menu *AMenu);
	void multiUserContextMenu(IMultiUser *AUser, Menu *AMenu);
protected:
	void initialize();
	void connectMultiChat();
	void createMessageWidgets();
	void createStaticRoomActions();
	void updateStaticRoomActions();
	void saveWindowState();
	void loadWindowState();
	void saveWindowGeometry();
	void loadWindowGeometry();
	void updateRecentItemActiveTime();
	void showDateSeparator(IViewWidget *AView, const QDateTime &ADateTime);
	bool showStatusCodes(const QString &ANick, const QList<int> &ACodes);
	void highlightUserRole(IMultiUser *AUser);
	void highlightUserAffiliation(IMultiUser *AUser);
	void setToolTipForUser(IMultiUser *AUser);
	bool execShortcutCommand(const QString &AText);
protected:
	bool isMentionMessage(const Message &AMessage) const;
	void setMessageStyle();
	void showTopic(const QString &ATopic);
	void showStatusMessage(const QString &AMessage, int AType=0, int AStatus=0, bool ADontSave=false, const QDateTime &ATime=QDateTime::currentDateTime());
	void showUserMessage(const Message &AMessage, const QString &ANick);
	void showHistory();
	void updateWindow();
	void refreshCompleteNicks();
	void updateListItem(const Jid &AContactJid);
	void removeActiveMessages();
protected:
	void setChatMessageStyle(IChatWindow *AWindow);
	void fillChatContentOptions(IChatWindow *AWindow, IMessageContentOptions &AOptions) const;
	void showChatStatus(IChatWindow *AWindow, const QString &AMessage, int AStatus=0, const QDateTime &ATime=QDateTime::currentDateTime());
	void showChatMessage(IChatWindow *AWindow, const Message &AMessage);
	void showChatHistory(IChatWindow *AWindow);
	IChatWindow *getChatWindow(const Jid &AContactJid);
	void removeActiveChatMessages(IChatWindow *AWindow);
	void updateChatWindow(IChatWindow *AWindow);
protected:
	bool event(QEvent *AEvent);
	void showEvent(QShowEvent *AEvent);
	void closeEvent(QCloseEvent *AEvent);
	bool eventFilter(QObject *AObject, QEvent *AEvent);
protected slots:
	void onChatOpened();
	void onChatNotify(const QString &ANotify);
	void onChatError(const QString &AMessage);
	void onChatClosed();
	void onStreamJidChanged(const Jid &ABefore, const Jid &AAfter);
	void onRejoinAfterKick();
	//Occupant
	void onUserPresence(IMultiUser *AUser, int AShow, const QString &AStatus);
	void onUserDataChanged(IMultiUser *AUser, int ARole, const QVariant &ABefore, const QVariant &AAfter);
	void onUserNickChanged(IMultiUser *AUser, const QString &AOldNick, const QString &ANewNick);
	void onPresenceChanged(int AShow, const QString &AStatus);
	void onSubjectChanged(const QString &ANick, const QString &ASubject);
	void onServiceMessageReceived(const Message &AMessage);
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
	void onNotifierActiveNotifyChanged(int ANotifyId);
	void onEditWidgetKeyEvent(QKeyEvent *AKeyEvent, bool &AHooked);
	void onViewContextQuoteActionTriggered(bool);
	void onViewWidgetContextMenu(const QPoint &APosition, const QTextDocumentFragment &AText, Menu *AMenu);
	void onWindowActivated();
	void onChatMessageReady();
	void onChatWindowActivated();
	void onChatWindowClosed();
	void onChatWindowDestroyed();
	void onChatNotifierActiveNotifyChanged(int ANotifyId);
	void onHorizontalSplitterMoved(int APos, int AIndex);
	void onStyleOptionsChanged(const IMessageStyleOptions &AOptions, int AMessageType, const QString &AContext);
	void onArchiveMessagesLoaded(const QString &AId, const IArchiveCollectionBody &ABody);
	void onArchiveRequestFailed(const QString &AId, const QString &AError);
protected slots:
	void onNickMenuActionTriggered(bool);
	void onToolBarActionTriggered(bool);
	void onOpenChatWindowActionTriggered(bool);
	void onChangeUserRoleActionTriggeted(bool);
	void onChangeUserAffiliationActionTriggered(bool);
	void onClearChatWindowActionTriggered(bool);
	void onDataFormMessageDialogAccepted();
	void onAffiliationListDialogAccepted();
	void onConfigFormDialogAccepted();
protected slots:
	void onUserItemDoubleClicked(const QModelIndex &AIndex);
	void onStatusIconsChanged();
	void onAccountOptionsChanged(const OptionsNode &ANode);
	void onShortcutActivated(const QString &AId, QWidget *AWidget);
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
	IRecentContacts *FRecentContacts;
private:
	IViewWidget *FViewWidget;
	IEditWidget *FEditWidget;
	IMenuBarWidget *FMenuBarWidget;
	IToolBarWidget *FToolBarWidget;
	IStatusBarWidget *FStatusBarWidget;
	ITabPageNotifier *FTabPageNotifier;
private:
	Action *FEnterRoom;
	Action *FExitRoom;
private:
	Menu *FToolsMenu;
	Action *FChangeNick;
	Action *FInviteContact;
	Action *FRequestVoice;
	Action *FClearChat;
	Action *FChangeTopic;
	Action *FBanList;
	Action *FMembersList;
	Action *FAdminsList;
	Action *FOwnersList;
	Action *FConfigRoom;
	Action *FDestroyRoom;
private:
	int FUsersListWidth;
	bool FShownDetached;
	bool FDestroyOnChatClosed;
	QString FTabPageToolTip;
	QList<int> FActiveMessages;
	QList<IChatWindow *> FChatWindows;
	QMap<IChatWindow *, QTimer *> FDestroyTimers;
	QMultiMap<IChatWindow *,int> FActiveChatMessages;
	QMap<int, IDataDialogWidget *> FDataFormMessages;
	QHash<IMultiUser *, UserStatus> FUserStatus;
	QMap<IViewWidget *, WindowStatus> FWindowStatus;
	QMap<QString, IChatWindow *> FHistoryRequests;
	QMap<IChatWindow *, QList<Message> > FPendingMessages;
private:
	UsersProxyModel *FUsersProxy;
	QStandardItemModel *FUsersModel;
	QHash<IMultiUser *, QStandardItem *> FUsers;
private:
	int FStartCompletePos;
	QString FCompleteNickStarts;
	QString FCompleteNickLast;
	QList<QString> FCompleteNicks;
	QList<QString>::const_iterator FCompleteIt;
};

#endif // MULTIUSERCHATWINDOW_H
