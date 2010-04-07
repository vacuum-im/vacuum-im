#ifndef MULTIUSERCHATWINDOW_H
#define MULTIUSERCHATWINDOW_H

#include <QStandardItemModel>
#include <definations/messagedataroles.h>
#include <definations/messagehandlerorders.h>
#include <definations/multiuserdataroles.h>
#include <definations/namespaces.h>
#include <definations/actiongroups.h>
#include <definations/notificationdataroles.h>
#include <definations/soundfiles.h>
#include <definations/resources.h>
#include <definations/menuicons.h>
#include <definations/toolbargroups.h>
#include <interfaces/imultiuserchat.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/imessageprocessor.h>
#include <interfaces/imessagestyles.h>
#include <interfaces/imessagearchiver.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/irostersview.h>
#include <interfaces/isettings.h>
#include <interfaces/istatusicons.h>
#include <interfaces/istatuschanger.h>
#include <interfaces/iaccountmanager.h>
#include <interfaces/iroster.h>
#include <interfaces/ipresence.h>
#include <utils/widgetmanager.h>
#include "edituserslistdialog.h"
#include "usercontextmenu.h"
#include "inputtextdialog.h"
#include "usersproxymodel.h"
#include "ui_multiuserchatwindow.h"

#define GROUP_NOTIFICATOR_ID      "GroupChatMessages"
#define PRIVATE_NOTIFICATOR_ID    "PrivateMessages"
#define MENTION_NOTIFICATOR_ID    "GroupChatMention"

struct WindowStatus 
{
  QDateTime startTime;
  QDateTime createTime;
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
  virtual void showWindow();
  virtual void closeWindow();
  //IMessageHandler
  virtual bool checkMessage(const Message &AMessage);
  virtual void receiveMessage(int AMessageId);
  virtual void showMessage(int AMessageId);
  virtual bool openWindow(const Jid &AStreamJid, const Jid &AContactJid, Message::MessageType AType);
  virtual INotification notification(INotifications *ANotifications, const Message &AMessage);
  //IMultiUserChatWindow
  virtual Jid streamJid() const { return FMultiChat->streamJid(); }
  virtual Jid roomJid() const { return FMultiChat->roomJid(); }
  virtual bool isActive() const { return isVisible() && isActiveWindow(); }
  virtual IViewWidget *viewWidget() const { return FViewWidget; }
  virtual IEditWidget *editWidget() const { return FEditWidget; }
  virtual IMenuBarWidget *menuBarWidget() const { return FMenuBarWidget; }
  virtual IToolBarWidget *toolBarWidget() const { return FToolBarWidget; }
  virtual IStatusBarWidget *statusBarWidget() const { return FStatusBarWidget; }
  virtual IMultiUserChat *multiUserChat() const { return FMultiChat; }
  virtual IChatWindow *openChatWindow(const Jid &AContactJid); 
  virtual IChatWindow *findChatWindow(const Jid &AContactJid) const;
  virtual void contextMenuForUser(IMultiUser *AUser, Menu *AMenu);
  virtual void exitAndDestroy(const QString &AStatus, int AWaitClose = 5000);
signals:
  void windowShow();
  void windowClose();
  void windowActivated();
  void windowChanged();
  void windowClosed();
  void windowDestroyed();
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
  void showMessage(const QString &AMessage, int AContentType=0);
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
  void showChatWindow(IChatWindow *AWindow);
  void removeActiveChatMessages(IChatWindow *AWindow);
  void updateChatWindow(IChatWindow *AWindow);
protected:
  virtual bool event(QEvent *AEvent);
  virtual void showEvent(QShowEvent *AEvent);
  virtual void closeEvent(QCloseEvent *AEvent);
  virtual bool eventFilter(QObject *AObject, QEvent *AEvent);
protected slots:
  void onChatOpened();
  void onChatNotify(const QString &ANick, const QString &ANotify);
  void onChatError(const QString &ANick, const QString &AError);
  void onChatClosed();
  void onStreamJidChanged(const Jid &ABefour, const Jid &AAfter);
  //Occupant
  void onUserPresence(IMultiUser *AUser, int AShow, const QString &AStatus);
  void onUserDataChanged(IMultiUser *AUser, int ARole, const QVariant &ABefour, const QVariant &AAfter);
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
  void onAccountChanged(const QString &AName, const QVariant &AValue);  
private:
  Ui::MultiUserChatWindowClass ui;
private:
  IMessageWidgets *FMessageWidgets;
  IMessageProcessor *FMessageProcessor;
  IMessageStyles *FMessageStyles;
  IMessageArchiver *FMessageArchiver;
  IDataForms *FDataForms;
  ISettings *FSettings;
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
  bool FShownDetached;
  bool FDestroyOnChatClosed;
  QList<int> FActiveMessages;
  QList<IChatWindow *> FChatWindows;
  QMultiMap<IChatWindow *,int> FActiveChatMessages;
  QMap<int, IDataDialogWidget *> FDataFormMessages;
  QMap<IViewWidget *, WindowStatus> FWindowStatus;
private:
  UsersProxyModel *FUsersProxy;
  QStandardItemModel *FUsersModel;
  QHash<IMultiUser *, QStandardItem *> FUsers;
};

#endif // MULTIUSERCHATWINDOW_H
