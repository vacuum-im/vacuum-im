#include "tabwidget.h"

#include <QTabBar>
#include <QMouseEvent>

TabWidget::TabWidget(QWidget *AParent) : QTabWidget(AParent)
{
  FPressedTabIndex = -1;
}

TabWidget::~TabWidget()
{

}

void TabWidget::mousePressEvent(QMouseEvent *AEvent)
{
  if (AEvent->buttons() == Qt::MidButton)
    FPressedTabIndex = tabBar()->tabAt(AEvent->pos());
  QTabWidget::mousePressEvent(AEvent);
}

void TabWidget::mouseReleaseEvent(QMouseEvent *AEvent)
{
  if (AEvent->buttons() == Qt::NoButton)
    if (FPressedTabIndex>=0 && FPressedTabIndex == tabBar()->tabAt(AEvent->pos()))
      emit tabCloseRequested(FPressedTabIndex);
  FPressedTabIndex = -1;
  QTabWidget::mouseReleaseEvent(AEvent);
}
