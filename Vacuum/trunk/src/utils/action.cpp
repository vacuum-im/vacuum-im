#include <QtDebug>
#include "action.h"

int Action::FNewRole = Action::DR_UserDefined + 1;

Action::Action(int AOrder, QObject *AParent)
  : QAction(AParent)
{
  FOrder = AOrder;
}

Action::~Action()
{

}

int Action::newRole()
{
  FNewRole++;
  return FNewRole;
}

void Action::setData(int ARole, const QVariant &AData)
{
  if (AData.isValid())
    FData.insert(ARole,AData);
  else
    FData.remove(ARole);
}

QVariant Action::data(int ARole) const
{
  return FData.value(ARole);
}

