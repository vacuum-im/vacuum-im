#include "systemmanager.h"
#include <thirdparty/idle/idle.h>
#include <thirdparty/screenstate/screenstate.h>

struct SystemManager::SystemManagerData
{
	SystemManagerData() {
		idle = NULL;
		screenState = NULL;
		idleSeconds = 0;
	}
	Idle *idle;
	ScreenState *screenState;

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
		manager->d->screenState = new ScreenState;
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
	return !d->screenState->isDummy() ? d->screenState->isScreenSaverRunning() : false;
}

bool SystemManager::isFullscreenAppActive()
{
	return !d->screenState->isDummy() ? d->screenState->isFullscreenAppActive() : false;
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
