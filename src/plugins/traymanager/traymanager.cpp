#include "traymanager.h"

#include <QApplication>

#define BLINK_VISIBLE_TIME      500
#define BLINK_INVISIBLE_TIME    500

TrayManager::TrayManager()
{
	FPluginManager = NULL;

	FActiveNotify = -1;
	FIconHidden = false;

	FContextMenu = new Menu;
	FSystemIcon.setContextMenu(FContextMenu);

	FBlinkTimer.setSingleShot(true);
	connect(&FBlinkTimer,SIGNAL(timeout()),SLOT(onBlinkTimerTimeout()));

	connect(&FSystemIcon,SIGNAL(messageClicked()), SIGNAL(messageClicked()));
	connect(&FSystemIcon,SIGNAL(activated(QSystemTrayIcon::ActivationReason)), SLOT(onTrayIconActivated(QSystemTrayIcon::ActivationReason)));
}

TrayManager::~TrayManager()
{
	while (FNotifyOrder.count() > 0)
		removeNotify(FNotifyOrder.first());
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

bool TrayManager::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);

	FPluginManager = APluginManager;
	connect(FPluginManager->instance(),SIGNAL(shutdownStarted()),SLOT(onShutdownStarted()));

	return true;
}

bool TrayManager::initObjects()
{
	Action *action = new Action(FContextMenu);
	action->setIcon(RSR_STORAGE_MENUICONS,MNI_MAINWINDOW_QUIT);
	action->setText(tr("Quit"));
	connect(action,SIGNAL(triggered()),FPluginManager->instance(),SLOT(quit()));
	FContextMenu->addAction(action,AG_TMTM_TRAYMANAGER);
	return true;
}

bool TrayManager::startPlugin()
{
	FSystemIcon.show();
	return true;
}

//ITrayManager
QRect TrayManager::geometry() const
{
	return FSystemIcon.geometry();
}

Menu *TrayManager::contextMenu() const
{
	return FContextMenu;
}

QIcon TrayManager::icon() const
{
	return FIcon;
}

void TrayManager::setIcon(const QIcon &AIcon)
{
	FIcon = AIcon;
	if (FActiveNotify <= 0)
		FSystemIcon.setIcon(AIcon);
	else
		updateTray();
}

QString TrayManager::toolTip() const
{
	return FToolTip;
}

void TrayManager::setToolTip(const QString &AToolTip)
{
	FToolTip = AToolTip;
	if (FActiveNotify <= 0)
		FSystemIcon.setToolTip(AToolTip);
	else
		updateTray();
}

bool TrayManager::isTrayIconVisible() const
{
	return FSystemIcon.isVisible();
}

void TrayManager::setTrayIconVisible(bool AVisible)
{
	FSystemIcon.setVisible(AVisible);
}

int TrayManager::activeNotify() const
{
	return FActiveNotify;
}

QList<int> TrayManager::notifies() const
{
	return FNotifyOrder;
}

ITrayNotify TrayManager::notifyById( int ANotifyId ) const
{
	return FNotifyItems.value(ANotifyId);
}

int TrayManager::appendNotify( const ITrayNotify &ANotify )
{
	int notifyId = qrand();
	while (notifyId<=0 || FNotifyItems.contains(notifyId))
		notifyId = qrand();
	FNotifyOrder.append(notifyId);
	FNotifyItems.insert(notifyId,ANotify);
	updateTray();
	emit notifyAppended(notifyId);
	return notifyId;
}

void TrayManager::removeNotify( int ANotifyId )
{
	if (FNotifyItems.contains(ANotifyId))
	{
		FNotifyItems.remove(ANotifyId);
		FNotifyOrder.removeAll(ANotifyId);
		updateTray();
		emit notifyRemoved(ANotifyId);
	}
}

void TrayManager::showMessage(const QString &ATitle, const QString &AMessage, QSystemTrayIcon::MessageIcon AIcon, int ATimeout)
{
	FSystemIcon.showMessage(ATitle,AMessage,AIcon,ATimeout);
	emit messageShown(ATitle,AMessage,AIcon,ATimeout);
}

void TrayManager::updateTray()
{
	int notifyId = !FNotifyOrder.isEmpty() ? FNotifyOrder.last() : -1;
	if (notifyId != FActiveNotify)
	{
		FIconHidden = false;
		FBlinkTimer.stop();
		FActiveNotify = notifyId;

		if (FActiveNotify > 0)
		{
			const ITrayNotify &notify = FNotifyItems.value(notifyId);
			if (notify.blink)
				FBlinkTimer.start(BLINK_VISIBLE_TIME);
			if (!notify.iconKey.isEmpty() && !notify.iconStorage.isEmpty())
				IconStorage::staticStorage(notify.iconStorage)->insertAutoIcon(&FSystemIcon,notify.iconKey);
			else
				FSystemIcon.setIcon(notify.icon);
			FSystemIcon.setToolTip(notify.toolTip);
		}
		else
		{
			FSystemIcon.setIcon(FIcon);
			FSystemIcon.setToolTip(FToolTip);
		}

		emit activeNotifyChanged(notifyId);
	}
}

void TrayManager::onTrayIconActivated(QSystemTrayIcon::ActivationReason AReason)
{
	emit notifyActivated(FActiveNotify,AReason);
}

void TrayManager::onBlinkTimerTimeout()
{
	const ITrayNotify &notify = FNotifyItems.value(FActiveNotify);
	if (FIconHidden)
	{
		if (!notify.iconStorage.isEmpty() && !notify.iconKey.isEmpty())
			IconStorage::staticStorage(notify.iconStorage)->insertAutoIcon(&FSystemIcon,notify.iconKey);
		else
			FSystemIcon.setIcon(notify.icon);
		FBlinkTimer.start(BLINK_VISIBLE_TIME);
	}
	else
	{
		FSystemIcon.setIcon(QIcon());
		FBlinkTimer.start(BLINK_INVISIBLE_TIME);
	}
	FIconHidden = !FIconHidden;
}

void TrayManager::onShutdownStarted()
{
	FSystemIcon.hide();
}

Q_EXPORT_PLUGIN2(plg_traymanager, TrayManager)
