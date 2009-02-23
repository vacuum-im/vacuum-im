#ifndef ROSTERCHANGER_H
#define ROSTERCHANGER_H

#include <QPointer>
#include <QDateTime>
#include "../../definations/actiongroups.h"
#include "../../definations/rosterlabelorders.h"
#include "../../definations/rosterindextyperole.h"
#include "../../definations/multiuserdataroles.h"
#include "../../definations/accountvaluenames.h"
#include "../../definations/notificationdataroles.h"
#include "../../definations/optionnodes.h"
#include "../../definations/optionwidgetorders.h"
#include "../../definations/resources.h"
#include "../../definations/menuicons.h"
#include "../../definations/soundfiles.h"
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
#include "../../interfaces/isettings.h"
#include "addcontactdialog.h"
#include "subscriptiondialog.h"
#include "subscriptionoptions.h"

struct AutoSubscription {
  AutoSubscription() {
    silent = false;
    autoOptions = 0;
  }
  bool silent;
  int autoOptions;
};

class RosterChanger : 
  public QObject,
  public IPlugin,
  public IRosterChanger,
  public IOptionsHolder
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IRosterChanger IOptionsHolder);
public:
  RosterChanger();
  ~RosterChanger();
  virtual QObject *instance() { return this; }
  //IPlugin
  virtual QUuid pluginUuid() const { return ROSTERCHANGER_UUID; }
  virtual void pluginInfo(IPluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects();
  virtual bool initSettings() { return true; }
  virtual bool startPlugin() { return true; }
  //IOptionsHolder
  virtual QWidget *optionsWidget(const QString &ANode, int &AOrder);
  //IRosterChanger
  virtual bool isAutoSubscribe(const Jid &AStreamJid, const Jid &AContactJid) const;
  virtual bool isAutoUnsubscribe(const Jid &AStreamJid, const Jid &AContactJid) const;
  virtual bool isSilentSubsctiption(const Jid &AStreamJid, const Jid &AContactJid) const;
  virtual void insertAutoSubscribe(const Jid &AStreamJid, const Jid &AContactJid, int AAutoOptions, bool ASilently);
  virtual void removeAutoSubscribe(const Jid &AStreamJid, const Jid &AContactJid);
  virtual void subscribeContact(const Jid &AStreamJid, const Jid &AContactJid, const QString &AMessage = "", bool ASilently = false);
  virtual void unsubscribeContact(const Jid &AStreamJid, const Jid &AContactJid, const QString &AMessage = "", bool ASilently = false);
  virtual bool checkOption(IRosterChanger::Option AOption) const;
  virtual void setOption(IRosterChanger::Option AOption, bool AValue);
  virtual IAddContactDialog *showAddContactDialog(const Jid &AStreamJid);
signals:
  virtual void addContactDialogCreated(IAddContactDialog *ADialog);
  virtual void subscriptionDialogCreated(ISubscriptionDialog *ADialog);
  virtual void optionChanged(IRosterChanger::Option AOption, bool AValue);
  virtual void optionsAccepted();
  virtual void optionsRejected();
protected:
  QString subscriptionNotify(int ASubsType, const Jid &AContactJid) const;
  Menu *createGroupMenu(const QHash<int,QVariant> &AData, const QSet<QString> &AExceptGroups, 
    bool ANewGroup, bool ARootGroup, const char *ASlot, Menu *AParent);
  SubscriptionDialog *subscriptionDialog(const Jid &AStreamJid, const Jid &AContactJid) const;
  SubscriptionDialog *createSubscriptionDialog(const Jid &AStreamJid, const Jid &AContactJid, const QString &ANotify, const QString &AMessage);
protected slots:
  //Operations on subscription
  void onContactSubscription(bool);
  void onSendSubscription(bool);
  void onReceiveSubscription(IRoster *ARoster, const Jid &AContactJid, int ASubsType, const QString &AMessage);
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
protected slots:
  void onSettingsOpened();
  void onSettingsClosed();
  void onShowAddContactDialog(bool);
  void onRosterOpened(IRoster *ARoster);
  void onRosterClosed(IRoster *ARoster);
  void onRostersViewContextMenu(IRosterIndex *AIndex, Menu *AMenu);
  void onNotificationActivated(int ANotifyId);
  void onSubscriptionDialogDestroyed();
  void onAccountChanged(const QString &AName, const QVariant &AValue);
  void onMultiUserContextMenu(IMultiUserChatWindow *AWindow, IMultiUser *AUser, Menu *AMenu);
private:
  IPluginManager *FPluginManager;
  IRosterPlugin *FRosterPlugin;
  IRostersModel *FRostersModel;
  IRostersView *FRostersView;
  IMainWindowPlugin *FMainWindowPlugin;
  ITrayManager *FTrayManager;
  INotifications *FNotifications;
  IAccountManager *FAccountManager;
  IMultiUserChatPlugin *FMultiUserChatPlugin;
  ISettingsPlugin *FSettingsPlugin;
private:
  int FOptions;
  Menu *FAddContactMenu;
  QHash<IRoster *,Action *> FActions;
  QHash<SubscriptionDialog *,int> FSubscrDialogs;
  QHash<Jid, QHash<Jid, AutoSubscription> > FAutoSubscriptions;
};

#endif // ROSTERCHANGER_H
