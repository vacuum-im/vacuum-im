#ifndef IMULTIUSERCHAT_H
#define IMULTIUSERCHAT_H

#include "../../interfaces/imessenger.h"
#include "../../interfaces/idataforms.h"
#include "../../utils/jid.h"
#include "../../utils/message.h"

#define MULTIUSERCHAT_UUID              "{EB960F92-59A9-4322-A646-F9AB4913706C}"

#define MUC_ROLE_NONE                   "none"
#define MUC_ROLE_VISITOR                "visitor"
#define MUC_ROLE_PARTICIPANT            "participant"
#define MUC_ROLE_MODERATOR              "moderator"

#define MUC_AFFIL_NONE                  "none"
#define MUC_AFFIL_OUTCAST               "outcast"
#define MUC_AFFIL_MEMBER                "member"
#define MUC_AFFIL_ADMIN                 "admin"
#define MUC_AFFIL_OWNER                 "owner"

#define MUC_SC_NON_ANONYMOUS            100
#define MUC_SC_AFFIL_CHANGED            101
#define MUC_SC_CONFIG_CHANGED           104
#define MUC_SC_NOW_LOGGING_ENABLED      170
#define MUC_SC_NOW_LOGGING_DISABLED     171
#define MUC_SC_NOW_NON_ANONYMOUS        172
#define MUC_SC_NOW_SEMI_ANONYMOUS       173
#define MUC_SC_NOW_FULLY_ANONYMOUS      174
#define MUC_SC_ROOM_CREATED             201
#define MUC_SC_ROOM_ENTER               210
#define MUC_SC_USER_BANNED              301
#define MUC_SC_NICK_CHANGED             303
#define MUC_SC_USER_KICKED              307
#define MUC_SC_AFFIL_CHANGE             321
#define MUC_SC_MEMBERS_ONLY             322
#define MUC_SC_SYSTEM_SHUTDOWN          332

#define MUC_FT_REQUEST                  "http://jabber.org/protocol/muc#request"
#define MUC_FT_ROOM_GONFIG              "http://jabber.org/protocol/muc#roomconfig"

#define MUC_FV_ROLE                     "muc#role"

struct IMultiUserListItem {
  Jid userJid;
  QString role;
  QString affiliation;
  QString notes;
};

class IMultiUser 
{
public:
  virtual QObject *instance() = 0;
  virtual Jid roomJid() const =0;
  virtual Jid contactJid() const =0;
  virtual QString nickName() const =0;
  virtual QString role() const =0;
  virtual QString affiliation() const =0;
  virtual QVariant data(int ARole) const =0;
  virtual void setData(int ARole, const QVariant &AValue) =0;
signals:
  virtual void dataChanged(int ARole, const QVariant &ABefour, const QVariant &AAfter) =0;
};

class IMultiUserChat 
{
public:
  virtual QObject *instance() = 0;
  virtual Jid streamJid() const =0;
  virtual Jid roomJid() const =0;
  virtual bool isOpen() const =0;
  virtual bool isChangingState() const =0;
  virtual bool autoPresence() const =0;
  virtual void setAutoPresence(bool AAuto) =0;
  virtual QList<int> statusCodes() const =0;
  virtual IMultiUser *mainUser() const =0;
  virtual IMultiUser *userByNick(const QString &ANick) const =0;
  virtual QList<IMultiUser *> allUsers() const =0;
  //Occupant
  virtual QString nickName() const =0;
  virtual void setNickName(const QString &ANick) =0;
  virtual QString password() const =0;
  virtual void setPassword(const QString &APassword) =0;
  virtual int show() const =0;
  virtual QString status() const =0;
  virtual void setPresence(int AShow, const QString &AStatus) =0;
  virtual bool sendMessage(const Message &AMessage, const QString &AToNick = "") =0;
  virtual void requestVoice() =0;
  //Moderator
  virtual QString subject() const =0;
  virtual void setSubject(const QString &ASubject) =0;
  virtual void submitDataFormMessage(IDataForm *AForm) =0;
  //Administrator
  virtual void setRole(const QString &ANick, const QString &ARole, const QString &AReason = "") =0;
  virtual void setAffiliation(const QString &ANick, const QString &AAffiliation, const QString &AReason = "") =0;
  virtual bool requestAffiliationList(const QString &AAffiliation) =0;
  virtual bool setAffiliationList(const QList<IMultiUserListItem> &ADeltaList) =0;
  //Owner
  virtual bool requestConfigForm() =0;
  virtual bool submitConfigForm(IDataForm *AForm) =0;
signals:
  virtual void chatOpened() =0;
  virtual void chatNotify(const QString &ANick, const QString &ANotify) =0;
  virtual void chatError(const QString &ANick, const QString &AError) =0;
  virtual void chatClosed() =0;
  virtual void chatDestroyed() =0;
  virtual void streamJidChanged(const Jid &ABefour, const Jid &AAfter) =0;
  //Occupant
  virtual void userPresence(IMultiUser *AUser, int AShow, const QString &AStatus) =0;
  virtual void userDataChanged(IMultiUser *AUser, int ARole, const QVariant &ABefour, const QVariant &AAfter) =0;
  virtual void userNickChanged(IMultiUser *AUser, const QString &AOldNick, const QString &ANewNick) =0;
  virtual void presenceChanged(int AShow, const QString &AStatus) =0;
  virtual void serviceMessageReceived(const Message &AMessage) =0;
  virtual void messageReceive(const QString &ANick, Message &AMessage) =0;
  virtual void messageReceived(const QString &ANick, const Message &AMessage) =0;
  virtual void messageSend(Message &AMessage) =0;
  virtual void messageSent(const Message &AMessage) =0;
  //Moderator
  virtual void topicChanged(const QString &ATopic) =0;
  virtual void userKicked(const QString &ANick, const QString &AReason, const QString &AByUser) =0;
  virtual void dataFormMessageReceived(const Message &AMessage) =0;
  virtual void dataFormMessageSubmited(IDataForm *AForm) =0;
  //Administrator
  virtual void userBanned(const QString &ANick, const QString &AReason, const QString &AByUser) =0;
  virtual void affiliationListReceived(const QString &AAffiliation, const QList<IMultiUserListItem> &AList) =0;
  //Owner
  virtual void configFormReceived(const QDomElement &AForm) =0;
  virtual void configFormSubmited(IDataForm *AForm) =0;
  virtual void configFormAccepted() =0;
  virtual void configFormRejected(const QString &AError) =0;
};

