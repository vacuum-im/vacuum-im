#ifndef MULTIUSERCHATWINDOW_H
#define MULTIUSERCHATWINDOW_H

#include <interfaces/iavatars.h>
#include <interfaces/imultiuserchat.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/imessageprocessor.h>
#include <interfaces/imessagestylemanager.h>
#include <interfaces/imessagearchiver.h>
#include <interfaces/ixmppstreammanager.h>
#include <interfaces/irostersview.h>
#include <interfaces/istatusicons.h>
#include <interfaces/istatuschanger.h>
#include <interfaces/irostermanager.h>
#include <interfaces/ipresencemanager.h>
#include <interfaces/irecentcontacts.h>
#include <interfaces/istanzaprocessor.h>
#include "multiuserview.h"
#include "inputtextdialog.h"
#include "edituserslistdialog.h"

struct WindowStatus {
	QDateTime startTime;
	QDateTime createTime;
	QDate lastDateSeparator;
};

struct WindowContent {
	QString html;
	IMessageStyleContentOptions options;
};

struct UserStatus {
	QString lastStatusShow;
};

class MultiUserChatWindow :
	public QMainWindow,
	public IMultiUserChatWindow,
	public IStanzaHandler,
	public IMessageHandler,
	public IMessageViewUrlHandler,
	public IMessageEditSendHandler
{
	Q_OBJECT;
	Q_INTERFACES(IMessageWindow IMultiUserChatWindow IMessageTabPage IStanzaHandler IMessageHandler IMessageViewUrlHandler IMessageEditSendHandler);
public:
	MultiUserChatWindow(IMultiUserChatManager *AMultiChatManager, IMultiUserChat *AMultiChat);
	~MultiUserChatWindow();
	virtual QMainWindow *instance() { return this; }
	//IMessageWindow
	virtual Jid streamJid() const;
	virtual Jid contactJid() const;
	virtual IMessageAddress *address() const;
	virtual IMessageInfoWidget *infoWidget() const;
	virtual IMessageViewWidget *viewWidget() const;
	virtual IMessageEditWidget *editWidget() const;
	virtual IMessageMenuBarWidget *menuBarWidget() const;
	virtual IMessageToolBarWidget *toolBarWidget() const;
	virtual IMessageStatusBarWidget *statusBarWidget() const;
	virtual IMessageReceiversWidget *receiversWidget() const;
	virtual SplitterWidget *messageWidgetsBox() const;
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
	virtual IMessageTabPageNotifier *tabPageNotifier() const;
	virtual void setTabPageNotifier(IMessageTabPageNotifier *ANotifier);
	//IStanzaHandler
	virtual bool stanzaReadWrite(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept);
	//IMessageEditSendHandler
	virtual bool messageEditSendPrepare(int AOrder, IMessageEditWidget *AWidget);
	virtual bool messageEditSendProcesse(int AOrder, IMessageEditWidget *AWidget);
	//IMessageViewUrlHandler
	virtual bool messageViewUrlOpen(int AOrder, IMessageViewWidget *AWidget, const QUrl &AUrl);
	//IMessageHandler
	virtual bool messageCheck(int AOrder, const Message &AMessage, int ADirection);
	virtual bool messageDisplay(const Message &AMessage, int ADirection);
	virtual INotification messageNotify(INotifications *ANotifications, const Message &AMessage, int ADirection);
	virtual bool messageShowWindow(int AMessageId);
	virtual bool messageShowWindow(int AOrder, const Jid &AStreamJid, const Jid &AContactJid, Message::MessageType AType, int AShowMode);
	//IMultiUserChatWindow
	virtual IMultiUserChat *multiUserChat() const;
	virtual IMultiUserView *multiUserView() const;
	virtual SplitterWidget *viewWidgetsBox() const;
	virtual SplitterWidget *usersWidgetsBox() const;
	virtual SplitterWidget *centralWidgetsBox() const;
	virtual IMessageChatWindow *openPrivateChatWindow(const Jid &AContactJid);
	virtual IMessageChatWindow *findPrivateChatWindow(const Jid &AContactJid) const;
	virtual void contextMenuForRoom(Menu *AMenu);
	virtual void contextMenuForUser(IMultiUser *AUser, Menu *AMenu);
	virtual void toolTipsForUser(IMultiUser *AUser, QMap<int,QString> &AToolTips);
	virtual void exitAndDestroy(const QString &AStatus, int AWaitClose = 5000);
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
	//IMessageWindow
	void widgetLayoutChanged();
	//IMultiUserChatWindow
	void multiChatContextMenu(Menu *AMenu);
	void multiUserContextMenu(IMultiUser *AUser, Menu *AMenu);
	void multiUserToolTips(IMultiUser *AUser, QMap<int,QString> &AToolTips);
	void privateChatWindowCreated(IMessageChatWindow *AWindow);
	void privateChatWindowDestroyed(IMessageChatWindow *AWindow);
protected:
	void initialize();
	void createMessageWidgets();
	void createStaticRoomActions();
protected:
	void saveWindowState();
	void loadWindowState();
	void saveWindowGeometry();
	void loadWindowGeometry();
protected:
	void refreshCompleteNicks();
	bool execShortcutCommand(const QString &AText);
	void updateRecentItemActiveTime(IMessageChatWindow *AWindow);
	void showDateSeparator(IMessageViewWidget *AView, const QDateTime &ADateTime);
	void showHTMLStatusMessage(IMessageViewWidget *AView, const QString &AHtml, int AType=0, int AStatus=0, const QDateTime &ATime=QDateTime::currentDateTime());
protected:
	bool isMentionMessage(const Message &AMessage) const;
	void setMultiChatMessageStyle();
	void showMultiChatTopic(const QString &ATopic, const QString &ANick = QString::null);
	void showMultiChatStatusMessage(const QString &AMessage, int AType=0, int AStatus=0, bool ADontSave=false, const QDateTime &ATime=QDateTime::currentDateTime());
	bool showMultiChatStatusCodes(const QList<int> &ACodes, const QString &ANick=QString::null, const QString &AMessage=QString::null);
	void showMultiChatUserMessage(const Message &AMessage, const QString &ANick);
	void requestMultiChatHistory();
	void updateMultiChatWindow();
	void removeMultiChatActiveMessages();
protected:
	IMessageChatWindow *getPrivateChatWindow(const Jid &AContactJid);
	void setPrivateChatMessageStyle(IMessageChatWindow *AWindow);
	void fillPrivateChatContentOptions(IMessageChatWindow *AWindow, IMessageStyleContentOptions &AOptions) const;
	void showPrivateChatStatusMessage(IMessageChatWindow *AWindow, const QString &AMessage, int AStatus=0, const QDateTime &ATime=QDateTime::currentDateTime());
	void showPrivateChatMessage(IMessageChatWindow *AWindow, const Message &AMessage);
	void requestPrivateChatHistory(IMessageChatWindow *AWindow);
	void updatePrivateChatWindow(IMessageChatWindow *AWindow);
	void removePrivateChatActiveMessages(IMessageChatWindow *AWindow);
protected:
	bool event(QEvent *AEvent);
	void showEvent(QShowEvent *AEvent);
	void closeEvent(QCloseEvent *AEvent);
	bool eventFilter(QObject *AObject, QEvent *AEvent);
protected slots:
	void onMultiChatStateChanged(int AState);
	//Common
	void onMultiChatRoomNameChanged(const QString &AName);
	void onMultiChatRequestFailed(const QString &AId, const XmppError &AError);
	//Occupant
	void onMultiChatPresenceChanged(const IPresenceItem &APresence);
	void onMultiChatInvitationDeclined(const Jid &AContactJid, const QString &AReason);
	void onMultiChatUserChanged(IMultiUser *AUser, int AData, const QVariant &ABefore);
	//Moderator
	void onMultiChatVoiceRequestReceived(const Message &AMessage);
	void onMultiChatSubjectChanged(const QString &ANick, const QString &ASubject);
	void onMultiChatUserKicked(const QString &ANick, const QString &AReason, const QString &AByUser);
	void onMultiChatUserBanned(const QString &ANick, const QString &AReason, const QString &AByUser);
	//Owner
	void onMultiChatRoomConfigLoaded(const QString &AId, const IDataForm &AForm);
	void onMultiChatRoomConfigUpdated(const QString &AId, const IDataForm &AForm);
	void onMultiChatRoomDestroyed(const QString &AId, const QString &AReason);
protected slots:
	void onMultiChatWindowActivated();
	void onMultiChatNotifierActiveNotifyChanged(int ANotifyId);
	void onMultiChatEditWidgetKeyEvent(QKeyEvent *AKeyEvent, bool &AHooked);
	void onMultiChatUserItemNotifyActivated(int ANotifyId);
	void onMultiChatUserItemDoubleClicked(const QModelIndex &AIndex);
	void onMultiChatUserItemContextMenu(QStandardItem *AItem, Menu *AMenu);
	void onMultiChatUserItemToolTips(QStandardItem *AItem, QMap<int,QString> &AToolTips);
	void onMultiChatContentAppended(const QString &AHtml, const IMessageStyleContentOptions &AOptions);
	void onMultiChatMessageStyleOptionsChanged(const IMessageStyleOptions &AOptions, bool ACleared);
protected slots:
	void onPrivateChatWindowActivated();
	void onPrivateChatWindowClosed();
	void onPrivateChatWindowDestroyed();
	void onPrivateChatClearWindowActionTriggered(bool);
	void onPrivateChatContextMenuRequested(Menu *AMenu);
	void onPrivateChatToolTipsRequested(QMap<int,QString> &AToolTips);
	void onPrivateChatNotifierActiveNotifyChanged(int ANotifyId);
	void onPrivateChatContentAppended(const QString &AHtml, const IMessageStyleContentOptions &AOptions);
	void onPrivateChatMessageStyleOptionsChanged(const IMessageStyleOptions &AOptions, bool ACleared);
protected slots:
	void onRoomActionTriggered(bool);
	void onNickCompleteMenuActionTriggered(bool);
	void onOpenPrivateChatWindowActionTriggered(bool);
	void onChangeUserRoleActionTriggeted(bool);
	void onChangeUserAffiliationActionTriggered(bool);
protected slots:
	void onStatusIconsChanged();
	void onAutoRejoinAfterKick();
	void onRoomConfigFormDialogAccepted();
	void onOptionsChanged(const OptionsNode &ANode);
	void onCentralSplitterHandleMoved(int AOrderId, int ASize);
	void onShortcutActivated(const QString &AId, QWidget *AWidget);
	void onArchiveRequestFailed(const QString &AId, const XmppError &AError);
	void onArchiveMessagesLoaded(const QString &AId, const IArchiveCollectionBody &ABody);
	void onStyleOptionsChanged(const IMessageStyleOptions &AOptions, int AMessageType, const QString &AContext);
private:
	Action *FClearChat;
	Action *FEnterRoom;
	Action *FExitRoom;
	Action *FChangeNick;
	Action *FInviteContact;
	Action *FRequestVoice;
	Action *FChangeTopic;
	Action *FEditAffiliations;
	Action *FConfigRoom;
	Action *FDestroyRoom;
	Action *FUsersHide;
private:
	IAvatars *FAvatars;
	IDataForms *FDataForms;
	IStatusIcons *FStatusIcons;
	IStatusChanger *FStatusChanger;
	IMessageWidgets *FMessageWidgets;
	IRecentContacts *FRecentContacts;
	IStanzaProcessor *FStanzaProcessor;
	IMessageArchiver *FMessageArchiver;
	IMessageProcessor *FMessageProcessor;
	IMultiUserChatManager *FMultiChatManager;
	IMessageStyleManager *FMessageStyleManager;
private:
	SplitterWidget *FMainSplitter;
	SplitterWidget *FCentralSplitter;
	SplitterWidget *FViewSplitter;
	SplitterWidget *FUsersSplitter;
private:
	IMessageAddress *FAddress;
	IMessageInfoWidget *FInfoWidget;
	IMessageViewWidget *FViewWidget;
	IMessageEditWidget *FEditWidget;
	IMessageMenuBarWidget *FMenuBarWidget;
	IMessageToolBarWidget *FToolBarWidget;
	IMessageStatusBarWidget *FStatusBarWidget;
	IMessageTabPageNotifier *FTabPageNotifier;
private:
	int FSHIAnyStanza;
	bool FStateLoaded;
	bool FShownDetached;
	bool FDestroyOnChatClosed;
	QString FTabPageToolTip;
	QString FLastAffiliation;
	QDateTime FLastStanzaTime;
private:
	QString FRoleRequestId;
	QString FAffilRequestId;
	QString FDestroyRequestId;
	QString FLoadConfigRequestId;
	QString FUpdateConfigRequestId;
private:
	int FStartCompletePos;
	QString FCompleteNickStarts;
	QString FCompleteNickLast;
	QList<QString> FCompleteNicks;
	QList<QString>::const_iterator FCompleteIt;
private:
	IMultiUserChat *FMultiChat;
	IMultiUserView *FUsersView;
	QHash<IMultiUser *, UserStatus> FUsers;
private:
	QMap<int, int> FActiveChatMessageNotify;
	QList<IMessageChatWindow *> FPrivateChatWindows;
	QMap<IMessageChatWindow *, QTimer *> FDestroyTimers;
	QMultiMap<IMessageChatWindow *,int> FActiveChatMessages;
private:
	QList<int> FActiveMessages;
	QMap<QString, IMessageChatWindow *> FHistoryRequests;
	QMap<IMessageViewWidget *, WindowStatus> FWindowStatus;
	QMap<IMessageChatWindow *, QList<Message> > FPendingMessages;
	QMap<IMessageChatWindow *, QList<WindowContent> > FPendingContent;
};

#endif // MULTIUSERCHATWINDOW_H
