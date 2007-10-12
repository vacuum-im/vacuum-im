#include "toolbarchanger.h"

ToolBarChanger::ToolBarChanger(QToolBar *AToolBar)
  : QObject(AToolBar)
{
  FToolBar = AToolBar;
  FToolBarMenu = new Menu;
  connect(FToolBarMenu,SIGNAL(actionInserted(QAction *, Action *)),SLOT(onActionInserted(QAction *, Action *)));
  connect(FToolBarMenu,SIGNAL(separatorInserted(Action *, QAction *)),SLOT(onSeparatorInserted(Action *, QAction *)));
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

void ToolBarChanger::removeAction(Action *AAction)
{
  FToolBarMenu->removeAction(AAction);
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

void ToolBarChanger::onSeparatorInserted(Action *ABefour, QAction * /*ASeparator*/)
{
  QAction *separator = FToolBar->insertSeparator(ABefour);
  emit separatorInserted(ABefour,separator);
}

void ToolBarChanger::onActionRemoved(Action *AAction)
{
  FToolBar->removeAction(AAction);
  FToolBar->setVisible(!FToolBarMenu->isEmpty());
  emit actionRemoved(AAction);
}

