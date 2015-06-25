#ifndef IMULTIUSERCHAT_H
#define IMULTIUSERCHAT_H

#include <QMenuBar>
#include <QTreeView>
#include <interfaces/idataforms.h>
#include <interfaces/irostersmodel.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/ipresencemanager.h>
#include <utils/advanceditemdelegate.h>
#include <utils/advanceditemmodel.h>
#include <utils/menubarchanger.h>
#include <utils/xmpperror.h>
#include <utils/message.h>
#include <utils/menu.h>
#include <utils/jid.h>

#define MULTIUSERCHAT_UUID "{EB960F92-59A9-4322-A646-F9AB4913706C}"

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
#define MUC_SC_MEMBERS_SHOW             102
#define MUC_SC_MEMBERS_HIDE             103
#define MUC_SC_CONFIG_CHANGED           104
#define MUC_SC_SELF_PRESENCE            110
#define MUC_SC_NOW_LOGGING_ENABLED      170
#define MUC_SC_NOW_LOGGING_DISABLED     171
#define MUC_SC_NOW_NON_ANONYMOUS        172
#define MUC_SC_NOW_SEMI_ANONYMOUS       173
#define MUC_SC_ROOM_CREATED             201
#define MUC_SC_NICK_ASSIGNED            210
#define MUC_SC_USER_BANNED              301
#define MUC_SC_NICK_CHANGED             303
#define MUC_SC_USER_KICKED              307
#define MUC_SC_AFFIL_CHANGE             321
#define MUC_SC_MEMBERS_ONLY             322
#define MUC_SC_SYSTEM_SHUTDOWN          332

#define MUC_FT_REQUEST                  "http://jabber.org/protocol/muc#request"
#define MUC_FT_ROOM_GONFIG              "http://jabber.org/protocol/muc#roomconfig"
#define MUC_FT_ROOM_INFO                "http://jabber.org/protocol/muc#roominfo"
#define MUC_FV_ROLE                     "muc#role"

#define MUC_NODE_ROOM_NICK              "x-roomuser-item"
#define MUC_NODE_ROOMS                  "http://jabber.org/protocol/muc#rooms"
#define MUC_NODE_TRAFFIC                "http://jabber.org/protocol/muc#traffic"

#define MUC_HIDDEN                      "muc_hidden"
#define MUC_MEMBERSONLY                 "muc_membersonly"
#define MUC_MODERATED                   "muc_moderated"
#define MUC_NONANONYMOUS                "muc_nonanonymous"
#define MUC_OPEN                        "muc_open"
#define MUC_PASSWORD                    "muc_password"
#define MUC_PASSWORDPROTECTED           "muc_passwordprotected"
#define MUC_PERSISTENT                  "muc_persistent"
#define MUC_PUBLIC                      "muc_public"
#define MUC_ROOMS                       "muc_rooms"
#define MUC_SEMIANONYMOUS               "muc_semianonymous"
#define MUC_TEMPORARY                   "muc_temporary"
#define MUC_UNMODERATED                 "muc_unmoderated"
#define MUC_UNSECURED                   "muc_unsecured"

struct IMultiUserListItem 
{
	QString jid;
	QString role;
	QString affiliation;
	QString notes;
};

struct IMultiUserChatHistory
{
	IMultiUserChatHistory() {
		empty = false;
		maxChars = 0;
		maxStanzas = 0;
		seconds = 0;
	}
	bool empty;
	quint32 maxChars;
	quint32 maxStanzas;
	quint32 seconds;
	QDateTime since;
};

struct IMultiUserViewNotify
{
	enum Flags {
		Blink          = 0x01,
	};
	IMultiUserViewNotify() {
		order = -1;
		flags = 0;
	}
	int order;
	int flags;
	QIcon icon;
	QString footer;
};


