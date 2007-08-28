#ifndef TRAYMANAGER_H
#define TRAYMANAGER_H

#include <QMap>
#include <QTimer>
#include "../../definations/actiongroups.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/itraymanager.h"

class TrayManager : 
  public QObject,
  public IPlugin,
  public ITrayManager
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin ITrayManager);

public:
  TrayManager();
  ~TrayManager();

  virtual QObject *instance() { return this; }

  //IPlugin
  virtual QUuid pluginUuid() const { return TRAYMANAGER_UUID; }
  virtual void pluginInfo(PluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects() { return true; }
  virtual bool initSettings() { return true; }
  virtual bool startPlugin();

  //ITrayManager
  virtual QIcon icon() const { return FTrayIcon.icon(); }
  virtual QString toolTip() const { return FTrayIcon.toolTip(); }
  virtual QIcon mainIcon() const { return FBaseIcon; }
  virtual void setMainIcon(const QIcon &AIcon);
  virtual void addAction(Action *AAction, int AGroup = AG_DEFAULT, bool ASort = false);
  virtual void removeAction(Action *AAction);
  virtual int appendNotify(const QIcon &AIcon, const QString &AToolTip, bool ABlink);
  virtual void updateNotify(int ANotifyId, const QIcon &AIcon, const QString &AToolTip, bool ABlink);
  virtual void removeNotify(int ANotifyId);
signals:
  virtual void activated(QSystemTrayIcon::ActivationReason AReason);
  virtual void notifyAdded(int ANotifyId);
  virtual void notifyActivated(int ANotifyId);
  virtual void notifyRemoved(int ANotifyId);
protected:
  void setTrayIcon(const QIcon &AIcon, const QString &AToolTip, bool ABlink);
protected slots:
  void onActivated(QSystemTrayIcon::ActivationReason AReason);
  void onBlinkTimer();
private:
  Menu *mnuContext;
  Action *actQuit;
private:
  QTimer FBlinkTimer;
  QSystemTrayIcon FTrayIcon;
private:
  static int FNextNotifyId;
  int FCurNotifyId;
  QIcon FBaseIcon;
  struct NotifyItem {
    QIcon icon;
    QString toolTip;
    bool blink;
  };
  QMap<int,NotifyItem *> FNotifyItems;
  QIcon FCurIcon;
  bool FBlinkShow;
};

#endif // TRAYMANAGER_H
