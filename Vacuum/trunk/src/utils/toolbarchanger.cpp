#include "toolbarchanger.h"

ToolBarChanger::ToolBarChanger(QToolBar *AToolBar)
  : QObject(AToolBar)
{
  FToolBar = AToolBar;
  FToolBarMenu = new Menu;
  connect(FToolBarMenu,SIGNAL(actionInserted(QAction *, Action *)),SLOT(onActionInserted(QAction *, Action *)));
  connect(FToolBarMenu,SIGNAL(separatorInserted(Action *, QAction *)),SLOT(onSeparatorInserted(Action *, QAction *)));
  connect(FToolBarMenu,SIGNAL(separatorRemoved(QAction *)),SLOT(onSeparatorRemoved(QAction *)));
  connect(FToolBarMenu,SIGNAL(actionRemoved(Action *)),SLOT(onActionRemoved(Action *)));
  FToolBar->setVisible(FToolBar->actions().count() > 0);
}

ToolBarChanger::~ToolBarChanger()
{
  emit toolBarChangerDestroyed(this);
  delete FToolBarMenu;
}

void ToolBarChanger::addAction(Action *AAction, int AGroup /*= AG_DEFAULT*/, bool ASort /*= false*/)
{
  FToolBarMenu->addAction(AAction,AGroup,ASort);
}

QAction *ToolBarChanger::addWidget(QWidget *AWidget, int AGroup /*= AG_DEFAULT*/)
{
  QAction *befour = FToolBarMenu->nextGroupSeparator(AGroup);
  QAction *action = befour != NULL ? FToolBar->insertWidget(befour,AWidget) : FToolBar->addWidget(AWidget);
  FWidgetActions.insert(AWidget,action);
  FToolBar->setVisible(true);
  connect(AWidget,SIGNAL(destroyed(QObject *)),SLOT(onWidgetDestroyed(QObject *)));
  emit widgetInserted(AWidget,action,befour);
  return action;
}

int ToolBarChanger::actionGroup(const Action *AAction) const
{
  return FToolBarMenu->actionGroup(AAction);
}

QList<Action *> ToolBarChanger::actions(int AGroup /*= AG_NULL*/) const
{
  return FToolBarMenu->actions(AGroup);
}

QList<Action *> ToolBarChanger::findActions(const QMultiHash<int, QVariant> AData, bool ASearchInSubMenu /*= false*/) const
{
  return FToolBarMenu->findActions(AData,ASearchInSubMenu);
}

QAction * ToolBarChanger::widgetAction(QWidget *AWidget) const
{
  return FWidgetActions.value(AWidget);
}

void ToolBarChanger::removeAction(Action *AAction)
{
  FToolBarMenu->removeAction(AAction);
}

void ToolBarChanger::removeWidget(QWidget *AWidget)
{
  QAction *action = FWidgetActions.value(AWidget);
  if (action)
  {
    FToolBar->removeAction(action);
    FWidgetActions.remove(AWidget);
    disconnect(AWidget,SIGNAL(destroyed(QObject *)),this,SLOT(onWidgetDestroyed(QObject *)));
    emit widgetRemoved(AWidget,action);
  }
}

void ToolBarChanger::clear()
{
  FToolBarMenu->clear();
  FToolBar->clear();
}

void ToolBarChanger::onActionInserted(QAction *ABefour, Action *AAction)
{
  ABefour != NULL ? FToolBar->insertAction(ABefour,AAction) : FToolBar->addAction(AAction);
  FToolBar->setVisible(true);
  emit actionInserted(ABefour,AAction);
}

void ToolBarChanger::onSeparatorInserted(Action *ABefour, QAction *ASeparator)
{
  QAction *separator = FToolBar->insertSeparator(ABefour);
  FBarSepByMenuSep.insert(ASeparator,separator);
  emit separatorInserted(ABefour,separator);
}

void ToolBarChanger::onSeparatorRemoved(QAction *ASeparator)
{
  QAction *separator = FBarSepByMenuSep.value(ASeparator);
  FToolBar->removeAction(separator);
  emit separatorRemoved(separator);
}

void ToolBarChanger::onActionRemoved(Action *AAction)
{
  FToolBar->removeAction(AAction);
  FToolBar->setVisible(!FToolBarMenu->isEmpty() || !FWidgetActions.isEmpty());
  emit actionRemoved(AAction);
}

void ToolBarChanger::onWidgetDestroyed(QObject *AObject)
{
  QWidget *widget = qobject_cast<QWidget *>(AObject);
  removeWidget(widget);
}

