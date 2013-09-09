#include "systemmanager.h"
#include <thirdparty/idle/idle.h>

struct SystemManager::SystemManagerData
{
	SystemManagerData() {
		idle = new Idle;
		idleSeconds = 0;
	}
	~SystemManagerData() {
		delete idle;
	}
	Idle *idle;
	int idleSeconds;
};

SystemManager::SystemManager()
{
	d = new SystemManagerData;
	connect(d->idle,SIGNAL(secondsIdle(int)),SLOT(onIdleChanged(int)));
}

SystemManager::~SystemManager()
{
	delete d;
}

SystemManager *SystemManager::instance()
{
	static SystemManager *inst = new SystemManager;
	return inst;
}

int SystemManager::systemIdle()
{
	return instance()->d->idleSeconds;
}

bool SystemManager::isSystemIdleActive()
{
	return instance()->d->idle->isActive();
}

void SystemManager::startSystemIdle()
{
	SystemManagerData *q = instance()->d;
	if (!q->idle->isActive())
		q->idle->start();
}

void SystemManager::stopSystemIdle()
{
	SystemManagerData *q = instance()->d;
	if (q->idle->isActive())
		q->idle->stop();
}

void SystemManager::onIdleChanged(int ASeconds)
{
	d->idleSeconds = ASeconds;
	emit systemIdleChanged(ASeconds);
}