class IMultiUser
{
public:
	virtual QObject *instance() = 0;
	virtual Jid streamJid() const =0;
	virtual Jid roomJid() const =0;
	virtual Jid userJid() const =0;
	virtual Jid realJid() const =0;
	virtual QString nick() const =0;
	virtual QString role() const =0;
	virtual QString affiliation() const =0;
	virtual IPresenceItem presence() const =0;
protected:
	virtual void changed(int AData, const QVariant &ABefore) =0;
};

class IMultiUserChat
{
public:
	virtual QObject *instance() = 0;
	virtual Jid streamJid() const =0;
	virtual Jid roomJid() const =0;
	virtual QString roomName() const =0;
	virtual QString roomShortName() const =0;
	virtual bool isOpen() const =0;
	virtual bool isConnected() const =0;
	virtual bool autoPresence() const =0;
	virtual void setAutoPresence(bool AAuto) =0;
	virtual QList<int> statusCodes() const =0;
	virtual bool isUserPresent(const Jid &AContactJid) const =0;
	virtual IMultiUser *mainUser() const =0;
	virtual QList<IMultiUser *> allUsers() const =0;
	virtual IMultiUser *findUser(const QString &ANick) const =0;
	//Occupant
	virtual QString nickName() const =0;
	virtual bool setNickName(const QString &ANick) =0;
	virtual QString password() const =0;
	virtual void setPassword(const QString &APassword) =0;
	virtual IMultiUserChatHistory history() const =0;
	virtual void setHistory(const IMultiUserChatHistory &AHistory) =0;
	virtual XmppError roomError() const =0;
	virtual IPresenceItem roomPresence() const =0;
	virtual bool sendStreamPresence() =0;
	virtual bool sendPresence(int AShow, const QString &AStatus, int APriority) =0;
	virtual bool sendMessage(const Message &AMessage, const QString &AToNick = QString::null) =0;
	virtual bool inviteContact(const Jid &AContactJid, const QString &AReason) =0;
	virtual bool requestVoice() =0;
	//Moderator
	virtual QString subject() const =0;
	virtual bool sendSubject(const QString &ASubject) =0;
	virtual bool sendDataFormMessage(const IDataForm &AForm) =0;
	//Administrator
	virtual bool setRole(const QString &ANick, const QString &ARole, const QString &AReason = QString::null) =0;
	virtual bool setAffiliation(const QString &ANick, const QString &AAffiliation, const QString &AReason = QString::null) =0;
	virtual bool requestAffiliationList(const QString &AAffiliation) =0;
	virtual bool changeAffiliationList(const QList<IMultiUserListItem> &ADeltaList) =0;
	//Owner
	virtual bool requestConfigForm() =0;
	virtual bool sendConfigForm(const IDataForm &AForm) =0;
	virtual bool destroyRoom(const QString &AReason) =0;
protected:
	virtual void chatAboutToConnect() =0;
	virtual void chatOpened() =0;
	virtual void chatNotify(const QString &ANotify) =0;
	virtual void chatError(const QString &AMessage) =0;
	virtual void chatAboutToDisconnect() =0;
	virtual void chatClosed() =0;
	virtual void chatDestroyed() =0;
	virtual void roomNameChanged(const QString &AName) =0;
	virtual void streamJidChanged(const Jid &ABefore, const Jid &AAfter) =0;
	//Occupant
	virtual void presenceChanged(const IPresenceItem &APresence) =0;
	virtual void messageSent(const Message &AMessage) =0;
	virtual void serviceMessageReceived(const Message &AMessage) =0;
	virtual void messageReceived(const QString &ANick, const Message &AMessage) =0;
	virtual void inviteDeclined(const Jid &AContactJid, const QString &AReason) =0;
	virtual void userChanged(IMultiUser *AUser, int AData, const QVariant &ABefore) =0;
	//Moderator
	virtual void subjectChanged(const QString &ANick, const QString &ASubject) =0;
	virtual void userKicked(const QString &ANick, const QString &AReason, const QString &AByUser) =0;
	virtual void dataFormMessageReceived(const Message &AMessage) =0;
	virtual void dataFormMessageSent(const IDataForm &AForm) =0;
	//Administrator
	virtual void userBanned(const QString &ANick, const QString &AReason, const QString &AByUser) =0;
	virtual void affiliationListReceived(const QString &AAffiliation, const QList<IMultiUserListItem> &AList) =0;
	virtual void affiliationListChanged(const QList<IMultiUserListItem> &ADeltaList) =0;
	//Owner
	virtual void configFormReceived(const IDataForm &AForm) =0;
	virtual void configFormSent(const IDataForm &AForm) =0;
	virtual void configFormAccepted() =0;
	virtual void configFormRejected(const XmppError &AError) =0;
	virtual void roomDestroyed(const QString &AReason) =0;
};

