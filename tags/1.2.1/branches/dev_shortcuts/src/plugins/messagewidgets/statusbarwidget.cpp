#include "statusbarwidget.h"

StatusBarWidget::StatusBarWidget(IInfoWidget *AInfo, IViewWidget *AView, IEditWidget *AEdit, IReceiversWidget *AReceivers)
{
	FInfoWidget = AInfo;
	FViewWidget = AView;
	FEditWidget = AEdit;
	FReceiversWidget = AReceivers;
	FStatusBarChanger = new StatusBarChanger(this);
}

StatusBarWidget::~StatusBarWidget()
{

}
