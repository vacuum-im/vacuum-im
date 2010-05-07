#include "menubarwidget.h"

MenuBarWidget::MenuBarWidget(IInfoWidget *AInfo, IViewWidget *AView, IEditWidget *AEdit, IReceiversWidget *AReceivers)
{
	FInfoWidget = AInfo;
	FViewWidget = AView;
	FEditWidget = AEdit;
	FReceiversWidget = AReceivers;
	FMenuBarChanger = new MenuBarChanger(this);
}

MenuBarWidget::~MenuBarWidget()
{

}
