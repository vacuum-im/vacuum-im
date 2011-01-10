#include "traymanager.h"

#include <QContextMenuEvent>

TrayManager::TrayManager()
{
	FNextNotifyId = 1;
	FCurNotifyId = 0;
	FContextMenu = new Menu;
	FTrayIcon.setContextMenu(FContextMenu);

	FBlinkTimer.setInterval(500);
	connect(&FBlinkTimer,SIGNAL(timeout()),SLOT(onBlinkTimer()));
	connect(&FTrayIcon,SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
		SLOT(onActivated(QSystemTrayIcon::ActivationReason)));
	connect(&FTrayIcon,SIGNAL(messageClicked()), SIGNAL(messageClicked()));
}

TrayManager::~TrayManager()
{
	delete FContextMenu;
}

void TrayManager::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Tray Icon");
	APluginInfo->description = tr("Allows other modules to access the icon and context menu in the tray");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
}

bool TrayManager::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
	FQuitAction = new Action(FContextMenu);
	FQuitAction->setIcon(RSR_STORAGE_MENUICONS,MNI_MAINWINDOW_QUIT);
	FQuitAction->setText(tr("Quit"));
	connect(FQuitAction,SIGNAL(triggered()),APluginManager->instance(),SLOT(quit()));
	addAction(FQuitAction,AG_TMTM_TRAYMANAGER);

	return true;
}

bool TrayManager::startPlugin()
{
	FTrayIcon.show();
	return true;
}

//ITrayManager
QIcon TrayManager::currentIcon() const
{
	return FCurIcon;
}

QString TrayManager::currentToolTip() const
{
	return FTrayIcon.toolTip();
}

int TrayManager::currentNotify() const
{
	return FCurNotifyId;
}

QIcon TrayManager::mainIcon() const
{
	return FMainIcon;
}

void TrayManager::setMainIcon(const QIcon &AIcon)
{
	FMainIcon = AIcon;
	if (FCurNotifyId == 0)
		setTrayIcon(FMainIcon,FMainToolTip,false);
}

QString TrayManager::mainToolTip() const
{
	return FTrayIcon.toolTip();
}

void TrayManager::setMainToolTip(const QString &AToolTip)
{
	FMainToolTip = AToolTip;
	if (FCurNotifyId == 0)
		setTrayIcon(FMainIcon,FMainToolTip,false);
}

bool TrayManager::isTrayIconVisible() const
{
	return FTrayIcon.isVisible();
}

void TrayManager::setTrayIconVisible(bool AVisible)
{
	FTrayIcon.setVisible(AVisible);
}

void TrayManager::showMessage(const QString &ATitle, const QString &AMessage, QSystemTrayIcon::MessageIcon AIcon, int ATimeout)
{
	FTrayIcon.showMessage(ATitle,AMessage,AIcon,ATimeout);
	emit messageShown(ATitle,AMessage,AIcon,ATimeout);
}

void TrayManager::addAction(Action *AAction, int AGroup, bool ASort)
{
	FContextMenu->addAction(AAction,AGroup,ASort);
}

void TrayManager::removeAction(Action *AAction)
{
	FContextMenu->removeAction(AAction);
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
		if (FCurNotifyId == ANotifyId)
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
		if (FCurNotifyId == ANotifyId)
		{
			if (FNotifyItems.isEmpty())
			{
				FCurNotifyId = 0;
				setTrayIcon(FMainIcon,FMainToolTip,false);
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
	emit notifyActivated(FCurNotifyId,AReason);
}

void TrayManager::onBlinkTimer()
{
	if (FBlinkShow)
		FTrayIcon.setIcon(QIcon());
	else
		FTrayIcon.setIcon(FCurIcon);
	FBlinkShow = !FBlinkShow;
}

Q_EXPORT_PLUGIN2(plg_traymanager, TrayManager)
