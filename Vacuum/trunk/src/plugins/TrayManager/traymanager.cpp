#include "traymanager.h"

#include <QContextMenuEvent>

int TrayManager::FNextNotifyId = 10;

TrayManager::TrayManager()
{
  FContextMenu = new Menu;
  FTrayIcon.setContextMenu(FContextMenu);

  FCurNotifyId = 0;
  connect(&FTrayIcon,SIGNAL(activated(QSystemTrayIcon::ActivationReason)),SLOT(onActivated(QSystemTrayIcon::ActivationReason)));
}

TrayManager::~TrayManager()
{
  delete FContextMenu;
}

void TrayManager::pluginInfo(PluginInfo *APluginInfo)
{
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->description = tr("Managing tray icon and events");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = tr("Tray manager"); 
  APluginInfo->uid = TRAYMANAGER_UUID;
  APluginInfo->version = "0.1";
}

bool TrayManager::startPlugin()
{
  FTrayIcon.show();
  return true;
}

//ITrayManager
void TrayManager::setBaseIcon(const QIcon &AIcon)
{
  FBaseIcon = AIcon;
  FTrayIcon.setIcon(AIcon);
}

int TrayManager::appendNotify(const QIcon &AIcon, const QString &AToolTip)
{
  int notifyId = FNextNotifyId++;
  NotifyItem *notify = new NotifyItem;
  notify->icon = AIcon;
  notify->toolTip = AToolTip;
  FCurNotifyId = notifyId;
  FNotifyItems.insert(notifyId,notify);
  FTrayIcon.setIcon(AIcon);
  FTrayIcon.setToolTip(AToolTip);
  emit notifyAdded(notifyId);
  return notifyId;
}

void TrayManager::updateNotify(int ANotifyId, const QIcon &AIcon, const QString &AToolTip)
{
  NotifyItem *notify = FNotifyItems.value(ANotifyId,NULL);
  if (notify)
  {
    notify->icon = AIcon;
    notify->toolTip = AToolTip;
    if (FCurNotifyId = ANotifyId)
    {
      FTrayIcon.setIcon(AIcon);
      FTrayIcon.setToolTip(AToolTip);
    }
  }
}

void TrayManager::removeNotify(int ANotifyId)
{
  NotifyItem *notify = FNotifyItems.value(ANotifyId,NULL);
  if (notify)
  {
    delete notify;
    FNotifyItems.remove(ANotifyId);
    if (FCurNotifyId = ANotifyId)
    {
      if (FNotifyItems.isEmpty())
      {
        FCurNotifyId = 0;
        FTrayIcon.setIcon(FBaseIcon);
        FTrayIcon.setToolTip("");
      }
      else
      {
        FCurNotifyId = FNotifyItems.keys().last();
        notify = FNotifyItems.value(FCurNotifyId);
        FTrayIcon.setIcon(notify->icon);
        FTrayIcon.setToolTip(notify->toolTip);
      }
    }
    emit notifyRemoved(ANotifyId);
  }
}

void TrayManager::onActivated(QSystemTrayIcon::ActivationReason AReason)
{
  if (AReason == QSystemTrayIcon::Trigger)
  {
    int notifyId = 0;
    if (!FNotifyItems.isEmpty())
      notifyId = FNotifyItems.keys().last();
    emit notifyActivated(notifyId);
    removeNotify(notifyId);
  }
  else if (AReason == QSystemTrayIcon::Context)
  {
    FContextMenu->clear();
    emit contextMenu(FCurNotifyId,FContextMenu);
  }
}

Q_EXPORT_PLUGIN2(TrayManagerPlugin, TrayManager)
