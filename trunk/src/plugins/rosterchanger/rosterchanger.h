#ifndef ROSTERCHANGER_H
#define ROSTERCHANGER_H

#include <QDateTime>
#include <definitions/actiongroups.h>
#include <definitions/rosterlabelorders.h>
#include <definitions/rosterindextyperole.h>
#include <definitions/rosternotifyorders.h>
#include <definitions/rosteredithandlerorders.h>
#include <definitions/rosterdragdropmimetypes.h>
#include <definitions/multiuserdataroles.h>
#include <definitions/notificationtypes.h>
#include <definitions/notificationdataroles.h>
#include <definitions/notificationtypeorders.h>
#include <definitions/optionvalues.h>
#include <definitions/optionnodes.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/soundfiles.h>
#include <definitions/shortcuts.h>
#include <definitions/xmppurihandlerorders.h>
#include <interfaces/irosterchanger.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/irostersmodel.h>
#include <interfaces/irostersview.h>
#include <interfaces/iroster.h>
#include <interfaces/imultiuserchat.h>
#include <interfaces/inotifications.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/ixmppuriqueries.h>
#include <utils/shortcuts.h>
#include <utils/widgetmanager.h>
#include "addcontactdialog.h"
#include "subscriptiondialog.h"

struct AutoSubscription {
	AutoSubscription() {
		silent = false;
		autoSubscribe = false;
		autoUnsubscribe = false;
	}
	bool silent;
	bool autoSubscribe;
	bool autoUnsubscribe;
};

