#ifndef INOTIFICATIONS_H
#define INOTIFICATIONS_H

#include <QIcon>
#include <QImage>
#include "../utils/jid.h"

#define NOTIFICATIONS_UUID  "{59887A91-A483-4a7c-A2DE-227A01D6BC5E}"

struct INotification {
  enum NotifyKinds {
    RosterIcon    = 0x01,
    PopupWindow   = 0x02,
    TrayIcon      = 0x04,
    TrayAction    = 0x08,
    PlaySound     = 0x10,
    AutoActivate  = 0x20
  };
  INotification() { kinds = 0; }
  uchar kinds;
  QHash<int, QVariant> data;
};

class INotifications {
public:
  enum Option {
    EnableRosterIcons     = 0x01,
    EnablePopupWindows    = 0x02,
    EnableTrayIcons       = 0x04,
    EnableTrayActions     = 0x08,
    EnableSounds          = 0x10,
    EnableAutoActivate    = 0x20,
    ExpandRosterGroups    = 0x40
  };
public:
  virtual QObject *instance() =0;
  virtual QList<int> notifications() const =0;
  virtual INotification notificationById(int ANotifyId) const =0;
  virtual int appendNotification(const INotification &ANotification) =0;
  virtual void activateNotification(int ANotifyId) =0;
  virtual void removeNotification(int ANotifyId) =0;
  virtual bool checkOption(INotifications::Option AOption) const =0;
  virtual void setOption(INotifications::Option AOption, bool AValue) =0;
  virtual void insertNotificator(const QString &AId, const QString &ATitle, uchar AKindMask, uchar ADefault) =0;
  virtual uchar notificatorKinds(const QString &AId) const =0;
  virtual void setNotificatorKinds(const QString &AId, uchar AKinds) =0;
  virtual void removeNotificator(const QString &AId) =0;
  virtual QImage contactAvatar(const Jid &AContactJid) const =0;
  virtual QIcon contactIcon(const Jid &AStreamJid, const Jid &AContactJid) const =0;
  virtual QString contactName(const Jid &AStreamJId, const Jid &AContactJid) const =0;
protected:
  virtual void notificationActivated(int ANotifyId) =0;
  virtual void notificationRemoved(int ANotifyId) =0;
  virtual void notificationAppend(int ANotifyId, INotification &ANotification) =0;
  virtual void notificationAppended(int ANotifyId, const INotification &ANotification) =0;
  virtual void optionChanged(INotifications::Option AOption, bool AValue) =0;
};

Q_DECLARE_INTERFACE(INotifications,"Vacuum.Plugin.INotifications/1.0")

#endif
