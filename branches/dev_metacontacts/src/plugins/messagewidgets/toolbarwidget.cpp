#include "toolbarwidget.h"

ToolBarWidget::ToolBarWidget(IMessageWindow *AWindow, QWidget *AParent) : QToolBar(AParent)
{
	FWindow = AWindow;
	FToolBarChanger = new ToolBarChanger(this);
	setIconSize(QSize(16,16));
}

ToolBarWidget::~ToolBarWidget()
{

}

bool ToolBarWidget::isVisibleOnWindow() const
{
	return isVisibleTo(FWindow->instance());
}

IMessageWindow *ToolBarWidget::messageWindow() const
{
	return FWindow;
}

ToolBarChanger *ToolBarWidget::toolBarChanger() const
{
	return FToolBarChanger;
}
