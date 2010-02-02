#ifndef NOTIFICATIONS_H
#define NOTIFICATIONS_H

#include <QTimer>
#include <QSound>
#include <definations/notificationdataroles.h>
#include <definations/actiongroups.h>
#include <definations/optionnodes.h>
#include <definations/optionnodeorders.h>
#include <definations/optionwidgetorders.h>
#include <definations/resources.h>
#include <definations/menuicons.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/inotifications.h>
#include <interfaces/irostersview.h>
#include <interfaces/itraymanager.h>
#include <interfaces/iroster.h>
#include <interfaces/iavatars.h>
#include <interfaces/istatusicons.h>
#include <interfaces/isettings.h>
#include <utils/action.h>
#include "notifywidget.h"
#include "optionswidget.h"
#include "notifykindswidget.h"

struct NotifyRecord {
  NotifyRecord() {
    trayId=0; 
    rosterId=0; 
    widget=NULL; 
    action=NULL;
  }
  int trayId;
  int rosterId;
  Action *action;
  NotifyWidget *widget;
  INotification notification;
};

struct Notificator {
  QString title;
  uchar kinds;
  uchar defaults;
  uchar kindMask;
};

class Notifications : 
  public QObject,
  public IPlugin,
  public INotifications,
  public IOptionsHolder
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin INotifications IOptionsHolder);
public:
  Notifications();
  ~Notifications();
  //IPlugin
  virtual QObject *instance() { return this; }
  virtual QUuid pluginUuid() const { return NOTIFICATIONS_UUID; }
  virtual void pluginInfo(IPluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects();
  virtual bool initSettings() { return true; }
  virtual bool startPlugin() { return true; }
  //IOptionsHolder
  virtual QWidget *optionsWidget(const QString &ANode, int &AOrder);
  //INotifications
  virtual QList<int> notifications() const;
  virtual INotification notificationById(int ANotifyId) const;
  virtual int appendNotification(const INotification &ANotification);
  virtual void activateNotification(int ANotifyId);
  virtual void removeNotification(int ANotifyId);
  virtual bool checkOption(INotifications::Option AOption) const;
  virtual void setOption(INotifications::Option AOption, bool AValue);
  //Kind options for notificators
  virtual void insertNotificator(const QString &AId, const QString &ATitle, uchar AKindMask, uchar ADefault);
  virtual uchar notificatorKinds(const QString &AId) const;
  virtual void setNotificatorKinds(const QString &AId, uchar AKinds);
  virtual void removeNotificator(const QString &AId);
  //Notification Utilities
  virtual QImage contactAvatar(const Jid &AContactJid) const;
  virtual QIcon contactIcon(const Jid &AStreamJid, const Jid &AContactJid) const;
  virtual QString contactName(const Jid &AStreamJId, const Jid &AContactJid) const;
signals:
  void notificationActivated(int ANotifyId);
  void notificationRemoved(int ANotifyId);
  void notificationAppend(int ANotifyId, INotification &ANotification);
  void notificationAppended(int ANotifyId, const INotification &ANotification);
  void optionChanged(INotifications::Option AOption, bool AValue);
  //IOptionsHolder
  void optionsAccepted();
  void optionsRejected();
protected:
  int notifyIdByRosterId(int ARosterId) const;
  int notifyIdByTrayId(int ATrayId) const;
  int notifyIdByWidget(NotifyWidget *AWidget) const;
protected slots:
  void onActivateDelayedActivations();
  void onTrayActionTriggered(bool);
  void onRosterNotifyActivated(IRosterIndex *AIndex, int ANotifyId);
  void onRosterNotifyRemoved(IRosterIndex *AIndex, int ANotifyId);
  void onTrayNotifyActivated(int ANotifyId, QSystemTrayIcon::ActivationReason AReason);
  void onTrayNotifyRemoved(int ANotifyId);
  void onWindowNotifyActivated();
  void onWindowNotifyRemoved();
  void onWindowNotifyDestroyed();
  void onActionNotifyActivated(bool);
  void onSettingsOpened();
  void onSettingsClosed();
private:
  IAvatars *FAvatars;
  IRosterPlugin *FRosterPlugin;
  IStatusIcons *FStatusIcons;
  ITrayManager *FTrayManager;
  IRostersModel *FRostersModel;
  IRostersViewPlugin *FRostersViewPlugin;
  ISettingsPlugin *FSettingsPlugin;
private:
  Action *FActivateAll;
  Action *FRemoveAll;
  Menu *FNotifyMenu;
private:
  uint FOptions;
  int FNotifyId;
  QSound *FSound;
  QList<int> FDelayedActivations;
  QMap<int, NotifyRecord> FNotifyRecords;
  mutable QMap<QString, Notificator> FNotificators;
};

#endif // NOTIFICATIONS_H
