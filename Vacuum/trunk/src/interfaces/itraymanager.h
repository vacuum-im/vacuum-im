#ifndef ITRAYMANAGER_H
#define ITRAYMANAGER_H

#include <QSystemTrayIcon>
#include "../../utils/menu.h"

#define TRAYMANAGER_UUID "{DF738BB8-B22B-4307-9EF8-F5833D7D2204}"

class ITrayManager {
public:
  virtual QObject *instance() =0;
  virtual QIcon icon() const =0;
  virtual QString toolTip() const =0;
  virtual QIcon baseIcon() const =0;
  virtual void setBaseIcon(const QIcon &AIcon) =0;
  virtual void addAction(Action *AAction, int AGroup = DEFAULT_ACTION_GROUP, bool ASort = false) =0;
  virtual void removeAction(Action *AAction) =0;
  virtual int appendNotify(const QIcon &AIcon, const QString &AToolTip, bool ABlink) =0;
  virtual void updateNotify(int ANotifyId, const QIcon &AIcon, const QString &AToolTip, bool ABlink) =0;
  virtual void removeNotify(int ANotifyId) =0;
signals:
  virtual void activated(QSystemTrayIcon::ActivationReason AReason) =0;
  virtual void notifyAdded(int ANotifyId) =0;
  virtual void notifyActivated(int ANotifyId) =0;
  virtual void notifyRemoved(int ANotifyId) =0;
};

Q_DECLARE_INTERFACE(ITrayManager,"Vacuum.Plugin.ITrayManager/1.0")

#endif