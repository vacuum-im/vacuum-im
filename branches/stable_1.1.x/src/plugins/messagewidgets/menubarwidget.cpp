#include "menubarwidget.h"

MenuBarWidget::MenuBarWidget(IInfoWidget *AInfo, IViewWidget *AView, IEditWidget *AEdit, IReceiversWidget *AReceivers, QWidget *AParent) : QMenuBar(AParent)
{
	FInfoWidget = AInfo;
	FViewWidget = AView;
	FEditWidget = AEdit;
	FReceiversWidget = AReceivers;
	FMenuBarChanger = new MenuBarChanger(this);

	// On Ubuntu 11.10 empty menubar cause segmentation fault
	addAction(QString::null)->setVisible(false);
}

MenuBarWidget::~MenuBarWidget()
{

}