class IMultiUserView
{
public:
	enum ViewMode {
		ViewFull,
		ViewSimple,
		ViewCompact
	};
public:
	virtual QTreeView *instance() =0;
	virtual int viewMode() const =0;
	virtual void setViewMode(int AMode) =0;
	// Items
	virtual AdvancedItemModel *model() const =0;
	virtual AdvancedItemDelegate *itemDelegate() const =0;
	virtual IMultiUser *findItemUser(const QStandardItem *AItem) const =0;
	virtual QStandardItem *findUserItem(const IMultiUser *AUser) const =0;
	virtual QModelIndex indexFromItem(const QStandardItem *AItem) const =0;
	virtual QStandardItem *itemFromIndex(const QModelIndex &AIndex) const =0;
	// Labels
	virtual AdvancedDelegateItem generalLabel(quint32 ALabelId) const =0;
	virtual void insertGeneralLabel(const AdvancedDelegateItem &ALabel) =0;
	virtual void removeGeneralLabel(quint32 ALabelId) =0;
	virtual void insertItemLabel(const AdvancedDelegateItem &ALabel, QStandardItem *AItem) =0;
	virtual void removeItemLabel(quint32 ALabelId, QStandardItem *AItem = NULL) =0;
	virtual QRect labelRect(quint32 ALabeld, const QModelIndex &AIndex) const =0;
	virtual quint32 labelAt(const QPoint &APoint, const QModelIndex &AIndex) const =0;
	// Notify
	virtual QStandardItem *notifyItem(int ANotifyId) const =0;
	virtual QList<int> itemNotifies(QStandardItem *AItem) const =0;
	virtual IMultiUserViewNotify itemNotify(int ANotifyId) const =0;
	virtual int insertItemNotify(const IMultiUserViewNotify &ANotify, QStandardItem *AItem) =0;
	virtual void activateItemNotify(int ANotifyId) =0;
	virtual void removeItemNotify(int ANotifyId) =0;
	// Context
	virtual void contextMenuForItem(QStandardItem *AItem, Menu *AMenu) =0;
	virtual void toolTipsForItem(QStandardItem *AItem, QMap<int,QString> &AToolTips) =0;
protected:
	virtual void viewModeChanged(int AMode) =0;
	// Notify
	virtual void itemNotifyInserted(int ANotifyId) =0;
	virtual void itemNotifyActivated(int ANotifyId) =0;
	virtual void itemNotifyRemoved(int ANotfyId) =0;
	// Context
	virtual void itemContextMenu(QStandardItem *AItem, Menu *AMenu) =0;
	virtual void itemToolTips(QStandardItem *AItem, QMap<int,QString> &AToolTips) =0;
};

