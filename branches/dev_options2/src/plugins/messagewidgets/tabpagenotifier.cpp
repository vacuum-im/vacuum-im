#include "tabpagenotifier.h"

#include <utils/logger.h>

TabPageNotifier::TabPageNotifier(IMessageTabPage *ATabPage) : QObject(ATabPage->instance())
{
	FTabPage = ATabPage;
	FActiveNotify = -1;

	FUpdateTimer.setInterval(0);
	FUpdateTimer.setSingleShot(true);
	connect(&FUpdateTimer,SIGNAL(timeout()),SLOT(onUpdateTimerTimeout()));
}

TabPageNotifier::~TabPageNotifier()
{
	while (!FNotifies.isEmpty())
		removeNotify(FNotifies.keys().first());
}

IMessageTabPage *TabPageNotifier::tabPage() const
{
	return FTabPage;
}

int TabPageNotifier::activeNotify() const
{
	return FActiveNotify;
}

QList<int> TabPageNotifier::notifies() const
{
	return FNotifies.keys();
}

IMessageTabPageNotify TabPageNotifier::notifyById(int ANotifyId) const
{
	return FNotifies.value(ANotifyId);
}

int TabPageNotifier::insertNotify(const IMessageTabPageNotify &ANotify)
{
	int notifyId = qrand();
	while (notifyId<=0 || FNotifies.contains(notifyId))
		notifyId = qrand();

	FNotifies.insert(notifyId,ANotify);
	FNotifyIdByPriority.insertMulti(ANotify.priority,notifyId);

	FUpdateTimer.start();

	LOG_DEBUG(QString("Tab page notification inserted, id=%1, priority=%2, blink=%3").arg(notifyId).arg(ANotify.priority).arg(ANotify.blink));
	emit notifyInserted(notifyId);

	return notifyId;
}

void TabPageNotifier::removeNotify(int ANotifyId)
{
	if (FNotifies.contains(ANotifyId))
	{
		IMessageTabPageNotify notify = FNotifies.take(ANotifyId);
		FNotifyIdByPriority.remove(notify.priority, ANotifyId);

		FUpdateTimer.start();

		LOG_DEBUG(QString("Tab page notification removed, id=%1").arg(ANotifyId));
		emit notifyRemoved(ANotifyId);
	}
}

void TabPageNotifier::onUpdateTimerTimeout()
{
	int notifyId = !FNotifyIdByPriority.isEmpty() ? FNotifyIdByPriority.value(FNotifyIdByPriority.keys().last()) : -1;
	if (notifyId != FActiveNotify)
	{
		FActiveNotify = notifyId;
		emit activeNotifyChanged(notifyId);
	}
}
