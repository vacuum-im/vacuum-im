#include "tabwidget.h"

#include <QTabBar>
#include <QMouseEvent>

TabWidget::TabWidget(QWidget *AParent) : QTabWidget(AParent)
{
	FPressedTabIndex = -1;
	tabBar()->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(tabBar(), SIGNAL(tabMoved(int,int)), SIGNAL(tabMoved(int,int)));
	connect(tabBar(), SIGNAL(customContextMenuRequested(const QPoint &)), SLOT(onTabBarContextMenuRequested(const QPoint &)));
}

TabWidget::~TabWidget()
{

}

void TabWidget::mousePressEvent(QMouseEvent *AEvent)
{
	FPressedTabIndex = tabBar()->tabAt(AEvent->pos());
	QTabWidget::mousePressEvent(AEvent);
}

void TabWidget::mouseReleaseEvent(QMouseEvent *AEvent)
{
	int index = tabBar()->tabAt(AEvent->pos());
	if (index>=0 && index==FPressedTabIndex)
	{
		if (AEvent->button() == Qt::MidButton)
		{
			emit tabCloseRequested(index);
		}
	}
	FPressedTabIndex = -1;
	QTabWidget::mouseReleaseEvent(AEvent);
}

void TabWidget::onTabBarContextMenuRequested(const QPoint &APos)
{
	emit tabMenuRequested(tabBar()->tabAt(APos));
}