class IMultiUserChatWindow :
	public IMessageWindow
{
public:
	virtual QMainWindow *instance() =0;
	virtual IMultiUserChat *multiUserChat() const =0;
	virtual IMultiUserView *multiUserView() const =0;
	virtual SplitterWidget *viewWidgetsBox() const =0;
	virtual SplitterWidget *usersWidgetsBox() const =0;
	virtual SplitterWidget *centralWidgetsBox() const =0;
	virtual IMessageChatWindow *openPrivateChatWindow(const Jid &AContactJid) =0;
	virtual IMessageChatWindow *findPrivateChatWindow(const Jid &AContactJid) const =0;
	virtual void contextMenuForRoom(Menu *AMenu) =0;
	virtual void contextMenuForUser(IMultiUser *AUser, Menu *AMenu) =0;
	virtual void toolTipsForUser(IMultiUser *AUser, QMap<int,QString> &AToolTips) =0;
	virtual void exitAndDestroy(const QString &AStatus, int AWaitClose = 15000) =0;
protected:
	virtual void multiChatContextMenu(Menu *AMenu) =0;
	virtual void multiUserContextMenu(IMultiUser *AUser, Menu *AMenu) =0;
	virtual void multiUserToolTips(IMultiUser *AUser, QMap<int,QString> &AToolTips) =0;
	virtual void privateChatWindowCreated(IMessageChatWindow *AWindow) =0;
	virtual void privateChatWindowDestroyed(IMessageChatWindow *AWindow) =0;
};

class IMultiUserChatManager
{
public:
	virtual QObject *instance() = 0;
	virtual QList<IMultiUserChat *> multiUserChats() const =0;
	virtual IMultiUserChat *findMultiUserChat(const Jid &AStreamJid, const Jid &ARoomJid) const =0;
	virtual IMultiUserChat *getMultiUserChat(const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANick, const QString &APassword) =0;
	virtual QList<IMultiUserChatWindow *> multiChatWindows() const =0;
	virtual IMultiUserChatWindow *findMultiChatWindow(const Jid &AStreamJid, const Jid &ARoomJid) const =0;
	virtual IMultiUserChatWindow *getMultiChatWindow(const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANick, const QString &APassword) =0;
	virtual QList<IRosterIndex *> multiChatRosterIndexes() const =0;
	virtual IRosterIndex *findMultiChatRosterIndex(const Jid &AStreamJid, const Jid &ARoomJid) const =0;
	virtual IRosterIndex *getMultiChatRosterIndex(const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANick, const QString &APassword) =0;
	virtual void showJoinMultiChatDialog(const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANick, const QString &APassword) =0;
	virtual bool requestRoomNick(const Jid &AStreamJid, const Jid &ARoomJid) =0;
protected:
	virtual void multiUserChatCreated(IMultiUserChat *AMultiChat) =0;
	virtual void multiUserChatDestroyed(IMultiUserChat *AMultiChat) =0;
	virtual void multiChatWindowCreated(IMultiUserChatWindow *AWindow) =0;
	virtual void multiChatWindowDestroyed(IMultiUserChatWindow *AWindow) =0;
	virtual void multiChatRosterIndexCreated(IRosterIndex *AIndex) =0;
	virtual void multiChatRosterIndexDestroyed(IRosterIndex *AIndex) =0;
	virtual void multiChatContextMenu(IMultiUserChatWindow *AWindow, Menu *AMenu) =0;
	virtual void multiUserContextMenu(IMultiUserChatWindow *AWindow, IMultiUser *AUser, Menu *AMenu) =0;
	virtual void multiUserToolTips(IMultiUserChatWindow *AWindow, IMultiUser *AUser, QMap<int,QString> &AToolTips) =0;
	virtual void roomNickReceived(const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANick) =0;
};

Q_DECLARE_INTERFACE(IMultiUser,"Vacuum.Plugin.IMultiUser/1.0")
Q_DECLARE_INTERFACE(IMultiUserChat,"Vacuum.Plugin.IMultiUserChat/1.6")
Q_DECLARE_INTERFACE(IMultiUserView,"Vacuum.Plugin.IMultiUserView/1.0")
Q_DECLARE_INTERFACE(IMultiUserChatWindow,"Vacuum.Plugin.IMultiUserChatWindow/1.3")
Q_DECLARE_INTERFACE(IMultiUserChatManager,"Vacuum.Plugin.IMultiUserChatManager/1.8")

#endif //IMULTIUSERCHAT_H
