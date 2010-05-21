#include "menubarchanger.h"

MenuBarChanger::MenuBarChanger(QMenuBar *AMenuBar) : QObject(AMenuBar)
{
	FMenuBar = AMenuBar;
}

MenuBarChanger::~MenuBarChanger()
{
	emit menuBarChangerDestroyed(this);
}

bool MenuBarChanger::isEmpty() const
{
	return FMenu.isEmpty();
}

QMenuBar *MenuBarChanger::menuBar() const
{
	return FMenuBar;
}

int MenuBarChanger::menuGroup(Menu *AMenu) const
{
	QMultiMap<int, Menu *>::const_iterator it = qFind(FMenu.begin(),FMenu.end(),AMenu);
	if (it != FMenu.constEnd())
		return it.key();
	return MBG_NULL;
}

QList<Menu *> MenuBarChanger::groupMenus(int AGroup) const
{
	if (AGroup == MBG_NULL)
		return FMenu.values();
	return FMenu.values(AGroup);
}

void MenuBarChanger::insertMenu(Menu *AMenu, int AGroup)
{
	QMultiMap<int, Menu *>::iterator it = qFind(FMenu.begin(),FMenu.end(),AMenu);
	if (it != FMenu.end())
	{
		FMenu.erase(it);
		FMenuBar->removeAction(AMenu->menuAction());
	}

	it = FMenu.upperBound(AGroup);
	Menu *befour = it!=FMenu.end() ? it.value() : NULL;

	if (befour)
		FMenuBar->insertAction(befour->menuAction(),AMenu->menuAction());
	else
		FMenuBar->addAction(AMenu->menuAction());

	FMenu.insertMulti(AGroup,AMenu);
	connect(AMenu,SIGNAL(menuDestroyed(Menu *)),SLOT(onMenuDestroyed(Menu *)));
	emit menuInserted(befour,AMenu,AGroup);
}

void MenuBarChanger::removeMenu(Menu *AMenu)
{
	QMultiMap<int, Menu *>::iterator it = qFind(FMenu.begin(),FMenu.end(),AMenu);
	if (it != FMenu.end())
	{
		disconnect(AMenu,SIGNAL(menuDestroyed(Menu *)),this,SLOT(onMenuDestroyed(Menu *)));

		FMenu.erase(it);
		FMenuBar->removeAction(AMenu->menuAction());
		emit menuRemoved(AMenu);

		if (AMenu->parent() == FMenuBar)
			AMenu->deleteLater();
	}
}

void MenuBarChanger::clear()
{
	foreach(Menu *menu,FMenu.values())
		removeMenu(menu);
	FMenuBar->clear();
}

void MenuBarChanger::onMenuDestroyed(Menu *AMenu)
{
	removeMenu(AMenu);
}
