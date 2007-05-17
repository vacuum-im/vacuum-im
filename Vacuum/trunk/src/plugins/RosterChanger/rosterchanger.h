#ifndef ROSTERCHANGER_H
#define ROSTERCHANGER_H

#include "../../definations/initorders.h"
#include "../../definations/actiongroups.h"
#include "../../interfaces/irosterchanger.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/irostersview.h"
#include "../../interfaces/iroster.h"
#include "../../interfaces/imainwindow.h"
#include "addcontactdialog.h"


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
    const QString &AGroup, const QString &ARequest) const;
  virtual Menu *addContactMenu() const { return FAddContactMenu; }
public slots:
  virtual void showAddContactDialogByAction(bool);
protected:
  Menu *createGroupMenu(const QHash<int,QVariant> AData, const QSet<QString> &AExceptGroups, 
    bool ANewGroup, bool ARootGroup, const char *ASlot, Menu *AParent);
protected slots:
  void onRostersViewContextMenu(const QModelIndex &AIndex, Menu *AMenu);
  //Operations on subscription
  void onSendSubscription(bool);
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
  void onRosterOpened(IRoster *ARoster);
  void onRosterClosed(IRoster *ARoster);
private:
  IRosterPlugin *FRosterPlugin;
  IRostersViewPlugin *FRostersViewPlugin;
  IMainWindowPlugin *FMainWindowPlugin;
private:
  Menu *FAddContactMenu;
  QHash<IRoster *,Action *> FActions;
};

#endif // ROSTERCHANGER_H
