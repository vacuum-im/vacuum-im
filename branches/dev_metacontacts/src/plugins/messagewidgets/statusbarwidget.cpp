#include "statusbarwidget.h"

StatusBarWidget::StatusBarWidget(IMessageWindow *AWindow, QWidget *AParent) : QStatusBar(AParent)
{
	FWindow = AWindow;
	FStatusBarChanger = new StatusBarChanger(this);
}

StatusBarWidget::~StatusBarWidget()
{

}

bool StatusBarWidget::isVisibleOnWindow() const
{
	return isVisibleTo(FWindow->instance());
}

IMessageWindow *StatusBarWidget::messageWindow() const
{
	return FWindow;
}

StatusBarChanger *StatusBarWidget::statusBarChanger() const
{
	return FStatusBarChanger;
}
