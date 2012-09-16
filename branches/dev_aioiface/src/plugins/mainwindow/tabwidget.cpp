#include "tabwidget.h"

#include <QTabBar>

TabWidget::TabWidget(QWidget *AParent) : QTabWidget(AParent)
{
	tabBar()->setVisible(false);
}

TabWidget::~TabWidget()
{

}

void TabWidget::tabInserted(int AIndex)
{
	QTabWidget::tabInserted(AIndex);
	tabBar()->setVisible(count()>1);
}

void TabWidget::tabRemoved(int AIndex)
{
	QTabWidget::tabRemoved(AIndex);
	tabBar()->setVisible(count()>1);
}
