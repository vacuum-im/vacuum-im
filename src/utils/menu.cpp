#include "menu.h"

Menu::Menu(QWidget *AParent) : QMenu(AParent)
{
	FIconStorage = NULL;

	FMenuAction = new Action(this);
	FMenuAction->setMenu(this);

	setSeparatorsCollapsible(true);
}

Menu::~Menu()
{
	if (FIconStorage)
		FIconStorage->removeAutoIcon(this);
	emit menuDestroyed(this);
}

bool Menu::isEmpty() const
{
	return FActions.isEmpty();
}

Action *Menu::menuAction() const
{
	return FMenuAction;
}

int Menu::actionGroup(const Action *AAction) const
{
	QMultiMap<int,Action *>::const_iterator it = qFind(FActions.begin(),FActions.end(),AAction);
	if (it != FActions.constEnd())
		return it.key();
	return AG_NULL;
}

QAction *Menu::nextGroupSeparator(int AGroup) const
{
	QMultiMap<int,Action *>::const_iterator it = FActions.lowerBound(AGroup);
	if (it != FActions.end())
		return FSeparators.value(it.key());
	return NULL;
}

QList<Action *> Menu::groupActions(int AGroup) const
{
	if (AGroup == AG_NULL)
		return FActions.values();
	return FActions.values(AGroup);
}

QList<Action *> Menu::findActions(const QMultiHash<int, QVariant> AData, bool ASearchInSubMenu /*= false*/) const
{
	QList<Action *> actionList;
	QList<int> keys = AData.keys();
	foreach(Action *action,FActions)
	{
		foreach (int key, keys)
		{
			if (AData.values(key).contains(action->data(key)))
			{
				actionList.append(action);
				break;
			}
		}
		if (ASearchInSubMenu && action->menu())
			actionList += action->menu()->findActions(AData,ASearchInSubMenu);
	}
	return actionList;
}

void Menu::addAction(Action *AAction, int AGroup, bool ASort)
{
	QAction *before = NULL;
	QAction *separator = NULL;
	QMultiMap<int,Action *>::iterator it = qFind(FActions.begin(),FActions.end(),AAction);
	if (it != FActions.end())
	{
		if (FActions.values(it.key()).count() == 1)
			FSeparators.remove(it.key());
		FActions.erase(it);
		QMenu::removeAction(AAction);
	}

	it = FActions.find(AGroup);
	if (it == FActions.end())
	{
		before = nextGroupSeparator(AGroup);
		before != NULL ? QMenu::insertAction(before,AAction) : QMenu::addAction(AAction);
		separator = insertSeparator(AAction);
		FSeparators.insert(AGroup,separator);
	}
	else
	{
		if (ASort)
		{
			QList<QAction *> actionList = QMenu::actions();

			bool sortRole = true;
			QString sortString = AAction->data(Action::DR_SortString).toString();
			if (sortString.isEmpty())
			{
				sortString = AAction->text();
				sortRole = false;
			}

			for (int i = 0; !before && i<actionList.count(); ++i)
			{
				Action *action = qobject_cast<Action *>(actionList.at(i));
				if (FActions.key(action)==AGroup)
				{
					QString curSortString = action->text();
					if (sortRole)
						curSortString = action->data(Action::DR_SortString).toString();
					if (QString::localeAwareCompare(curSortString,sortString) > 0)
						before = actionList.at(i);
				}
			}
		}

		if (!before)
		{
			QMap<int,QAction *>::const_iterator sepIt= FSeparators.upperBound(AGroup);
			if (sepIt != FSeparators.constEnd())
				before = sepIt.value();
		}

		if (before)
			QMenu::insertAction(before,AAction);
		else
			QMenu::addAction(AAction);
	}

	FActions.insertMulti(AGroup,AAction);
	connect(AAction,SIGNAL(actionDestroyed(Action *)),SLOT(onActionDestroyed(Action *)));
	emit actionInserted(before,AAction,AGroup,ASort);
	if (separator) emit separatorInserted(AAction,separator);
}

void Menu::addMenuActions(const Menu *AMenu, int AGroup, bool ASort)
{
	foreach(Action *action,AMenu->groupActions(AGroup))
		addAction(action,AMenu->actionGroup(action),ASort);
}

void Menu::removeAction(Action *AAction)
{
	QMultiMap<int,Action *>::iterator it = qFind(FActions.begin(),FActions.end(),AAction);
	if (it != FActions.end())
	{
		disconnect(AAction,SIGNAL(actionDestroyed(Action *)),this,SLOT(onActionDestroyed(Action *)));

		if (FActions.values(it.key()).count() == 1)
		{
			QAction *separator = FSeparators.value(it.key());
			FSeparators.remove(it.key());
			QMenu::removeAction(separator);
			emit separatorRemoved(separator);
		}

		FActions.erase(it);
		QMenu::removeAction(AAction);

		emit actionRemoved(AAction);

		Menu *menu = AAction->menu();
		if (menu && menu->parent() == this)
			menu->deleteLater();
		else if (AAction->parent() == this)
			AAction->deleteLater();
	}
}

void Menu::clear()
{
	foreach(Action *action,FActions.values())
		removeAction(action);
	QMenu::clear();
}

void Menu::setIcon(const QIcon &AIcon)
{
	setIcon(QString::null,QString::null,0);
	FMenuAction->setIcon(AIcon);
	QMenu::setIcon(AIcon);
}

void Menu::setIcon(const QString &AStorageName, const QString &AIconKey, int AIconIndex)
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

void Menu::setTitle(const QString &ATitle)
{
	FMenuAction->setText(ATitle);
	QMenu::setTitle(ATitle);
}

void Menu::onActionDestroyed(Action *AAction)
{
	removeAction(AAction);
}
