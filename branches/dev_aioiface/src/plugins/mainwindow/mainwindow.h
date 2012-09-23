#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QFrame>
#include <QSplitter>
#include <interfaces/imainwindow.h>
#include <definitions/version.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/optionvalues.h>
#include <definitions/mainwindowwidgets.h>
#include <utils/widgetmanager.h>
#include <utils/options.h>
#include "maintabwidget.h"
#include "maincentralwidget.h"

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
	virtual bool isActive() const;
	virtual void showWindow(bool AMinimized = false);
	virtual void closeWindow();
	// Menu Management
	virtual Menu *mainMenu() const;
	virtual MenuBarChanger *mainMenuBar() const;
	// Widgets Management
	virtual QList<QWidget *> widgets() const;
	virtual int widgetOrder(QWidget *AWidget) const;
	virtual QWidget *widgetByOrder(int AOrderId) const;
	virtual void insertWidget(int AOrderId, QWidget *AWidget, int AStretch=0);
	virtual void removeWidget(QWidget *AWidget);
	// Tool Bars Management
	virtual ToolBarChanger *topToolBarChanger() const;
	virtual ToolBarChanger *bottomToolBarChanger() const;
	virtual QList<ToolBarChanger *> toolBarChangers() const;
	virtual int toolBarChangerOrder(ToolBarChanger *AChanger) const;
	virtual ToolBarChanger *toolBarChangerByOrder(int AOrderId) const;
	virtual void insertToolBarChanger(int AOrderId, ToolBarChanger *AChanger);
	virtual void removeToolBarChanger(ToolBarChanger *AChanger);
	// Pages Management
	virtual IMainTabWidget *mainTabWidget() const;
	virtual IMainCentralWidget *mainCentralWidget() const;
	virtual bool isCentralWidgetVisible() const;
	virtual void setCentralWidgetVisible(bool AEnabled);
signals:
	void widgetInserted(int AOrderId, QWidget *AWidget);
	void widgetRemoved(QWidget *AWidget);
	void toolBarChangerInserted(int AOrderId, ToolBarChanger *AChanger);
	void toolBarChangerRemoved(ToolBarChanger *AChanger);
	void centralWidgetVisibleChanged(bool AVisible);
public:
	void saveWindowGeometryAndState();
	void loadWindowGeometryAndState();
protected:
	void updateWindow();
	QMenu *createPopupMenu();
	void correctWindowPosition();
protected:
	void showEvent(QShowEvent *AEvent);
	bool eventFilter(QObject *AObject, QEvent *AEvent);
protected slots:
	void onCurrentCentralPageChanged();
	void onSplitterMoved(int APos, int AIndex);
private:
	IMainTabWidget *FTabWidget;
	IMainCentralWidget *FCentralWidget;
private:
	Menu *FMainMenu;
	QFrame *FLeftWidget;
	QVBoxLayout *FLeftLayout;
	QSplitter *FSplitter;
	MenuBarChanger *FMainMenuBar;
private:
	bool FAligned;
	bool FCentralVisible;
	int FLeftWidgetWidth;
	int FSplitterHandleWidth;
	QMap<int, QWidget *> FWidgetOrders;
	QMap<int, ToolBarChanger *> FToolBarOrders;
};

#endif // MAINWINDOW_H
