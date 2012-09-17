#include "maintabwidget.h"

#include <QTabBar>

MainTabWidget::MainTabWidget(QWidget *AParent) : QTabWidget(AParent)
{
	setMovable(false);
	setDocumentMode(true);
	tabBar()->setVisible(false);
}

MainTabWidget::~MainTabWidget()
{

}

QList<QWidget *> MainTabWidget::tabPages() const
{
	return FTabPageOrders.values();
}

int MainTabWidget::tabPageOrder(QWidget *APage) const
{
	return FTabPageOrders.key(APage);
}

QWidget *MainTabWidget::tabPageByOrder(int AOrderId) const
{
	return FTabPageOrders.value(AOrderId);
}

QWidget *MainTabWidget::currentTabPage() const
{
	return currentWidget();
}

void MainTabWidget::setCurrentTabPage(QWidget *APage)
{
	if (tabPages().contains(APage))
	{
		setCurrentWidget(APage);
		emit currentTabPageChanged(APage);
	}
}

void MainTabWidget::insertTabPage(int AOrderId, QWidget *APage)
{
	if (!FTabPageOrders.contains(AOrderId))
	{
		removeTabPage(APage);
		QMap<int, QWidget *>::const_iterator it = FTabPageOrders.lowerBound(AOrderId);
		int index = it!=FTabPageOrders.constEnd() ? indexOf(it.value()) : -1;
		insertTab(index,APage,APage->windowIcon(),APage->windowIconText());
		FTabPageOrders.insert(AOrderId,APage);
		updateTabPage(APage);
		emit tabPageInserted(AOrderId,APage);
	}
}

void MainTabWidget::updateTabPage(QWidget *APage)
{
	int index = indexOf(APage);
	if (index >= 0)
	{
		setTabIcon(index,APage->windowIcon());
		setTabText(index,APage->windowIconText());
		emit tabPageUpdated(APage);
	}
}

void MainTabWidget::removeTabPage(QWidget *APage)
{
	if (tabPages().contains(APage))
	{
		removeTab(indexOf(APage));
		FTabPageOrders.remove(tabPageOrder(APage));
		emit tabPageRemoved(APage);
	}
}

void MainTabWidget::tabInserted(int AIndex)
{
	QTabWidget::tabInserted(AIndex);
	tabBar()->setVisible(count()>1);
}

void MainTabWidget::tabRemoved(int AIndex)
{
	QTabWidget::tabRemoved(AIndex);
	tabBar()->setVisible(count()>1);
}
