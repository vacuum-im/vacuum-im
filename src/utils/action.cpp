#include "action.h"

Action::Action(QObject *AParent) : QAction(AParent)
{
	FMenu = NULL;
	FCarbon = NULL;
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
	if (FMenu != AMenu)
	{
		if (FMenu)
			disconnect(FMenu,SIGNAL(menuDestroyed(Menu *)),this,SLOT(onMenuDestroyed(Menu *)));
		if (AMenu)
			connect(AMenu,SIGNAL(menuDestroyed(Menu *)),SLOT(onMenuDestroyed(Menu *)));
		FMenu = AMenu;
		QAction::setMenu(AMenu);
	}
}

void Action::setIcon(const QIcon &AIcon)
{
	setIcon(QString::null,QString::null,0);
	QAction::setIcon(AIcon);
}

QAction *Action::carbonAction() const
{
	return FCarbon;
}

QVariant Action::data(int ARole) const
{
	return FData.value(ARole);
}

void Action::setData(int ARole, const QVariant &AData)
{
	if (FData.value(ARole) != AData)
	{
		if (AData.isValid())
			FData.insert(ARole,AData);
		else
			FData.remove(ARole);
		emit changed();
	}
}

void Action::setData(const QHash<int,QVariant> &AData)
{
	if (AData != FData)
	{
		FData.unite(AData);
		emit changed();
	}
}

QString Action::shortcutId() const
{
	return FShortcutId;
}

void Action::setShortcutId(const QString &AId)
{
	if (FShortcutId != AId)
	{
		FShortcutId = AId;
		Shortcuts::bindObjectShortcut(AId, this);
		emit changed();
	}
}

void Action::setIcon(const QString &AStorageName, const QString &AIconKey, int AIconIndex)
{
	if (!AStorageName.isEmpty() && !AIconKey.isEmpty())
	{
		FIconStorage = IconStorage::staticStorage(AStorageName);
		FIconStorage->insertAutoIcon(this,AIconKey,AIconIndex);
		emit changed();
	}
	else if (FIconStorage)
	{
		FIconStorage->removeAutoIcon(this);
		FIconStorage = NULL;
		emit changed();
	}
}

void Action::copyActionProperties(Action *ADestination, QAction *ASource)
{
	if (ADestination != ASource)
	{
		ADestination->setAutoRepeat(ASource->autoRepeat());
		ADestination->setCheckable(ASource->isCheckable());
		ADestination->setFont(ASource->font());
		ADestination->setIcon(ASource->icon());
		ADestination->setIconText(ASource->iconText());
		ADestination->setIconVisibleInMenu(ASource->isIconVisibleInMenu());
		ADestination->setMenuRole(ASource->menuRole());
		ADestination->setPriority(ASource->priority());
		ADestination->setSeparator(ASource->isSeparator());
		ADestination->setShortcuts(ASource->shortcuts());
		ADestination->setStatusTip(ASource->statusTip());
		ADestination->setText(ASource->text());
		ADestination->setToolTip(ASource->toolTip());
		ADestination->setWhatsThis(ASource->whatsThis());

		ADestination->setChecked(ASource->isChecked());
		ADestination->setEnabled(ASource->isEnabled());
		ADestination->setVisible(ASource->isVisible());

		Action *source = qobject_cast<Action *>(ASource);
		if (source != NULL)
		{
			ADestination->setData(source->FData);
			ADestination->setShortcutId(source->shortcutId());
		}
		else
		{
			ADestination->setShortcut(ASource->shortcut());
			ADestination->setShortcutContext(ASource->shortcutContext());
		}
	}
}

Action *Action::duplicateAction(QAction *ASource, QObject *AParent)
{
	if (ASource != NULL)
	{
		Action *duplAction = new Action(AParent);
		Action::copyActionProperties(duplAction,ASource);

		duplAction->FCarbon = ASource;
		connect(ASource,SIGNAL(changed()),duplAction,SLOT(onCarbonActionChanged()));

		connect(duplAction,SIGNAL(triggered()),ASource,SLOT(trigger()));
		connect(duplAction,SIGNAL(toggled(bool)),ASource,SLOT(setChecked(bool)));
		connect(ASource,SIGNAL(destroyed()),duplAction,SLOT(deleteLater()));

		return duplAction;
	}
	return NULL;
}

Action *Action::duplicateActionAndMenu(QAction *ASource, QWidget *AParent)
{
	if (ASource != NULL)
	{
		Action *duplAction = NULL;
		if (ASource->menu() == NULL)
		{
			duplAction = Action::duplicateAction(ASource,AParent);
		}
		else if (ASource->menu()->menuAction() == ASource)
		{
			duplAction = Menu::duplicateMenu(ASource->menu(),AParent)->menuAction();
		}
		else
		{
			duplAction = Action::duplicateAction(ASource,AParent);
			duplAction->setMenu(Menu::duplicateMenu(ASource->menu(),AParent));
		}
		return duplAction;
	}
	return NULL;
}

void Action::onCarbonActionChanged()
{
	Action::copyActionProperties(this,FCarbon);
}

void Action::onMenuDestroyed(Menu *AMenu)
{
	if (AMenu == FMenu)
	{
		FMenu = NULL;
		QAction::setMenu(NULL);
	}
}
