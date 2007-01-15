#include <QtDebug>
#include "action.h"

Action::Action(int AOrder, QObject *AParent)
  : QAction(AParent)
{
  FOrder = AOrder;
  FContextDepended = false; 
}

Action::Action(int AOrder, const QString &AText, QObject *AParent)
  : QAction(AText,AParent)
{
  FOrder = AOrder;
  FContextDepended = false; 
}

Action::Action(int AOrder, const QIcon &AIcon, const QString &AText, QObject *AParent)
  : QAction(AIcon,AText,AParent)
{
  FOrder = AOrder;
  FContextDepended = false; 
}

Action::~Action()
{

}

bool Action::contextSupported( const ActionContext &AContext ) const
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
