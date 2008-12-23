#include "toolbarchanger.h"

#include <QTimer>

ToolBarChanger::ToolBarChanger(QToolBar *AToolBar) : QObject(AToolBar)
{
  FToolBar = AToolBar;
  FToolBar->clear();
  FIntVisible = false;
  FExtVisible = FToolBar->isVisible();
  FSeparatorsVisible = true;
  FManageVisibility = true;
  FVisibleTimerStarted = false;
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

void ToolBarChanger::setSeparatorsVisible(bool ASeparatorsVisible)
{
  FSeparatorsVisible = ASeparatorsVisible;
  foreach(QAction *separator, FBarSepByMenuSep)
    separator->setVisible(ASeparatorsVisible);
}

void ToolBarChanger::setManageVisibility(bool AManageVisibility)
{
  FManageVisibility = AManageVisibility;
  updateVisible();
}

void ToolBarChanger::addAction(Action *AAction, int AGroup, bool ASort)
{
  FToolBarMenu->addAction(AAction,AGroup,ASort);
}

QToolButton *ToolBarChanger::addToolButton(Action *AAction, int AGroup , bool ASort)
{
  QToolButton *button = new QToolButton;
  button->setDefaultAction(AAction);
  FActionButtons.insert(AAction,button);
  FToolBarMenu->addAction(AAction,AGroup,ASort);
  return button;
}

QAction *ToolBarChanger::addWidget(QWidget *AWidget, int AGroup)
{
  QAction *befour = FToolBarMenu->nextGroupSeparator(AGroup);
  QAction *action = befour != NULL ? FToolBar->insertWidget(befour,AWidget) : FToolBar->addWidget(AWidget);
  FWidgetActions.insert(AWidget,action);
  connect(AWidget,SIGNAL(destroyed(QObject *)),SLOT(onWidgetDestroyed(QObject *)));
  emit widgetInserted(AWidget,action,befour);
  emit toolBarChanged();
  updateVisible();
  return action;
}

int ToolBarChanger::actionGroup(const Action *AAction) const
{
  return FToolBarMenu->actionGroup(AAction);
}

QList<Action *> ToolBarChanger::actions(int AGroup) const
{
  return FToolBarMenu->actions(AGroup);
}

QList<Action *> ToolBarChanger::findActions(const QMultiHash<int, QVariant> AData, bool ASearchInSubMenu) const
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
    emit toolBarChanged();
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
  if (FManageVisibility && !FVisibleTimerStarted)
  {
    QTimer::singleShot(0,this,SLOT(onChangeVisible()));
    FVisibleTimerStarted = true;
  }
}

bool ToolBarChanger::eventFilter(QObject *AObject, QEvent *AEvent)
{
  if (AEvent->type() == QEvent::Show)
  {
    if (FChangingIntVisible == 0)
    {
      FExtVisible = true;
      if (FManageVisibility && !FIntVisible)
        updateVisible();
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
  QAction *befour = FWidgetActions.value(FActionButtons.value((Action*)ABefour),FBarSepByMenuSep.value(ABefour,ABefour));
  if (FActionButtons.contains(AAction))
  {
    QToolButton *button = FActionButtons.value(AAction);
    QAction *action = befour != NULL ? FToolBar->insertWidget(befour,button) : FToolBar->addWidget(button);
    FWidgetActions.insert(button,action);
  }
  else
    befour != NULL ? FToolBar->insertAction(befour,AAction) : FToolBar->addAction(AAction);
  emit actionInserted(ABefour,AAction);
  emit toolBarChanged();
  updateVisible();
}

void ToolBarChanger::onSeparatorInserted(Action *ABefour, QAction *ASeparator)
{
  QAction *befour = FWidgetActions.value(FActionButtons.value(ABefour),FBarSepByMenuSep.value(ABefour,ABefour));
  QAction *separator = FToolBar->insertSeparator(befour);
  separator->setVisible(FSeparatorsVisible);
  FBarSepByMenuSep.insert(ASeparator,separator);
  emit separatorInserted(ABefour,separator);
  emit toolBarChanged();
}

void ToolBarChanger::onSeparatorRemoved(QAction *ASeparator)
{
  QAction *separator = FBarSepByMenuSep.value(ASeparator);
  FToolBar->removeAction(separator);
  emit separatorRemoved(separator);
  emit toolBarChanged();
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
  emit toolBarChanged();
  updateVisible();
}

void ToolBarChanger::onWidgetDestroyed(QObject *AObject)
{
  QWidget *widget = qobject_cast<QWidget *>(AObject);
  removeWidget(widget);
}

void ToolBarChanger::onChangeVisible()
{
  if (FManageVisibility && !FToolBar->isWindow() && (FIntVisible && FExtVisible)!=FToolBar->isVisible())
  {
    FChangingIntVisible++;
    FToolBar->setVisible(FIntVisible && FExtVisible);
    FChangingIntVisible--;
  }
  FVisibleTimerStarted = false;
}
