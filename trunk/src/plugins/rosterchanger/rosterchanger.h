#ifndef ROSTERCHANGER_H
#define ROSTERCHANGER_H

#include <QDateTime>
#include <definations/actiongroups.h>
#include <definations/rosterlabelorders.h>
#include <definations/rosterindextyperole.h>
#include <definations/multiuserdataroles.h>
#include <definations/notificationdataroles.h>
#include <definations/rosterdragdropmimetypes.h>
#include <definations/optionnodes.h>
#include <definations/optionwidgetorders.h>
#include <definations/resources.h>
#include <definations/menuicons.h>
#include <definations/soundfiles.h>
#include <definations/xmppurihandlerorders.h>
#include <interfaces/irosterchanger.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/irostersmodel.h>
#include <interfaces/irostersview.h>
#include <interfaces/iroster.h>
#include <interfaces/imultiuserchat.h>
#include <interfaces/inotifications.h>
#include <interfaces/isettings.h>
#include <interfaces/ixmppuriqueries.h>
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
  public IOptionsHolder,
  public IRostersDragDropHandler,
  public IXmppUriHandler
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IRosterChanger IOptionsHolder IRostersDragDropHandler IXmppUriHandler);
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
  //IRostersDragDropHandler
  virtual Qt::DropActions rosterDragStart(const QMouseEvent *AEvent, const QModelIndex &AIndex, QDrag *ADrag);
  virtual bool rosterDragEnter(const QDragEnterEvent *AEvent);
  virtual bool rosterDragMove(const QDragMoveEvent *AEvent, const QModelIndex &AHover);
  virtual void rosterDragLeave(const QDragLeaveEvent *AEvent);
  virtual bool rosterDropAction(const QDropEvent *AEvent, const QModelIndex &AIndex, Menu *AMenu);
  //IXmppUriHandler
  virtual bool xmppUriOpen(const Jid &AStreamJid, const Jid &AContactJid, const QString &AAction, const QMultiMap<QString, QString> &AParams);
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
  void addContactDialogCreated(IAddContactDialog *ADialog);
  void subscriptionDialogCreated(ISubscriptionDialog *ADialog);
  void optionChanged(IRosterChanger::Option AOption, bool AValue);
  void optionsAccepted();
  void optionsRejected();
protected:
  QString subscriptionNotify(int ASubsType, const Jid &AContactJid) const;
  Menu *createGroupMenu(const QHash<int,QVariant> &AData, const QSet<QString> &AExceptGroups, 
    bool ANewGroup, bool ARootGroup, const char *ASlot, Menu *AParent);
  SubscriptionDialog *findSubscriptionDialog(const Jid &AStreamJid, const Jid &AContactJid) const;
  SubscriptionDialog *createSubscriptionDialog(const Jid &AStreamJid, const Jid &AContactJid, const QString &ANotify, const QString &AMessage);
protected slots:
  //Operations on subscription
  void onContactSubscription(bool);
  void onSendSubscription(bool);
  void onReceiveSubscription(IRoster *ARoster, const Jid &AContactJid, int ASubsType, const QString &AMessage);
  //Operations on items
  void onAddItemToGroup(bool);
  void onRenameItem(bool);
  void onCopyItemToGroup(bool);
  void onMoveItemToGroup(bool);
  void onRemoveItemFromGroup(bool);
  void onRemoveItemFromRoster(bool);
  //Operations on group
  void onAddGroupToGroup(bool);
  void onRenameGroup(bool);
  void onCopyGroupToGroup(bool);
  void onMoveGroupToGroup(bool);
  void onRemoveGroup(bool);
  void onRemoveGroupItems(bool);
protected slots:
  void onSettingsOpened();
  void onSettingsClosed();
  void onShowAddContactDialog(bool);
  void onRosterClosed(IRoster *ARoster);
  void onRosterIndexContextMenu(IRosterIndex *AIndex, Menu *AMenu);
  void onNotificationActivated(int ANotifyId);
  void onNotificationRemoved(int ANotifyId);
  void onSubscriptionDialogDestroyed();
  void onMultiUserContextMenu(IMultiUserChatWindow *AWindow, IMultiUser *AUser, Menu *AMenu);
private:
  IPluginManager *FPluginManager;
  IRosterPlugin *FRosterPlugin;
  IRostersModel *FRostersModel;
  IRostersView *FRostersView;
  INotifications *FNotifications;
  ISettingsPlugin *FSettingsPlugin;
  IXmppUriQueries *FXmppUriQueries;
  IMultiUserChatPlugin *FMultiUserChatPlugin;
private:
  int FOptions;
  QMap<int, SubscriptionDialog *> FNotifyDialog;
  QMap<Jid, QMap<Jid, AutoSubscription> > FAutoSubscriptions;
};

#endif // ROSTERCHANGER_H
