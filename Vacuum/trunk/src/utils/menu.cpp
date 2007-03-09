#include <QtDebug>
#include "menu.h"

Menu::Menu(int AOrder, QWidget *AParent)
  : QMenu(AParent)
{
  FOrder = AOrder;
  FMenuAction = 0;
  setSeparatorsCollapsible(true);
}

Menu::~Menu()
{

}

void Menu::addAction(Action *AAction)
{
  if (!AAction)
    return;

  clearNullActions();

  ActionList::iterator i = findAction(AAction);
  if (i == FActions.end())
  {
    i = FActions.find(AAction->order());  
    if (i == FActions.end())  
    {
      i = FActions.lowerBound(AAction->order());
      if (i == FActions.end())
        QMenu::addAction(AAction); 
      else
        QMenu::insertAction(findOrderSeparator(i.value()->order()),AAction);
      insertSeparator(AAction)->setData(AAction->order());
    }
    else
      QMenu::insertAction(i.value(),AAction); 

    FActions.insertMulti(AAction->order(),AAction);
  
    emit addedAction(AAction);
  }
}

void Menu::addMenuActions(const Menu *AMenu)
{
  QAction *qaction;
  QList<QAction *> actionList = AMenu->actions();
  foreach(qaction,actionList)
  {
    Action *action = qobject_cast<Action *>(qaction);
    if (action)
      addAction(action);
    else
      QMenu::addAction(qaction);
  }
}

Action *Menu::menuAction()
{
  if (!FMenuAction)
  {
    FMenuAction = new Action(order(),this);
    FMenuAction->setMenu(this); 
  }
  FMenuAction->setIcon(icon());
  FMenuAction->setText(title());
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
  clearNullActions();

  ActionList::iterator i = findAction(AAction);
  if (i != FActions.end())
  {
    emit removedAction(AAction);

    if (FActions.values(AAction->order()).count() == 1)
      QMenu::removeAction(findOrderSeparator(AAction->order()));  

    FActions.erase(i);
    QMenu::removeAction(AAction); 
  }
}

void Menu::clear() 
{
  FActions.clear();
  QMenu::clear(); 
}

Menu::ActionList::iterator Menu::findAction(const Action *AAction)
{
  if (!AAction)
    return FActions.end();

  ActionList::iterator i = FActions.find(AAction->order());
  while (i != FActions.end() && i.key() == AAction->order() && i.value() != AAction)
    ++i;
  
  if (i.key() == AAction->order())
    return i;

  return FActions.end();
}

void Menu::clearNullActions()
{
  ActionList::iterator i = FActions.begin();
  while (i!=FActions.end())
  {
    if (i.value().isNull())
      i = FActions.erase(i);
    else
      ++i;
  }
}

QAction *Menu::findOrderSeparator(int AOrder) const
{
  QAction *action;
  QList<QAction *> actionList = actions();
  foreach(action,actionList)
    if (action->isSeparator() && action->data().toInt() == AOrder)
      return action;
  return 0;
}