class IMultiUserChatWindow : 
  public QMainWindow,
  public ITabWidget
{
public:
  virtual Jid streamJid() const =0;
  virtual Jid roomJid() const =0;
  virtual bool isActive() const =0;
  virtual IViewWidget *viewWidget() const =0;
  virtual IEditWidget *editWidget() const =0;
  virtual IToolBarWidget *toolBarWidget() const =0;
  virtual IMultiUserChat *multiUserChat() const =0;
  virtual IChatWindow *openChatWindow(const Jid &AContactJid) =0; 
  virtual IChatWindow *findChatWindow(const Jid &AContactJid) const =0;
  virtual void exitMultiUserChat() =0;
signals:
  virtual void windowActivated() =0;
  virtual void windowClosed() =0;
  virtual void chatWindowCreated(IChatWindow *AWindow) =0;
  virtual void chatWindowDestroyed(IChatWindow *AWindow) =0;
  virtual void multiUserContextMenu(IMultiUser *AUser, Menu *AMenu) =0;
};

class IMultiUserChatPlugin 
{
public:
  virtual QObject *instance() = 0;
  virtual IPluginManager *pluginManager() const =0;
  virtual bool requestRoomNick(const Jid &AStreamJid, const Jid &ARoomJid) =0;
  virtual IMultiUserChat *getMultiUserChat(const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANick, 
    const QString &APassword, bool ADedicated = false) =0;
  virtual QList<IMultiUserChat *> multiUserChats() const =0;
  virtual IMultiUserChat *multiUserChat(const Jid &AStreamJid, const Jid &ARoomJid) const =0;
  virtual IMultiUserChatWindow *getMultiChatWindow(const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANick, 
    const QString &APassword) =0;
  virtual QList<IMultiUserChatWindow *> multiChatWindows() const =0;
  virtual IMultiUserChatWindow *multiChatWindow(const Jid &AStreamJid, const Jid &ARoomJid) const =0;
  virtual void showJoinMultiChatDialog(const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANick, const QString &APassword) =0;
signals:
  virtual void roomNickReceived(const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANick) =0;
  virtual void multiUserChatCreated(IMultiUserChat *AMultiChat) =0;
  virtual void multiUserChatDestroyed(IMultiUserChat *AMultiChat) =0;
  virtual void multiChatWindowCreated(IMultiUserChatWindow *AWindow) =0;
  virtual void multiChatWindowDestroyed(IMultiUserChatWindow *AWindow) =0;
  virtual void multiUserContextMenu(IMultiUserChatWindow *AWindow, IMultiUser *AUser, Menu *AMenu) =0;
};

Q_DECLARE_INTERFACE(IMultiUser,"Vacuum.Plugin.IMultiUser/1.0")
Q_DECLARE_INTERFACE(IMultiUserChat,"Vacuum.Plugin.IMultiUserChat/1.0")
Q_DECLARE_INTERFACE(IMultiUserChatWindow,"Vacuum.Plugin.IMultiUserChatWindow/1.0")
Q_DECLARE_INTERFACE(IMultiUserChatPlugin,"Vacuum.Plugin.IMultiUserChatPlugin/1.0")

#endif