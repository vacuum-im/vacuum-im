#include "statusbarchanger.h"

#include <QTimer>

StatusBarChanger::StatusBarChanger(QStatusBar *AStatusBar) : QObject(AStatusBar)
{
  FStatusBar = AStatusBar;

  FIntVisible = false;
  FExtVisible = FStatusBar->isVisible();
  FManageVisibility = true;
  FChangingIntVisible = 0;
  FVisibleTimerStarted = false;

  FStatusBar->installEventFilter(this);
  connect(FStatusBar,SIGNAL(messageChanged(const QString &)),SLOT(updateVisible()));

  updateVisible();
}

StatusBarChanger::~StatusBarChanger()
{
  emit statusBarChangerDestroyed(this);
}

bool StatusBarChanger::manageVisibitily() const
{
  return FManageVisibility;
}

void StatusBarChanger::setManageVisibility(bool AManageVisibility)
{
  FManageVisibility = AManageVisibility;
  updateVisible();
}

QStatusBar *StatusBarChanger::statusBar() const
{
  return FStatusBar;
}

int StatusBarChanger::widgetGroup(QWidget *AWidget) const
{
  QMultiMap<int, QWidget *>::const_iterator it = qFind(FWidgets.begin(),FWidgets.end(),AWidget);
  if (it != FWidgets.constEnd())
    return it.key();
  return SBG_NULL;
}

QList<QWidget *> StatusBarChanger::groupWidgets(int AGroup) const
{
  if (AGroup == SBG_NULL)
    return FWidgets.values();
  return FWidgets.values(AGroup);
}

void StatusBarChanger::insertWidget(QWidget *AWidget, int AGroup, bool APermanent, int AStretch)
{
  QMultiMap<int, QWidget *>::iterator it = qFind(FWidgets.begin(),FWidgets.end(),AWidget);
  if (it == FWidgets.end())
  {
    it = FWidgets.upperBound(AGroup);
    QWidget *befour = it!=FWidgets.end() ? it.value() : NULL;
    int index = FWidgets.values().indexOf(befour);

    if (index>=0)
    {
      if (APermanent)
        FStatusBar->insertPermanentWidget(index,AWidget,AStretch);
      else
        FStatusBar->insertWidget(index,AWidget,AStretch);
    }
    else
    {
      if (APermanent)
        FStatusBar->addPermanentWidget(AWidget,AStretch);
      else
        FStatusBar->addWidget(AWidget,AStretch);
    }

    FWidgets.insertMulti(AGroup,AWidget);
    connect(AWidget,SIGNAL(destroyed(QObject *)),SLOT(onWidgetDestroyed(QObject *)));
    emit widgetInserted(befour,AWidget,AGroup,APermanent,AStretch);
    updateVisible();
  }
}

void StatusBarChanger::removeWidget(QWidget *AWidget)
{
  QMultiMap<int, QWidget *>::iterator it = qFind(FWidgets.begin(),FWidgets.end(),AWidget);
  if (it != FWidgets.end())
  {
    disconnect(AWidget,SIGNAL(destroyed(QObject *)),this,SLOT(onWidgetDestroyed(QObject *)));

    FWidgets.erase(it);
    FStatusBar->removeWidget(AWidget);
    emit widgetRemoved(AWidget);

    if (AWidget->parent() == FStatusBar)
      AWidget->deleteLater();

    updateVisible();
  }
}

void StatusBarChanger::clear()
{
  foreach(QWidget *widget,FWidgets.values())
    removeWidget(widget);
}

void StatusBarChanger::updateVisible()
{
  FIntVisible = !FWidgets.isEmpty() || !FStatusBar->currentMessage().isEmpty();
  if (FManageVisibility && !FVisibleTimerStarted)
  {
    QTimer::singleShot(0,this,SLOT(onChangeVisible()));
    FVisibleTimerStarted = true;
  }
}

bool StatusBarChanger::eventFilter(QObject *AObject, QEvent *AEvent)
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

void StatusBarChanger::onWidgetDestroyed(QObject *AObject)
{
  QWidget *widget = reinterpret_cast<QWidget *>(AObject);
  QMultiMap<int, QWidget *>::iterator it = qFind(FWidgets.begin(),FWidgets.end(),widget);
  if (it != FWidgets.end())
  {
    FWidgets.erase(it);
    emit widgetRemoved(widget);
    updateVisible();
  }
}

void StatusBarChanger::onChangeVisible()
{
  if (FManageVisibility && !FStatusBar->isWindow() && (FIntVisible && FExtVisible)!=FStatusBar->isVisible())
  {
    FChangingIntVisible++;
    FStatusBar->setVisible(FIntVisible && FExtVisible);
    FChangingIntVisible--;
  }
  FVisibleTimerStarted = false;
}
