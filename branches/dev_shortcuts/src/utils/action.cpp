#include "action.h"

Action::Action(QObject *AParent) : QAction(AParent)
{
	FMenu = NULL;
	FIconStorage = NULL;
}

Action::~Action()
{
	if (FIconStorage)
		FIconStorage->removeAutoIcon(this);
	emit actionDestroyed(this);
}

Menu *Action::menu() const
{
	return FMenu;
}

void Action::setMenu(Menu *AMenu)
{
	if (FMenu)
	{
		disconnect(FMenu,SIGNAL(menuDestroyed(Menu *)),this,SLOT(onMenuDestroyed(Menu *)));
		if (FMenu!=AMenu && FMenu->parent()==this)
			delete FMenu;
	}
	if (AMenu)
	{
		connect(AMenu,SIGNAL(menuDestroyed(Menu *)),SLOT(onMenuDestroyed(Menu *)));
	}
	QAction::setMenu(AMenu);
	FMenu = AMenu;
}

void Action::setIcon(const QIcon &AIcon)
{
	setIcon(QString::null,QString::null,0);
	QAction::setIcon(AIcon);
}

void Action::setIcon(const QString &AStorageName, const QString &AIconKey, int AIconIndex)
{
	if (!AStorageName.isEmpty() && !AIconKey.isEmpty())
	{
		FIconStorage = IconStorage::staticStorage(AStorageName);
		FIconStorage->insertAutoIcon(this,AIconKey,AIconIndex);
	}
	else if (FIconStorage)
	{
		FIconStorage->removeAutoIcon(this);
		FIconStorage = NULL;
	}
}

QVariant Action::data(int ARole) const
{
	return FData.value(ARole);
}

void Action::setData(int ARole, const QVariant &AData)
{
	if (AData.isValid())
		FData.insert(ARole,AData);
	else
		FData.remove(ARole);
}

void Action::setData(const QHash<int,QVariant> &AData)
{
	FData.unite(AData);
}

QString Action::shortcutId() const
{
	return FShortcutId;
}

void Action::setShortcutId(const QString &AId)
{
	if (!AId.isEmpty())
		Shortcuts::instance()->bind(AId, this);
	FShortcutId = AId;
}

void Action::onMenuDestroyed(Menu *AMenu)
{
	Q_UNUSED(AMenu);
	FMenu = NULL;
	QAction::setMenu(NULL);
}
