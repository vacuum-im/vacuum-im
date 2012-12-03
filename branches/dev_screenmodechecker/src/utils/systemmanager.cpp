#include "systemmanager.h"
#include <thirdparty/idle/idle.h>
#include <thirdparty/screenmodechecker/screenmodechecker.h>
#include <QDebug>

struct SystemManager::SystemManagerData
{
	SystemManagerData() {
		idle = NULL;
		screenmode = NULL;
		idleSeconds = 0;
	}
	Idle *idle;
	ScreenModeChecker *screenmode;

	int idleSeconds;
};

SystemManager::SystemManagerData *SystemManager::d = new SystemManager::SystemManagerData;

SystemManager *SystemManager::instance()
{
	static SystemManager *manager = NULL;
	if (!manager)
	{
		manager = new SystemManager;
		manager->d->idle = new Idle;
		manager->d->screenmode = new ScreenModeChecker;
		connect(manager->d->idle,SIGNAL(secondsIdle(int)),manager,SLOT(onIdleChanged(int)));
	}
	return manager;
}

int SystemManager::systemIdle()
{
	return d->idleSeconds;
}

bool SystemManager::isSystemIdleActive()
{
	return d->idle!=NULL ? d->idle->isActive() : false;
}

void SystemManager::startSystemIdle()
{
	if (d->idle && !d->idle->isActive())
		d->idle->start();
}

bool SystemManager::isScreenSaverRunning()
{
	return !d->screenmode->isDummy() ? d->screenmode->isScreensaverActive() : false;
}

bool SystemManager::isFullScreenMode()
{
	qDebug() << "isDummy?" << d->screenmode->isDummy();
	qDebug() << "isFull?" << d->screenmode->isFullscreenAppActive();
	qDebug() << "isScreen?" << d->screenmode->isScreensaverActive();
	return !d->screenmode->isDummy() ? d->screenmode->isFullscreenAppActive() : false;
}

void SystemManager::stopSystemIdle()
{
	if (d->idle && d->idle->isActive())
		d->idle->stop();
}

void SystemManager::onIdleChanged(int ASeconds)
{
	d->idleSeconds = ASeconds;
	emit systemIdleChanged(ASeconds);
}
