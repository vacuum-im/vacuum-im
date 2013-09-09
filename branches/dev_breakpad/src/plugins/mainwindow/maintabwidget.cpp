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
	while (currentTabPage()!=NULL)
		removeTabPage(currentTabPage());
}

QList<IMainTabPage *> MainTabWidget::tabPages() const
{
	return FTabPageOrders.values();
}

int MainTabWidget::tabPageOrder(IMainTabPage *APage) const
{
	return FTabPageOrders.key(APage);
}

IMainTabPage *MainTabWidget::tabPageByOrder(int AOrderId) const
{
	return FTabPageOrders.value(AOrderId);
}

IMainTabPage *MainTabWidget::currentTabPage() const
{
	return qobject_cast<IMainTabPage *>(currentWidget());
}

void MainTabWidget::setCurrentTabPage(IMainTabPage *APage)
{
	if (tabPages().contains(APage))
	{
		setCurrentWidget(APage->instance());
		emit currentTabPageChanged(APage);
	}
}

void MainTabWidget::insertTabPage(int AOrderId, IMainTabPage *APage)
{
	if (!FTabPageOrders.contains(AOrderId))
	{
		removeTabPage(APage);
		QMap<int, IMainTabPage *>::const_iterator it = FTabPageOrders.lowerBound(AOrderId);
		int index = it!=FTabPageOrders.constEnd() ? indexOf(it.value()->instance()) : -1;
		index = insertTab(index,APage->instance(),APage->tabPageIcon(),APage->tabPageCaption());
		setTabToolTip(index,APage->tabPageToolTip());
		FTabPageOrders.insert(AOrderId,APage);
		connect(APage->instance(),SIGNAL(tabPageChanged()),SLOT(onTabPageChanged()));
		connect(APage->instance(),SIGNAL(tabPageDestroyed()),SLOT(onTabPageDestroyed()));
		emit tabPageInserted(AOrderId,APage);
	}
}

void MainTabWidget::removeTabPage(IMainTabPage *APage)
{
	if (tabPages().contains(APage))
	{
		removeTab(indexOf(APage->instance()));
		FTabPageOrders.remove(tabPageOrder(APage));
		disconnect(APage->instance());
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

void MainTabWidget::onTabPageChanged()
{
	IMainTabPage *page = qobject_cast<IMainTabPage *>(sender());
	int index = page!=NULL ? indexOf(page->instance()) : -1;
	if (index >= 0)
	{
		setTabIcon(index,page->tabPageIcon());
		setTabText(index,page->tabPageCaption());
		setTabToolTip(index,page->tabPageToolTip());
	}
}

void MainTabWidget::onTabPageDestroyed()
{
	IMainTabPage *page = qobject_cast<IMainTabPage *>(sender());
	removeTabPage(page);
}
