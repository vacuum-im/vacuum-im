#ifndef TOOLBARWIDGET_H
#define TOOLBARWIDGET_H

#include <QToolBar>
#include <interfaces/imessagewidgets.h>

class ToolBarWidget :
	public QToolBar,
	public IMessageToolBarWidget
{
	Q_OBJECT;
	Q_INTERFACES(IMessageWidget IMessageToolBarWidget);
public:
	ToolBarWidget(IMessageWindow *AWindow, QWidget *AParent);
	~ToolBarWidget();
	// IMessageWidget
	virtual QToolBar *instance() { return this; }
	virtual IMessageWindow *messageWindow() const;
	// IMessageToolBarWidget
	virtual ToolBarChanger *toolBarChanger() const;
private:
	IMessageWindow *FWindow;
	ToolBarChanger *FToolBarChanger;
};

#endif // TOOLBARWIDGET_H
