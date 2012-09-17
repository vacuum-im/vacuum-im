#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QFrame>
#include <QSplitter>
#include <interfaces/imainwindow.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/optionvalues.h>
#include <definitions/mainwindowwidgets.h>
#include <utils/widgetmanager.h>
#include <utils/options.h>
#include "maintabwidget.h"

class MainWindow :
	public QMainWindow,
	public IMainWindow
{
	Q_OBJECT;
	Q_INTERFACES(IMainWindow);
public:
	MainWindow(QWidget *AParent = NULL, Qt::WindowFlags AFlags = 0);
	~MainWindow();
	//IMainWindow
	virtual QMainWindow *instance() { return this; }
	virtual void showWindow();
	virtual void closeWindow();
	virtual bool isActive() const;
	// Menu Management
	virtual Menu *mainMenu() const;
	virtual MenuBarChanger *mainMenuBar() const;
	// Widgets Management
	virtual QList<QWidget *> widgets() const;
	virtual int widgetOrder(QWidget *AWidget) const;
	virtual QWidget *widgetByOrder(int AOrderId) const;
	virtual void insertWidget(int AOrderId, QWidget *AWidget, int AStretch=0);
	virtual void removeWidget(QWidget *AWidget);
	// Tab Pages Management
	virtual IMainTabWidget *mainTabWidget() const;
	// Tool Bars Management
	virtual ToolBarChanger *topToolBarChanger() const;
	virtual ToolBarChanger *bottomToolBarChanger() const;
	virtual QList<ToolBarChanger *> toolBarChangers() const;
	virtual int toolBarChangerOrder(ToolBarChanger *AChanger) const;
	virtual ToolBarChanger *toolBarChangerByOrder(int AOrderId) const;
	virtual void insertToolBarChanger(int AOrderId, ToolBarChanger *AChanger);
	virtual void removeToolBarChanger(ToolBarChanger *AChanger);
	// One Window Mode Management
	virtual bool isOneWindowModeEnabled() const;
	virtual void setOneWindowModeEnabled(bool AEnabled);
signals:
	void oneWindowModeChanged(bool AEnabled);
	void widgetInserted(int AOrderId, QWidget *AWidget);
	void widgetRemoved(QWidget *AWidget);
	void toolBarChangerInserted(int AOrderId, ToolBarChanger *AChanger);
	void toolBarChangerRemoved(ToolBarChanger *AChanger);
public:
	void saveWindowGeometryAndState();
	void loadWindowGeometryAndState();
protected:
	QMenu *createPopupMenu();
	void correctWindowPosition();
protected:
	void showEvent(QShowEvent *AEvent);
	bool eventFilter(QObject *AObject, QEvent *AEvent);
protected slots:
	void onSplitterMoved(int APos, int AIndex);
private:
	IMainTabWidget *FMainTabWidget;
private:
	Menu *FMainMenu;
	QSplitter *FSplitter;
	MenuBarChanger *FMainMenuBar;
private:
	int FLeftFrameWidth;
	QFrame *FLeftFrame;
	QFrame *FRightFrame;
	QVBoxLayout *FLeftLayout;
private:
	bool FAligned;
	bool FOneWindowEnabled;
	QMap<int, QWidget *> FWidgetOrders;
	QMap<int, ToolBarChanger *> FToolBarOrders;
};

#endif // MAINWINDOW_H
