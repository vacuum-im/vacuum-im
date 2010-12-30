#include "systemmanager.h"
#include <thirdparty/idle/idle.h>

struct SystemManager::SystemManagerData
{
	SystemManager::SystemManagerData() {
		idle = NULL;
		idleSeconds = 0;
	}
	Idle *idle;
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
