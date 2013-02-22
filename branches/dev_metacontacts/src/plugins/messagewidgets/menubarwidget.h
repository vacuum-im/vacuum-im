#ifndef MENUBARWIDGET_H
#define MENUBARWIDGET_H

#include <QMenuBar>
#include <interfaces/imessagewidgets.h>

class MenuBarWidget :
	public QMenuBar,
	public IMessageMenuBarWidget
{
	Q_OBJECT;
	Q_INTERFACES(IMessageWidget IMessageMenuBarWidget);
public:
	MenuBarWidget(IMessageWindow *AWindow, QWidget *AParent);
	~MenuBarWidget();
	// IMessageWidget
	virtual QMenuBar *instance() { return this; }
	virtual IMessageWindow *messageWindow() const;
	// IMessageMenuBarWidget
	virtual MenuBarChanger *menuBarChanger() const;
private:
	IMessageWindow *FWindow;
	MenuBarChanger *FMenuBarChanger;
};

#endif // MENUBARWIDGET_H
