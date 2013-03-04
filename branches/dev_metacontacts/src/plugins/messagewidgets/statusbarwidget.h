#ifndef STATUSBARWIDGET_H
#define STATUSBARWIDGET_H

#include <QStatusBar>
#include <interfaces/imessagewidgets.h>

class StatusBarWidget :
	public QStatusBar,
	public IMessageStatusBarWidget
{
	Q_OBJECT;
	Q_INTERFACES(IMessageWidget IMessageStatusBarWidget);
public:
	StatusBarWidget(IMessageWindow *AWindow, QWidget *AParent);
	~StatusBarWidget();
	// IMessageWidget
	virtual QStatusBar *instance() { return this; }
	virtual bool isVisibleOnWindow() const;
	virtual IMessageWindow *messageWindow() const;
	// IMessageStatusBarWidget
	virtual StatusBarChanger *statusBarChanger() const;
private:
	IMessageWindow *FWindow;
	StatusBarChanger *FStatusBarChanger;
};

#endif // STATUSBARWIDGET_H
