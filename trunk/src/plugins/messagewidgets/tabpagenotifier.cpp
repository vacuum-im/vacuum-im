#include "tabpagenotifier.h"

TabPageNotifier::TabPageNotifier(ITabPage *ATabPage) : QObject(ATabPage->instance())
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

ITabPage *TabPageNotifier::tabPage() const
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

ITabPageNotify TabPageNotifier::notifyById(int ANotifyId) const
{
	return FNotifies.value(ANotifyId);
}

int TabPageNotifier::insertNotify(const ITabPageNotify &ANotify)
{
	if (ANotify.priority > 0)
	{
		int notifyId = qrand();
		while (notifyId<=0 || FNotifies.contains(notifyId))
			notifyId = qrand();
		FNotifies.insert(notifyId,ANotify);
		FNotifyIdByPriority.insertMulti(ANotify.priority, notifyId);
		FUpdateTimer.start();
		emit notifyInserted(notifyId);
		return notifyId;
	}
	return -1;
}

void TabPageNotifier::removeNotify(int ANotifyId)
{
	if (FNotifies.contains(ANotifyId))
	{
		ITabPageNotify notify = FNotifies.take(ANotifyId);
		FNotifyIdByPriority.remove(notify.priority, ANotifyId);
		FUpdateTimer.start();
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
