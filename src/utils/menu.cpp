#include "menu.h"

#include <QActionEvent>

Menu::Menu(QWidget *AParent) : QMenu(AParent)
{
	FCarbon = NULL;
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

void Menu::clear()
{
	foreach(QList<Action *> groupActions, FActions)
		foreach(Action *action, groupActions)
			removeAction(action);
	QMenu::clear();
}

Action *Menu::menuAction() const
{
	return FMenuAction;
}

QList<Action *> Menu::actions() const
{
	QList<Action *> allActions;
	foreach(QList<Action *> groupActions, FActions)
		allActions += groupActions;
	return allActions;
}

void Menu::setIcon(const QIcon &AIcon)
{
	setIcon(QString::null,QString::null,0);
	FMenuAction->setIcon(AIcon);
	QMenu::setIcon(AIcon);
}

void Menu::setTitle(const QString &ATitle)
{
	FMenuAction->setText(ATitle);
	QMenu::setTitle(ATitle);
}

QMenu *Menu::carbonMenu() const
{
	return FCarbon;
}

QList<Action *> Menu::actions(int AGroup) const
{
	return AGroup!=AG_NULL ? FActions.value(AGroup) : actions();
}

int Menu::actionGroup(const Action *AAction) const
{
	for (QMap<int, QList<Action *> >::const_iterator groupIt=FActions.constBegin(); groupIt!=FActions.constEnd(); ++groupIt)
		if (qFind(groupIt->constBegin(),groupIt->constEnd(),AAction) != groupIt->constEnd())
			return groupIt.key();
	return AG_NULL;
}

QAction *Menu::nextGroupSeparator(int AGroup) const
{
	QMap<int, QAction *>::const_iterator it = FSeparators.upperBound(AGroup);
	return it!=FSeparators.end() ? it.value() : NULL;
}

QList<Action *> Menu::findActions(const QMultiHash<int, QVariant> &AData, bool ADeep) const
{
	QList<Action *> actionList;
	for (QMap<int, QList<Action *> >::const_iterator groupIt=FActions.constBegin(); groupIt!=FActions.constEnd(); ++groupIt)
	{
		for(QList<Action *>::const_iterator actionIt=groupIt->constBegin(); actionIt!=groupIt->constEnd(); ++actionIt)
		{
			Action *action = *actionIt;
			
			for (QMultiHash<int, QVariant>::const_iterator dataIt=AData.constBegin(); dataIt!=AData.constEnd(); ++dataIt)
			{
				if (action->data(dataIt.key()) == dataIt.value())
				{
					actionList.append(action);
					break;
				}
			}

			if (ADeep && action->menu()!=NULL)
				actionList += action->menu()->findActions(AData,ADeep);
		}
	}
	return actionList;
}

void Menu::addAction(Action *ABefore, Action *AAction, int AGroup)
{
#ifdef Q_OS_MAC
	// https://bugreports.qt.io/browse/QTBUG-34160
	QList<QWidget *> actionWidgets = AAction->associatedWidgets();
	QList<QWidget *> menuWidgets = AAction->menu()!=NULL ? AAction->menu()->menuAction()->associatedWidgets() : QList<QWidget *>();
	if ((!actionWidgets.isEmpty() && !actionWidgets.contains(this)) || (!menuWidgets.isEmpty() && !menuWidgets.contains(this)))
	{
		AAction = Action::duplicateActionAndMenu(AAction,this);
	}
#endif

	removeAction(AAction);
	QList<Action *> &groupActions = FActions[AGroup];

	int insertIndex = ABefore!=NULL ? groupActions.indexOf(ABefore) : -1;
	QAction *insertBefore = insertIndex>=0 ? ABefore : nextGroupSeparator(AGroup);

	if (insertIndex >= 0)
		groupActions.insert(insertIndex,AAction);
	else
		groupActions.append(AAction);

	if (insertBefore != NULL)
		QMenu::insertAction(insertBefore,AAction);
	else
		QMenu::addAction(AAction);

	connect(AAction,SIGNAL(actionDestroyed(Action *)),SLOT(onActionDestroyed(Action *)));
	emit actionInserted(insertBefore,AAction,AGroup);

	if (!FSeparators.contains(AGroup))
	{
		QAction *separator = insertSeparator(AAction);
		FSeparators.insert(AGroup,separator);
		emit separatorInserted(AAction,separator);
	}
}

void Menu::addAction(Action *AAction, int AGroup, bool ASort)
{
	if (ASort && FActions.contains(AGroup))
	{
		bool isSortByRole = AAction->data(Action::DR_SortString).isValid();
		QString actionSortData = isSortByRole ? AAction->data(Action::DR_SortString).toString() : AAction->text();

		Action *before = NULL;
		foreach(Action *action, actions(AGroup))
		{
			QString beforeSortData = isSortByRole ? action->data(Action::DR_SortString).toString() : action->text();
			if (QString::localeAwareCompare(actionSortData,beforeSortData) < 0)
			{
				before = action;
				break;
			}
		}
		addAction(before,AAction,AGroup);
	}
	else
	{
		addAction(NULL,AAction,AGroup);
	}
}

void Menu::addMenuActions(const Menu *AMenu, int AGroup, bool ASort)
{
	foreach(Action *action, AMenu->actions(AGroup))
	{
		if (AGroup != AG_NULL)
			addAction(action,AGroup,ASort);
		else
			addAction(action,AMenu->actionGroup(action),ASort);
	}
}

void Menu::removeAction(Action *AAction)
{
	for (QMap<int, QList<Action *> >::iterator groupIt=FActions.begin(); groupIt!=FActions.end(); ++groupIt)
	{
		QList<Action *>::iterator actionIt = qFind(groupIt->begin(), groupIt->end(), AAction);
		if (actionIt != groupIt->end())
		{
			disconnect(AAction,SIGNAL(actionDestroyed(Action *)),this,SLOT(onActionDestroyed(Action *)));

			if (groupIt->count() == 1)
			{
				QAction *separator = FSeparators.take(groupIt.key());
				if (separator)
				{
					QMenu::removeAction(separator);
					emit separatorRemoved(separator);
				}
				FActions.erase(groupIt);
			}
			else
			{
				groupIt->erase(actionIt);
			}
			
			QMenu::removeAction(AAction);
			emit actionRemoved(AAction);

			break;
		}
	}
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

void Menu::copyMenuProperties(Menu *ADestination, QMenu *ASource, int AFirstGroup)
{
	if (ADestination != ASource)
	{
		Menu *source = qobject_cast<Menu *>(ASource);
		if (source == NULL)
		{
			foreach(QAction *srcAction, ASource->actions())
			{
				if (!srcAction->isSeparator())
				{
					Action *destAction = Action::duplicateActionAndMenu(srcAction,ADestination);
					ADestination->addAction(destAction,AFirstGroup);
				}
				else
				{
					AFirstGroup += 10;
				}
			}
		}
		else foreach(Action *srcAction, source->actions())
		{
			Action *destAction = Action::duplicateActionAndMenu(srcAction,ADestination);
			ADestination->addAction(destAction,source->actionGroup(srcAction));
		}

		ADestination->setDefaultAction(ASource->defaultAction());
		ADestination->setSeparatorsCollapsible(ASource->separatorsCollapsible());
		ADestination->setTearOffEnabled(ASource->isTearOffEnabled());

		Action::copyActionProperties(ADestination->menuAction(),ASource->menuAction());
	}
}

Action *Menu::findDuplicateAction(Menu *ADuplicate, QAction *ASource)
{
	if (ASource != NULL)
	{
		foreach(Action *destAction, ADuplicate->actions())
		{
			if (destAction->carbonAction() == ASource)
				return destAction;
			else if (destAction->menu()!=NULL && ASource->menu()!=NULL && destAction->menu()->carbonMenu()==ASource->menu())
				return destAction;
		}
	}
	return NULL;
}

Menu *Menu::duplicateMenu(QMenu *ASource, QWidget *AParent)
{
	if (ASource != NULL)
	{
		Menu *duplMenu = new Menu(AParent);
		Menu::copyMenuProperties(duplMenu,ASource,AG_DEFAULT);
		connect(ASource->menuAction(),SIGNAL(changed()),duplMenu,SLOT(onCarbonMenuActionChanged()));

		duplMenu->FCarbon = ASource;
		ASource->installEventFilter(duplMenu);
		connect(ASource,SIGNAL(destroyed()),duplMenu,SLOT(deleteLater()));

		return duplMenu;
	}
	return NULL;
}

bool Menu::eventFilter(QObject *AObject, QEvent *AEvent)
{
	if (AObject == FCarbon)
	{
		if (AEvent->type() == QEvent::ActionAdded)
		{
			QActionEvent *actionEvent = static_cast<QActionEvent *>(AEvent);
			QAction *srcAction = actionEvent->action();
			if (!srcAction->isSeparator())
			{
				Action *duplAction = Action::duplicateActionAndMenu(srcAction,this);

				Menu *source = qobject_cast<Menu *>(FCarbon);
				if (source != NULL)
				{
					int group = source->actionGroup(qobject_cast<Action *>(actionEvent->action()));
					Action *srcBefore = qobject_cast<Action *>(actionEvent->before());
					Action *duplBefore = Menu::findDuplicateAction(this,srcBefore);
					addAction(duplBefore, duplAction, group!=AG_NULL ? group : AG_DEFAULT);
				}
				else
				{
					addAction(duplAction);
				}
			}
		}
		else if (AEvent->type() == QEvent::ActionRemoved)
		{
			QActionEvent *actionEvent = static_cast<QActionEvent *>(AEvent);
			QAction *srcAction = actionEvent->action();
			if (!srcAction->isSeparator())
			{
				Action *duplAction = findDuplicateAction(this,srcAction);
				if (duplAction)
				{
					removeAction(duplAction);
					if (duplAction->menu() != NULL)
						duplAction->menu()->deleteLater();
					else
						duplAction->deleteLater();
				}
			}
		}
	}
	return QMenu::eventFilter(AObject,AEvent);
}

void Menu::onCarbonMenuActionChanged()
{
	Action::copyActionProperties(FMenuAction,FCarbon->menuAction());
}

void Menu::onActionDestroyed(Action *AAction)
{
	removeAction(AAction);
}
