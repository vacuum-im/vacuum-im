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
  FMenu = AMenu;
  if (AMenu)
  {
    setIcon(AMenu->icon());
    setText(AMenu->title());
    setToolTip(AMenu->toolTip());
    setWhatsThis(AMenu->whatsThis());
  }
  QAction::setMenu(AMenu);
}

int Action::newRole()
{
  FNewRole++;
  return FNewRole;
}

