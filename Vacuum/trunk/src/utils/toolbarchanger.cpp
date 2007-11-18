#include "toolbarchanger.h"

ToolBarChanger::ToolBarChanger(QToolBar *AToolBar) : QObject(AToolBar)
{
  FToolBar = AToolBar;
  FToolBar->clear();
  FIntVisible = false;
  FExtVisible = FToolBar->isVisible();
  FChangingIntVisible = 0;
  FToolBar->installEventFilter(this);
  FToolBarMenu = new Menu;
  connect(FToolBarMenu,SIGNAL(actionInserted(QAction *, Action *)),SLOT(onActionInserted(QAction *, Action *)));
  connect(FToolBarMenu,SIGNAL(separatorInserted(Action *, QAction *)),SLOT(onSeparatorInserted(Action *, QAction *)));
  connect(FToolBarMenu,SIGNAL(separatorRemoved(QAction *)),SLOT(onSeparatorRemoved(QAction *)));
  connect(FToolBarMenu,SIGNAL(actionRemoved(Action *)),SLOT(onActionRemoved(Action *)));
  updateVisible();
}

ToolBarChanger::~ToolBarChanger()
{
  emit toolBarChangerDestroyed(this);
  delete FToolBarMenu;
}

bool ToolBarChanger::isEmpty() const
{
  return FToolBarMenu->isEmpty() && FWidgetActions.isEmpty();
}

void ToolBarChanger::addAction(Action *AAction, int AGroup, bool ASort)
{
  FToolBarMenu->addAction(AAction,AGroup,ASort);
}

void ToolBarChanger::addToolButton(Action *AAction, Qt::ToolButtonStyle AStyle, QToolButton::ToolButtonPopupMode AMode, 
                                   int AGroup , bool ASort)
{
  QToolButton *button = new QToolButton;
  button->setDefaultAction(AAction);
  button->setPopupMode(AMode);
  button->setToolButtonStyle(AStyle);
  FActionButtons.insert(AAction,button);
  FToolBarMenu->addAction(AAction,AGroup,ASort);
}

QAction *ToolBarChanger::addWidget(QWidget *AWidget, int AGroup)
{
  QAction *befour = FToolBarMenu->nextGroupSeparator(AGroup);
  QAction *action = befour != NULL ? FToolBar->insertWidget(befour,AWidget) : FToolBar->addWidget(AWidget);
  FWidgetActions.insert(AWidget,action);
  connect(AWidget,SIGNAL(destroyed(QObject *)),SLOT(onWidgetDestroyed(QObject *)));
  emit widgetInserted(AWidget,action,befour);
  updateVisible();
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
    updateVisible();
  }
}

void ToolBarChanger::clear()
{
  FToolBarMenu->clear();
  FToolBar->clear();
}

void ToolBarChanger::updateVisible()
{
  FIntVisible = !isEmpty();
  if (!FToolBar->isWindow() && (FIntVisible && FExtVisible) != FToolBar->isVisible())
  {
    FChangingIntVisible++;
    FToolBar->setVisible(FIntVisible && FExtVisible);
    FChangingIntVisible--;
  }
}

bool ToolBarChanger::eventFilter(QObject *AObject, QEvent *AEvent)
{
  if (AEvent->type() == QEvent::Show)
  {
    if (FChangingIntVisible == 0)
      FExtVisible = true;
    if (!FIntVisible || !FExtVisible)
    {
      FChangingIntVisible++;
      FToolBar->hide();
      FChangingIntVisible--;
      return true;
    }
  }
  else if (AEvent->type() == QEvent::Hide)
  {
    if (FChangingIntVisible == 0)
      FExtVisible = false;
  }
  return QObject::eventFilter(AObject,AEvent);
}

void ToolBarChanger::onActionInserted(QAction *ABefour, Action *AAction)
{
  if (FActionButtons.contains(AAction))
  {
    QToolButton *button = FActionButtons.value(AAction);
    QAction *action = ABefour != NULL ? FToolBar->insertWidget(ABefour,button) : FToolBar->addWidget(button);
    FWidgetActions.insert(button,action);
  }
  else
    ABefour != NULL ? FToolBar->insertAction(ABefour,AAction) : FToolBar->addAction(AAction);
  emit actionInserted(ABefour,AAction);
  updateVisible();
}

void ToolBarChanger::onSeparatorInserted(Action *ABefour, QAction *ASeparator)
{
  QAction *separator = NULL;
  if (FActionButtons.contains(ABefour))
  {
    QAction *action = FWidgetActions.value(FActionButtons.value(ABefour));
    separator = FToolBar->insertSeparator(action);
  }
  else
    separator = FToolBar->insertSeparator(ABefour);
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
  if (FActionButtons.contains(AAction))
  {
    QToolButton *button = FActionButtons.take(AAction);
    QAction *action = FWidgetActions.take(button);
    FToolBar->removeAction(action);
    delete button;
  }
  else
    FToolBar->removeAction(AAction);
  emit actionRemoved(AAction);
  updateVisible();
}

void ToolBarChanger::onWidgetDestroyed(QObject *AObject)
{
  QWidget *widget = qobject_cast<QWidget *>(AObject);
  removeWidget(widget);
}

