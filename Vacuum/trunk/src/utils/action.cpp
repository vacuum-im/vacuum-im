#include <QtDebug>
#include "action.h"

int Action::FNewRole = Action::DR_UserDefined + 1;

Action::Action(QObject *AParent)
  : QAction(AParent)
{
  FMenu = NULL;
}

Action::~Action()
{
  emit actionDestroyed(this);
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

QVariant Action::data(int ARole) const
{
  return FData.value(ARole);
}

void Action::setMenu(Menu *AMenu)
{
  if (FMenu)
  {
    disconnect(FMenu,SIGNAL(menuDestroyed(Menu *)),this,SLOT(onMenuDestroyed(Menu *)));
    if (FMenu != AMenu && FMenu->parent()==this)
      delete FMenu;
  }

  if (AMenu)
  {
    setIcon(AMenu->icon());
    setText(AMenu->title());
    setToolTip(AMenu->toolTip());
    setWhatsThis(AMenu->whatsThis());
    connect(AMenu,SIGNAL(menuDestroyed(Menu *)),SLOT(onMenuDestroyed(Menu *)));
  }
  QAction::setMenu(AMenu);
  FMenu = AMenu;
}

int Action::newRole()
{
  FNewRole++;
  return FNewRole;
}

void Action::onMenuDestroyed(Menu *AMenu)
{
  if (AMenu == FMenu)
    FMenu = NULL;
}

