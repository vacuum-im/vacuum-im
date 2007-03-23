#include <QtDebug>
#include "menu.h"

Menu::Menu(QWidget *AParent)
  : QMenu(AParent)
{
  FMenuAction = NULL;
  setSeparatorsCollapsible(true);
}

Menu::~Menu()
{

}

void Menu::addAction(Action *AAction, int AGroup, bool ASort)
{
  ActionList::iterator it = qFind(FActions.begin(),FActions.end(),AAction);
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
    it = FActions.lowerBound(AGroup);
    if (it != FActions.end())
    {
      QAction *sepataror = FSeparators.value(it.key()); 
      QMenu::insertAction(sepataror,AAction);
    }
    else
      QMenu::addAction(AAction); 
    FSeparators.insert(AGroup,insertSeparator(AAction));
  }
  else
  {
    QAction *befour = NULL;
    if (ASort)
    {
      while (!befour && it != FActions.end() && it.key() == AGroup)
      {
        if (QString::localeAwareCompare(it.value()->text(),AAction->text()) > 0)
          befour = it.value();
        it++;
      }
    }

    if (!befour)
    {
      QMap<int,QAction *>::const_iterator sepIt= FSeparators.upperBound(AGroup);
      if (sepIt != FSeparators.constEnd())
        befour = sepIt.value();
    }

    if (befour)
      QMenu::insertAction(befour,AAction); 
    else
      QMenu::addAction(AAction); 
  }

  FActions.insertMulti(AGroup,AAction);
  connect(AAction,SIGNAL(destroyed(QObject *)),SLOT(onActionDestroyed(QObject *)));
  emit addedAction(AAction);
}

void Menu::addMenuActions(const Menu *AMenu, int AGroup, bool ASort)
{
  Action *action;
  QList<Action *> actionList = AMenu->actions(AGroup);
  foreach(action,actionList)
    addAction(action,AMenu->actionGroup(action),ASort);
}

Action *Menu::menuAction()
{
  if (!FMenuAction)
  {
    FMenuAction = new Action(this);
    FMenuAction->setMenu(this);
    FMenuAction->setIcon(icon());
    FMenuAction->setText(title());
    connect(FMenuAction,SIGNAL(triggered(bool)),SLOT(onMenuActionTriggered(bool)));
  }
  FMenuAction->setToolTip(toolTip());
  FMenuAction->setWhatsThis(whatsThis()); 
  return FMenuAction;
}

void Menu::setIcon( const QIcon &AIcon )
{
  if (FMenuAction)
    FMenuAction->setIcon(AIcon);
  QMenu::setIcon(AIcon);
}

void Menu::setTitle(const QString &ATitle)
{
  if (FMenuAction)
    FMenuAction->setText(ATitle);
  QMenu::setTitle(ATitle);
}

void Menu::removeAction(Action *AAction)
{
  ActionList::iterator it = qFind(FActions.begin(),FActions.end(),AAction);
  if (it != FActions.end())
  {
    disconnect(AAction,SIGNAL(destroyed(QObject *)),this,SLOT(onActionDestroyed(QObject *)));

    emit removedAction(AAction);

    if (FActions.values(it.key()).count() == 1)
    {
      QAction *separator = FSeparators.value(it.key());
      FSeparators.remove(it.key());
      QMenu::removeAction(separator);  
    }

    FActions.erase(it);
    QMenu::removeAction(AAction); 
  }
}

int Menu::actionGroup(const Action *AAction) const
{
  ActionList::const_iterator it = qFind(FActions.begin(),FActions.end(),AAction);
  if (it != FActions.constEnd())
    return it.key();
  return NULL_ACTION_GROUP;
}

QList<Action *> Menu::actions(int AGroup) const
{
  if (AGroup != NULL_ACTION_GROUP)
    return FActions.values();
  return FActions.values(AGroup);
}

void Menu::clear() 
{
  Action * action;
  foreach(action,FActions)
  {
    Menu *menu = action->menu();
    if (menu && menu->parent() == this)
      delete menu;
    else if (action->parent() == this)
      delete action;
  }
  FSeparators.clear();
  FActions.clear();
  QMenu::clear(); 
}

void Menu::onMenuActionTriggered(bool)
{

}

void Menu::onActionDestroyed(QObject *AAction)
{
  Action *action = qobject_cast<Action *>(AAction);
  if (action)
    removeAction(action);
}
