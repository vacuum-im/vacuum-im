#ifndef NOTIFICATIONS_H
#define NOTIFICATIONS_H

#include "../../definations/notificationdataroles.h"
#include "../../definations/soundnames.h"
#include "../../definations/actiongroups.h"
#include "../../definations/optionnodes.h"
#include "../../definations/optionorders.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/inotifications.h"
#include "../../interfaces/irostersview.h"
#include "../../interfaces/itraymanager.h"
#include "../../interfaces/iroster.h"
#include "../../interfaces/iavatars.h"
#include "../../interfaces/istatusicons.h"
#include "../../interfaces/isettings.h"
#include "../../utils/action.h"
#include "notifywidget.h"
#include "optionswidget.h"

struct NotifyRecord {
  NotifyRecord() {
    rosterId=0; 
    trayId=0; 
    widget=NULL; 
    action=NULL;
  }
  int rosterId;
  int trayId;
  NotifyWidget *widget;
  Action *action;
  INotification notification;
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
  virtual void pluginInfo(PluginInfo *APluginInfo);
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
  //Notification Utilities
  virtual QImage contactAvatar(const Jid &AContactJid) const;
  virtual QIcon contactIcon(const Jid &AStreamJid, const Jid &AContactJid) const;
  virtual QString contactName(const Jid &AStreamJId, const Jid &AContactJid) const;
signals:
  virtual void notificationAppended(int ANotifyId);
  virtual void notificationActivated(int ANotifyId);
  virtual void notificationRemoved(int ANotifyId);
  virtual void optionChanged(INotifications::Option AOption, bool AValue);
  virtual void optionsAccepted();
  virtual void optionsRejected();
protected:
  int notifyIdByRosterId(int ARosterId) const;
  int notifyIdByTrayId(int ATrayId) const;
  int notifyIdByWidget(NotifyWidget *AWidget) const;
protected slots:
  void onRosterNotifyActivated(IRosterIndex *AIndex, int ANotifyId);
  void onRosterNotifyRemoved(IRosterIndex *AIndex, int ANotifyId);
  void onTrayNotifyActivated(int ANotifyId, QSystemTrayIcon::ActivationReason AReason);
  void onTrayNotifyRemoved(int ANotifyId);
  void onWindowNotifyActivated();
  void onWindowNotifyRemoved();
  void onActionNotifyActivated(bool);
  void onSettingsOpened();
  void onSettingsClosed();
  void onOptionsDialogAccepted();
  void onOptionsDialogRejected();
  void onOptionsDialogClosed();
private:
  IAvatars *FAvatars;
  IRosterPlugin *FRosterPlugin;
  IStatusIcons *FStatusIcons;
  ITrayManager *FTrayManager;
  IRostersModel *FRostersModel;
  IRostersViewPlugin *FRostersViewPlugin;
  ISettingsPlugin *FSettingsPlugin;
private:
  uint FOptions;
  int FNotifyId;
  OptionsWidget *FOptionsWidget;
  QHash<int,NotifyRecord> FNotifyRecords;
};

#endif // NOTIFICATIONS_H
