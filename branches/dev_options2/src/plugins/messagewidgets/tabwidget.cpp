#include "tabwidget.h"

#include <QTabBar>
#include <QMouseEvent>

TabWidget::TabWidget(QWidget *AParent) : QTabWidget(AParent)
{
	FTabBarVisible = true;
	FPressedTabIndex = -1;
	tabBar()->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(tabBar(), SIGNAL(tabMoved(int,int)), SIGNAL(tabMoved(int,int)));
	connect(tabBar(), SIGNAL(customContextMenuRequested(const QPoint &)), SLOT(onTabBarContextMenuRequested(const QPoint &)));
}

TabWidget::~TabWidget()
{

}

bool TabWidget::isTabBarVisible() const
{
	return FTabBarVisible;
}

void TabWidget::setTabBarVisible(bool AVisible)
{
	if (AVisible != FTabBarVisible)
	{
		FTabBarVisible = AVisible;
		tabBar()->setVisible(AVisible);
	}
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
			emit tabCloseRequested(index);
	}
	FPressedTabIndex = -1;
	QTabWidget::mouseReleaseEvent(AEvent);
}

void TabWidget::onTabBarContextMenuRequested(const QPoint &APos)
{
	emit tabMenuRequested(tabBar()->tabAt(APos));
}
