#ifndef STATUSBARWIDGET_H
#define STATUSBARWIDGET_H

#include <QStatusBar>
#include <interfaces/imessagewidgets.h>

class StatusBarWidget :
			public QStatusBar,
			public IStatusBarWidget
{
	Q_OBJECT;
	Q_INTERFACES(IStatusBarWidget);
public:
	StatusBarWidget(IInfoWidget *AInfo, IViewWidget *AView, IEditWidget *AEdit, IReceiversWidget *AReceivers);
	~StatusBarWidget();
	virtual QStatusBar *instance() { return this; }
	virtual StatusBarChanger *statusBarChanger() const { return FStatusBarChanger; }
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
	StatusBarChanger *FStatusBarChanger;
};

#endif // STATUSBARWIDGET_H