class RosterChanger :
			public QObject,
			public IPlugin,
			public IRosterChanger,
			public IOptionsHolder,
			public IRostersDragDropHandler,
			public IRostersEditHandler,
			public IXmppUriHandler
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IRosterChanger IOptionsHolder IRostersDragDropHandler IRostersEditHandler IXmppUriHandler);
public:
	RosterChanger();
	~RosterChanger();
	virtual QObject *instance() { return this; }
	//IPlugin
	virtual QUuid pluginUuid() const { return ROSTERCHANGER_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin() { return true; }
	//IOptionsHolder
	virtual QMultiMap<int, IOptionsWidget *> optionsWidgets(const QString &ANodeId, QWidget *AParent);
	//IRostersDragDropHandler
	virtual Qt::DropActions rosterDragStart(const QMouseEvent *AEvent, const QModelIndex &AIndex, QDrag *ADrag);
	virtual bool rosterDragEnter(const QDragEnterEvent *AEvent);
	virtual bool rosterDragMove(const QDragMoveEvent *AEvent, const QModelIndex &AHover);
	virtual void rosterDragLeave(const QDragLeaveEvent *AEvent);
	virtual bool rosterDropAction(const QDropEvent *AEvent, const QModelIndex &AIndex, Menu *AMenu);
	//IRostersEditHandler
	virtual bool rosterEditStart(int ADataRole, const QModelIndex &AIndex) const;
	virtual QWidget *rosterEditEditor(int ADataRole, QWidget *AParent, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
	virtual void rosterEditLoadData(int ADataRole, QWidget *AEditor, const QModelIndex &AIndex) const;
	virtual void rosterEditSaveData(int ADataRole, QWidget *AEditor, const QModelIndex &AIndex) const;
	virtual void rosterEditGeometry(int ADataRole, QWidget *AEditor, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
	//IXmppUriHandler
	virtual bool xmppUriOpen(const Jid &AStreamJid, const Jid &AContactJid, const QString &AAction, const QMultiMap<QString, QString> &AParams);
	//IRosterChanger
	virtual bool isAutoSubscribe(const Jid &AStreamJid, const Jid &AContactJid) const;
	virtual bool isAutoUnsubscribe(const Jid &AStreamJid, const Jid &AContactJid) const;
	virtual bool isSilentSubsctiption(const Jid &AStreamJid, const Jid &AContactJid) const;
	virtual void insertAutoSubscribe(const Jid &AStreamJid, const Jid &AContactJid, bool ASilently, bool ASubscr, bool AUnsubscr);
	virtual void removeAutoSubscribe(const Jid &AStreamJid, const Jid &AContactJid);
	virtual void subscribeContact(const Jid &AStreamJid, const Jid &AContactJid, const QString &AMessage = "", bool ASilently = false);
	virtual void unsubscribeContact(const Jid &AStreamJid, const Jid &AContactJid, const QString &AMessage = "", bool ASilently = false);
	virtual IAddContactDialog *showAddContactDialog(const Jid &AStreamJid);
signals:
	void addContactDialogCreated(IAddContactDialog *ADialog);
	void subscriptionDialogCreated(ISubscriptionDialog *ADialog);
protected:
	bool isSelectionAccepted(const QList<IRosterIndex *> &ASelected) const;
	QString subscriptionNotify(int ASubsType, const Jid &AContactJid) const;
	Menu *createGroupMenu(const QHash<int,QVariant> &AData, const QSet<QString> &AExceptGroups,bool ANewGroup, bool ARootGroup, bool ABlank, const char *ASlot, Menu *AParent);
	SubscriptionDialog *findSubscriptionDialog(const Jid &AStreamJid, const Jid &AContactJid) const;
	SubscriptionDialog *createSubscriptionDialog(const Jid &AStreamJid, const Jid &AContactJid, const QString &ANotify, const QString &AMessage);
	QStringList indexesRoleList(const QList<IRosterIndex *> &AIndexes, int ARole, bool AUnique) const;
protected:
	void contactsSubscription(const Jid &AStreamJid, const QStringList &AContacts, int ASubsc);
	void sendSubscription(const Jid &AStreamJid, const QStringList &AContacts, int ASubsc) const;
	void addContactToGroup(const Jid &AStreamJid, const Jid &AContactJid, const QString &AGroup) const;
	void renameContact(const Jid &AStreamJid, const Jid &AContactJid, const QString &AOldName) const;
	void copyContactsToGroup(const Jid &AStreamJid, const QStringList &AContacts, const QString &AGroup) const;
	void moveContactsToGroup(const Jid &AStreamJid, const QStringList &AContacts, const QStringList &AGroupsFrom, const QString &AGroupTo) const;
	void removeContactsFromGroups(const Jid &AStreamJid, const QStringList &AContacts, const QStringList &AGroups) const;
	void removeContactsFromRoster(const Jid &AStreamJid, const QStringList &AContacts) const;
	void addGroupToGroup(const Jid &AToStreamJid, const Jid &AFromStreamJid, const QString &AGroup, const QString &AGroupTo) const;
	void renameGroup(const Jid &AStreamJid, const QString &AGroup) const;
	void copyGroupsToGroup(const Jid &AStreamJid, const QStringList &AGroups, const QString &AGroupTo) const;
	void moveGroupsToGroup(const Jid &AStreamJid, const QStringList &AGroups, const QString &AGroupTo) const;
	void removeGroups(const Jid &AStreamJid, const QStringList &AGroups) const;
	void removeGroupsContacts(const Jid &AStreamJid, const QStringList &AGroups) const;
protected:
	bool eventFilter(QObject *AObject, QEvent *AEvent);
protected slots:
	void onReceiveSubscription(IRoster *ARoster, const Jid &AContactJid, int ASubsType, const QString &AMessage);
	//Operations on subscription
	void onContactsSubscription(bool);
	void onSendSubscription(bool);
	//Operations on contacts
	void onAddContactToGroup(bool);
	void onRenameContact(bool);
	void onCopyContactsToGroup(bool);
	void onMoveContactsToGroup(bool);
	void onRemoveContactsFromGroups(bool);
	void onRemoveContactsFromRoster(bool);
	//Operations on groups
	void onAddGroupToGroup(bool);
	void onRenameGroup(bool);
	void onCopyGroupsToGroup(bool);
	void onMoveGroupsToGroup(bool);
	void onRemoveGroups(bool);
	void onRemoveGroupsContacts(bool);
protected slots:
	void onShowAddContactDialog(bool);
	void onRosterItemRemoved(IRoster *ARoster, const IRosterItem &ARosterItem);
	void onRosterClosed(IRoster *ARoster);
	void onShortcutActivated(const QString &AId, QWidget *AWidget);
	void onRosterIndexMultiSelection(const QList<IRosterIndex *> &ASelected, bool &AAccepted);
	void onRosterIndexContextMenu(const QList<IRosterIndex *> &AIndexes, int ALabelId, Menu *AMenu);
	void onMultiUserContextMenu(IMultiUserChatWindow *AWindow, IMultiUser *AUser, Menu *AMenu);
	void onNotificationActivated(int ANotifyId);
	void onNotificationRemoved(int ANotifyId);
	void onSubscriptionDialogDestroyed();
private:
	IPluginManager *FPluginManager;
	IRosterPlugin *FRosterPlugin;
	IRostersModel *FRostersModel;
	IRostersView *FRostersView;
	INotifications *FNotifications;
	IOptionsManager *FOptionsManager;
	IXmppUriQueries *FXmppUriQueries;
	IMultiUserChatPlugin *FMultiUserChatPlugin;
private:
	QMap<int, SubscriptionDialog *> FNotifyDialog;
	QMap<Jid, QMap<Jid, AutoSubscription> > FAutoSubscriptions;
};

#endif // ROSTERCHANGER_H
