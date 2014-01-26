#ifndef TOOLBARWIDGET_H
#define TOOLBARWIDGET_H

#include <QToolBar>
#include <interfaces/imessagewidgets.h>

class ToolBarWidget :
			public QToolBar,
			public IToolBarWidget
{
	Q_OBJECT;
	Q_INTERFACES(IToolBarWidget);
public:
	ToolBarWidget(IInfoWidget *AInfo, IViewWidget *AView, IEditWidget *AEdit, IReceiversWidget *AReceivers, QWidget *AParent);
	~ToolBarWidget();
	virtual QToolBar *instance() { return this; }
	virtual ToolBarChanger *toolBarChanger() const { return FToolBarChanger; }
	virtual IInfoWidget *infoWidget() const { return FInfoWidget; }
	virtual IViewWidget *viewWidget() const { return FViewWidget; }
	virtual IEditWidget *editWidget() const { return FEditWidget; }
	virtual IReceiversWidget *receiversWidget() const { return FReceiversWidget; }
private:
	IInfoWidget *FInfoWidget;
	IViewWidget *FViewWidget;
	IEditWidget *FEditWidget;
	IReceiversWidget *FReceiversWidget;
private:
	ToolBarChanger *FToolBarChanger;
};

#endif // TOOLBARWIDGET_H
