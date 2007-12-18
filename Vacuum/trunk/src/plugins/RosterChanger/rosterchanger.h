#ifndef ROSTERCHANGER_H
#define ROSTERCHANGER_H

#include <QDateTime>
#include <QPointer>
#include "../../definations/initorders.h"
#include "../../definations/actiongroups.h"
#include "../../definations/rosterlabelorders.h"
#include "../../definations/rosterindextyperole.h"
#include "../../interfaces/irosterchanger.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/irostersmodel.h"
#include "../../interfaces/irostersview.h"
#include "../../interfaces/iroster.h"
#include "../../interfaces/imainwindow.h"
#include "../../interfaces/itraymanager.h"
#include "../../interfaces/iaccountmanager.h"
#include "../../utils/skin.h"
#include "addcontactdialog.h"
#include "subscriptiondialog.h"

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
  Menu *createGroupMenu(const QHash<int,QVariant> AData, const QSet<QString> &AExceptGroups, 
    bool ANewGroup, bool ARootGroup, const char *ASlot, Menu *AParent);
  void openSubsDialog(int ASubsId);
  void removeSubsMessage(int ASubsId);
protected slots:
  //Operations on subscription
  void onSendSubscription(bool);
  void onReceiveSubscription(IRoster *ARoster, const Jid &AFromJid, 
    IRoster::SubsType AType, const QString &AStatus);
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
  //Operations on stream
  void onAddContact(AddContactDialog *ADialog);
protected slots:
  void onShowAddContactDialog(bool);
  void onRosterOpened(IRoster *ARoster);
  void onRosterClosed(IRoster *ARoster);
  void onRostersViewContextMenu(IRosterIndex *AIndex, Menu *AMenu);
  void onRosterNotifyActivated(IRosterIndex *AIndex, int ANotifyId);
  void onTrayNotifyActivated(int ANotifyId, QSystemTrayIcon::ActivationReason AReason);
  void onSubsDialogShowNext();
  void onSubsDialogDestroyed(QObject *AObject);
  void onAddContactDialogDestroyed(QObject *AObject);
  void onAccountChanged(const QString &AName, const QVariant &AValue);
private:
  IRosterPlugin *FRosterPlugin;
  IRostersModelPlugin *FRostersModelPlugin;
  IRostersModel *FRostersModel;
  IRostersViewPlugin *FRostersViewPlugin;
  IRostersView *FRostersView;
  IMainWindowPlugin *FMainWindowPlugin;
  ITrayManager *FTrayManager;
  IAccountManager *FAccountManager;
private:
  Menu *FAddContactMenu;
  SkinIconset *FSystemIconset;
private:
  struct SubsItem 
  {
    int subsId;
    Jid streamJid;
    Jid contactJid;
    IRoster::SubsType type;
    QString status;
    QDateTime time;
    int trayId;
    int rosterId;
  };
  int FSubsId;
  QHash<int,SubsItem> FSubsItems;
private:
  QHash<IRoster *,Action *> FActions;
  SubscriptionDialog *FSubsDialog;
  QHash<IRoster *, QList<AddContactDialog *> > FAddContactDialogs;
};

#endif // ROSTERCHANGER_H
