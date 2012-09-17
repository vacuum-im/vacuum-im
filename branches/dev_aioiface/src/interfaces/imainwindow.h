#ifndef IMAINWINDOW_H
#define IMAINWINDOW_H

#include <QToolBar>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QMainWindow>
#include <QStackedWidget>
#include <utils/menu.h>
#include <utils/menubarchanger.h>
#include <utils/toolbarchanger.h>

#define MAINWINDOW_UUID "{A6F3D775-8464-4599-AB79-97BA1BAA6E96}"

class IMainTabWidget
{
public:
	virtual QTabWidget *instance() = 0;
	virtual QList<QWidget *> tabPages() const =0;
	virtual int tabPageOrder(QWidget *APage) const =0;
	virtual QWidget *tabPageByOrder(int AOrderId) const =0;
	virtual QWidget *currentTabPage() const =0;
	virtual void setCurrentTabPage(QWidget *APage) =0;
	virtual void insertTabPage(int AOrderId, QWidget *APage) =0;
	virtual void updateTabPage(QWidget *APage) =0;
	virtual void removeTabPage(QWidget *APage) =0;
protected:
	virtual void currentTabPageChanged(QWidget *APage) =0;
	virtual void tabPageInserted(int AOrderId, QWidget *APage) =0;
	virtual void tabPageUpdated(QWidget *APage) =0;
	virtual void tabPageRemoved(QWidget *APage) =0;
};

class IMainWindow
{
public:
	virtual QMainWindow *instance() =0;
	virtual void showWindow() =0;
	virtual void closeWindow() =0;
	virtual bool isActive() const =0;
	// Menu Management
	virtual Menu *mainMenu() const =0;
	virtual MenuBarChanger *mainMenuBar() const =0;
	// Widgets Management
	virtual QList<QWidget *> widgets() const =0;
	virtual int widgetOrder(QWidget *AWidget) const =0;
	virtual QWidget *widgetByOrder(int AOrderId) const =0;
	virtual void insertWidget(int AOrderId, QWidget *AWidget, int AStretch=0) =0;
	virtual void removeWidget(QWidget *AWidget) =0;
	// Tab Pages Management
	virtual IMainTabWidget *mainTabWidget() const =0;
	// Tool Bars Management
	virtual ToolBarChanger *topToolBarChanger() const =0;
	virtual ToolBarChanger *bottomToolBarChanger() const =0;
	virtual QList<ToolBarChanger *> toolBarChangers() const =0;
	virtual int toolBarChangerOrder(ToolBarChanger *AChanger) const =0;
	virtual ToolBarChanger *toolBarChangerByOrder(int AOrderId) const =0;
	virtual void insertToolBarChanger(int AOrderId, ToolBarChanger *AChanger) =0;
	virtual void removeToolBarChanger(ToolBarChanger *AChanger) =0;
	// One Window Mode Management
	virtual bool isOneWindowModeEnabled() const =0;
	virtual void setOneWindowModeEnabled(bool AEnabled) =0;
protected:
	virtual void oneWindowModeChanged(bool AEnabled) =0;
	virtual void widgetInserted(int AOrderId, QWidget *AWidget) =0;
	virtual void widgetRemoved(QWidget *AWidget) =0;
	virtual void toolBarChangerInserted(int AOrderId, ToolBarChanger *AChanger) =0;
	virtual void toolBarChangerRemoved(ToolBarChanger *AChanger) =0;
};

class IMainWindowPlugin
{
public:
	virtual QObject *instance() = 0;
	virtual IMainWindow *mainWindow() const = 0;
};

Q_DECLARE_INTERFACE(IMainWindow,"Vacuum.Plugin.IMainWindow/1.1")
Q_DECLARE_INTERFACE(IMainTabWidget,"Vacuum.Plugin.IMainTabWidget/1.0")
Q_DECLARE_INTERFACE(IMainWindowPlugin,"Vacuum.Plugin.IMainWindowPlugin/1.1")

#endif // IMAINWINDOW_H
