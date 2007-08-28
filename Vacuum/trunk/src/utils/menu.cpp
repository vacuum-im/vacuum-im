#include <QtDebug>
#include "menu.h"

Menu::Menu(QWidget *AParent)
  : QMenu(AParent)
{
  FMenuAction = NULL;
  FIconset = NULL;
  setSeparatorsCollapsible(true);
}

Menu::~Menu()
{
  emit menuDestroyed(this);
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
      QList<QAction *> actionList = QMenu::actions();
      
      bool sortRole = true;
      QString sortString = AAction->data(Action::DR_SortString).toString();
      if (sortString.isEmpty())
      {
        sortString = AAction->text();
        sortRole = false;
      }
      
      for (int i = 0; !befour && i<actionList.count(); ++i)
      {
        QAction *qaction = actionList.at(i);
        if (FActions.key((Action *)qaction)==AGroup)
        {
          QString curSortString = qaction->text();
          if (sortRole)
          {
            Action *action = qobject_cast<Action *>(qaction);
            if (action)
              curSortString = action->data(Action::DR_SortString).toString();
          }
          if (QString::localeAwareCompare(curSortString,sortString) > 0)
            befour = actionList.at(i);
        }
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
  connect(AAction,SIGNAL(actionDestroyed(Action *)),SLOT(onActionDestroyed(Action *)));
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
  }
  return FMenuAction;
}

void Menu::setIcon(const QIcon &AIcon)
{
  if (FIconset)
  {
    FIconName.clear();
    delete FIconset;
    FIconset = NULL;
  }
  QMenu::setIcon(AIcon);
  
  if (FMenuAction)
    FMenuAction->setIcon(AIcon);
}

void Menu::setIcon(const QString &AIconsetFile, const QString &AIconName)
{
  if (!FIconset)
  {
    FIconset = new SkinIconset(AIconsetFile,this);
    connect(FIconset,SIGNAL(skinChanged()),SLOT(onSkinChanged()));
  }
  FIconName = AIconName;
  FIconset->openFile(AIconsetFile);
  QMenu::setIcon(FIconset->iconByName(AIconName));
  
  if (FMenuAction)
    FMenuAction->setIcon(this->icon());
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
    disconnect(AAction,SIGNAL(actionDestroyed(Action *)),this,SLOT(onActionDestroyed(Action *)));

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
  return AG_NULL;
}

QList<Action *> Menu::actions(int AGroup) const
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
      if (AData.values(key).contains(action->data(key)))
      {
        actionList.append(action);
        break;
      }
    if (ASearchInSubMenu && action->menu())
      actionList += action->menu()->findActions(AData,ASearchInSubMenu);
  }

  return actionList;
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

void Menu::onActionDestroyed(Action *AAction)
{
  removeAction(AAction);
}

void Menu::onSkinChanged()
{
  if (FIconset)
  {
    QMenu::setIcon(FIconset->iconByName(FIconName));
    if (FMenuAction)
      FMenuAction->setIcon(this->icon());
  }
}

