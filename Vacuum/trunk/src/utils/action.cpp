#include <QtDebug>
#include "action.h"

Action::Action(int AOrder, const QString &AActionId, QObject *AParent)
  : QAction(AParent)
{
  FOrder = AOrder;
  FActionId = AActionId;
  FContextDepended = false; 
}

Action::~Action()
{

}

bool Action::contextIsSupported( const ActionContext &AContext ) const
{
  bool support = true;
  if (!isContextDepended())
    supportContext(this,AContext,support);
  return support;
}

void Action::setContext(const ActionContext &AContext)
{
  if (FContext != AContext)
  {
    FContext = AContext;
    onNewContext();
  }
}

void Action::onNewContext()
{
  if (isContextDepended())
    emit newContext(this,FContext);
}
