#include "maincentralwidget.h"

MainCentralWidget::MainCentralWidget(IMainWindow *AMainWindow, QWidget *AParent) : QStackedWidget(AParent)
{
	FMainWindow = AMainWindow;
	connect(this,SIGNAL(currentChanged(int)),SLOT(onCurrentIndexChanged(int)));
}

MainCentralWidget::~MainCentralWidget()
{
	while (currentCentralPage()!=NULL)
		removeCentralPage(currentCentralPage());
}

QList<IMainCentralPage *> MainCentralWidget::centralPages() const
{
	return FCentralPages;
}

IMainCentralPage *MainCentralWidget::currentCentralPage() const
{
	return qobject_cast<IMainCentralPage *>(currentWidget());
}

void MainCentralWidget::setCurrentCentralPage(IMainCentralPage *APage)
{
	if (centralPages().contains(APage))
		setCurrentWidget(APage->instance());
}

void MainCentralWidget::appendCentralPage(IMainCentralPage *APage)
{
	if (!FCentralPages.contains(APage))
	{
		FCentralPages.append(APage);
		connect(APage->instance(),SIGNAL(centralPageShow(bool)),SLOT(onCentralPageShow(bool)));
		connect(APage->instance(),SIGNAL(centralPageChanged()),SLOT(onCentralPageChanged()));
		connect(APage->instance(),SIGNAL(centralPageDestroyed()),SLOT(onCentralPageDestroyed()));
		emit centralPageAppended(APage);
		addWidget(APage->instance());
	}
}

void MainCentralWidget::removeCentralPage(IMainCentralPage *APage)
{
	if (FCentralPages.contains(APage))
	{
		FCentralPages.removeAll(APage);
		disconnect(APage->instance());
		removeWidget(APage->instance());
		APage->instance()->setParent(NULL);
		emit centralPageRemoved(APage);
	}
}

void MainCentralWidget::onCurrentIndexChanged(int AIndex)
{
	IMainCentralPage *page = qobject_cast<IMainCentralPage *>(widget(AIndex));
	emit currentCentralPageChanged(page);
	emit currentCentralPageChanged();
}

void MainCentralWidget::onCentralPageShow(bool AMinimized)
{
	IMainCentralPage *page = qobject_cast<IMainCentralPage *>(sender());
	if (page)
	{
		setCurrentCentralPage(page);
		FMainWindow->showWindow(AMinimized);
	}
}

void MainCentralWidget::onCentralPageChanged()
{
	IMainCentralPage *page = qobject_cast<IMainCentralPage *>(sender());
	if (page && page==currentCentralPage())
		emit currentCentralPageChanged();
}

void MainCentralWidget::onCentralPageDestroyed()
{
	IMainCentralPage *page = qobject_cast<IMainCentralPage *>(sender());
	removeCentralPage(page);
}
