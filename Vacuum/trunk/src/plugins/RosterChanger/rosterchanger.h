#ifndef ROSTERCHANGER_H
#define ROSTERCHANGER_H

#include <QDateTime>
#include <QPointer>
#include "../../definations/actiongroups.h"
#include "../../definations/rosterlabelorders.h"
#include "../../definations/rosterindextyperole.h"
#include "../../definations/multiuserdataroles.h"
#include "../../definations/accountvaluenames.h"
#include "../../definations/notificationdataroles.h"
#include "../../interfaces/irosterchanger.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/irostersmodel.h"
#include "../../interfaces/irostersview.h"
#include "../../interfaces/iroster.h"
#include "../../interfaces/imainwindow.h"
#include "../../interfaces/itraymanager.h"
#include "../../interfaces/iaccountmanager.h"
#include "../../interfaces/imultiuserchat.h"
#include "../../interfaces/inotifications.h"
#include "../../utils/skin.h"
#include "addcontactdialog.h"
#include "subscriptiondialog.h"

struct SubscriptionItem 
{
  int subsId;
  Jid streamJid;
  Jid contactJid;
  int type;
  QString status;
  QDateTime time;
  int notifyId;
};

class RosterChanger : 
  public QObject,
  public IPlugin,
  public IRosterChanger
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IRosterChanger);
public:
  RosterChanger();
  ~RosterChanger();
  virtual QObject *instance() { return this; }
  //IPlugin
  virtual QUuid pluginUuid() const { return ROSTERCHANGER_UUID; }
  virtual void pluginInfo(PluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/);
  virtual bool initObjects();
  virtual bool initSettings() { return true; }
  virtual bool startPlugin() { return true; }
  //IRosterChanger
  virtual void showAddContactDialog(const Jid &AStreamJid, const Jid &AJid, const QString &ANick,
    const QString &AGroup, const QString &ARequest);
signals:
  virtual void subscriptionDialogCreated(ISubscriptionDialog *ADialog);
  virtual void subscriptionDialogDestroyed(ISubscriptionDialog *ADialog);
protected:
  QString subscriptionNotify(int ASubsType, const Jid &AContactJid) const;
  Menu *createGroupMenu(const QHash<int,QVariant> AData, const QSet<QString> &AExceptGroups, 
    bool ANewGroup, bool ARootGroup, const char *ASlot, Menu *AParent);
  void openSubscriptionDialog(int ASubsId);
  void removeSubscriptionMessage(int ASubsId);
protected slots:
  //Operations on subscription
  void onSendSubscription(bool);
  void onReceiveSubscription(IRoster *ARoster, const Jid &AFromJid, int ASubsType, const QString &AText);
  //Operations on items
  void onRenameItem(bool);
  void onCopyItemToGroup(bool);
  void onMoveItemToGroup(bool);
  void onRemoveItemFromGroup(bool);
  void onRemoveItemFromRoster(bool);
  //Operations on group
  void onRenameGroup(bool);
  void onCopyGroupToGroup(bool);
  void onMoveGroupToGroup(bool);
  void onRemoveGroup(bool);
  void onRemoveGroupItems(bool);
  //Operations on stream
  void onAddContact(AddContactDialog *ADialog);
protected slots:
  void onShowAddContactDialog(bool);
  void onRosterOpened(IRoster *ARoster);
  void onRosterClosed(IRoster *ARoster);
  void onRostersViewContextMenu(IRosterIndex *AIndex, Menu *AMenu);
  void onNotificationActivated(int ANotifyId);
  void onSubscriptionDialogShowNext();
  void onSubscriptionDialogDestroyed(QObject *AObject);
  void onAddContactDialogDestroyed(QObject *AObject);
  void onAccountChanged(const QString &AName, const QVariant &AValue);
  void onMultiUserContextMenu(IMultiUserChatWindow *AWindow, IMultiUser *AUser, Menu *AMenu);
private:
  IRosterPlugin *FRosterPlugin;
  IRostersModel *FRostersModel;
  IRostersViewPlugin *FRostersViewPlugin;
  IRostersView *FRostersView;
  IMainWindowPlugin *FMainWindowPlugin;
  ITrayManager *FTrayManager;
  INotifications *FNotifications;
  IAccountManager *FAccountManager;
  IMultiUserChatPlugin *FMultiUserChatPlugin;
private:
  Menu *FAddContactMenu;
private:
  int FSubsId;
  SubscriptionDialog *FSubscriptionDialog;
  QMap<int, SubscriptionItem> FSubsItems;
private:
  QHash<IRoster *,Action *> FActions;
  QHash<IRoster *, QList<AddContactDialog *> > FAddContactDialogs;
};

#endif // ROSTERCHANGER_H
