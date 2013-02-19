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
	FShortcutId = AId;
	Shortcuts::bindObjectShortcut(AId, this);
}

QString Action::reduceString(const QString &AString, int AMaxChars)
{
	if (AString.length()>AMaxChars && AMaxChars>3)
	{
		QString string = AString.left(AMaxChars-3);
		while (string.endsWith(' '))
			string.chop(1);
		return string + "...";
	}
	return AString;
}

bool Action::copyStandardAction(Action *ADestination, QAction *ASource)
{
	if (ADestination && ASource && !ASource->isSeparator())
	{
		ADestination->setActionGroup(ASource->actionGroup());
		ADestination->setAutoRepeat(ASource->autoRepeat());
		ADestination->setCheckable(ASource->isCheckable());
		ADestination->setChecked(ASource->isChecked());
		ADestination->setEnabled(ASource->isEnabled());
		ADestination->setFont(ASource->font());
		ADestination->setIcon(ASource->icon());
		ADestination->setIconText(ASource->iconText());
		ADestination->setIconVisibleInMenu(ASource->isIconVisibleInMenu());
		ADestination->setMenuRole(ASource->menuRole());
		ADestination->setPriority(ASource->priority());
		ADestination->setShortcut(ASource->shortcut());
		ADestination->setShortcutContext(ASource->shortcutContext());
		ADestination->setStatusTip(ASource->statusTip());
		ADestination->setText(ASource->text());
		ADestination->setToolTip(ASource->toolTip());
		ADestination->setVisible(ASource->isVisible());
		ADestination->setWhatsThis(ASource->whatsThis());
		connect(ADestination,SIGNAL(triggered()),ASource,SLOT(trigger()));
		connect(ADestination,SIGNAL(toggled(bool)),ASource,SLOT(toggle()));
		return true;
	}
	return false;
}

void Action::onMenuDestroyed(Menu *AMenu)
{
	Q_UNUSED(AMenu);
	FMenu = NULL;
	QAction::setMenu(NULL);
}
