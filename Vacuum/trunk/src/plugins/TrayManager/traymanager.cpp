#include "traymanager.h"

#include <QContextMenuEvent>

int TrayManager::FNextNotifyId = 1;

TrayManager::TrayManager()
{
  FCurNotifyId = 0;
  mnuContext = new Menu;
  FTrayIcon.setContextMenu(mnuContext);

  FBlinkTimer.setInterval(500);
  connect(&FBlinkTimer,SIGNAL(timeout()),SLOT(onBlinkTimer()));

  connect(&FTrayIcon,SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
    SLOT(onActivated(QSystemTrayIcon::ActivationReason)));
  connect(&FTrayIcon,SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
    SIGNAL(activated(QSystemTrayIcon::ActivationReason)));
}

TrayManager::~TrayManager()
{
  delete mnuContext;
}

void TrayManager::pluginInfo(PluginInfo *APluginInfo)
{
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->description = tr("Managing tray icon");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = tr("Tray manager"); 
  APluginInfo->uid = TRAYMANAGER_UUID;
  APluginInfo->version = "0.1";
}

bool TrayManager::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  actQuit = new Action(mnuContext);
  actQuit->setIcon(SYSTEM_ICONSETFILE,"psi/quit");
  actQuit->setText(tr("Quit"));
  connect(actQuit,SIGNAL(triggered()),APluginManager->instance(),SLOT(quit()));
  addAction(actQuit,MAINWINDOW_ACTION_GROUP_QUIT);
  
  return FTrayIcon.isSystemTrayAvailable();
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
  if (FCurNotifyId == 0)
    setTrayIcon(AIcon,"",false);
}

void TrayManager::addAction(Action *AAction, int AGroup /*= DEFAULT_ACTION_GROUP*/, bool ASort /*= false*/)
{
  mnuContext->addAction(AAction,AGroup,ASort);
}

void TrayManager::removeAction(Action *AAction)
{
  mnuContext->removeAction(AAction);
}

int TrayManager::appendNotify(const QIcon &AIcon, const QString &AToolTip, bool ABlink)
{
  int notifyId = FNextNotifyId++;
  FCurNotifyId = notifyId;
  NotifyItem *notify = new NotifyItem;
  notify->icon = AIcon;
  notify->toolTip = AToolTip;
  notify->blink = ABlink;
  FNotifyItems.insert(notifyId,notify);
  setTrayIcon(AIcon,AToolTip,ABlink);
  emit notifyAdded(notifyId);
  return notifyId;
}

void TrayManager::updateNotify(int ANotifyId, const QIcon &AIcon, const QString &AToolTip, bool ABlink)
{
  NotifyItem *notify = FNotifyItems.value(ANotifyId,NULL);
  if (notify)
  {
    notify->icon = AIcon;
    notify->toolTip = AToolTip;
    notify->blink = ABlink;
    if (FCurNotifyId = ANotifyId)
      setTrayIcon(AIcon,AToolTip,ABlink);
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
        setTrayIcon(FBaseIcon,"",false);
      }
      else
      {
        FCurNotifyId = FNotifyItems.keys().last();
        notify = FNotifyItems.value(FCurNotifyId);
        setTrayIcon(notify->icon,notify->toolTip,notify->blink);
      }
    }
    emit notifyRemoved(ANotifyId);
  }
}

void TrayManager::setTrayIcon(const QIcon &AIcon, const QString &AToolTip, bool ABlink)
{
  FCurIcon = AIcon;
  FBlinkShow = true;
  if (ABlink)
    FBlinkTimer.start();
  else
    FBlinkTimer.stop();
  FTrayIcon.setIcon(AIcon);
  FTrayIcon.setToolTip(AToolTip);
}

void TrayManager::onActivated(QSystemTrayIcon::ActivationReason AReason)
{
  if (AReason == QSystemTrayIcon::DoubleClick)
  {
    int notifyId = 0;
    if (!FNotifyItems.isEmpty())
      notifyId = FNotifyItems.keys().last();
    emit notifyActivated(notifyId);
    removeNotify(notifyId);
  }
}

void TrayManager::onBlinkTimer()
{
  if (FBlinkShow)
    FTrayIcon.setIcon(QIcon());
  else
    FTrayIcon.setIcon(FCurIcon);
  FBlinkShow = !FBlinkShow;
}

Q_EXPORT_PLUGIN2(TrayManagerPlugin, TrayManager)
