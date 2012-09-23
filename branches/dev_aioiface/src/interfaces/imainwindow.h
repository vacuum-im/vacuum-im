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

class IMainTabPage
{
public:
	virtual QWidget *instance() = 0;
	virtual QIcon tabPageIcon() const =0;
	virtual QString tabPageCaption() const =0;
	virtual QString tabPageToolTip() const =0;
protected:
	virtual void tabPageChanged() =0;
	virtual void tabPageDestroyed() =0;
};

class IMainTabWidget
{
public:
	virtual QTabWidget *instance() = 0;
	virtual QList<IMainTabPage *> tabPages() const =0;
	virtual int tabPageOrder(IMainTabPage *APage) const =0;
	virtual IMainTabPage *tabPageByOrder(int AOrderId) const =0;
	virtual IMainTabPage *currentTabPage() const =0;
	virtual void setCurrentTabPage(IMainTabPage *APage) =0;
	virtual void insertTabPage(int AOrderId, IMainTabPage *APage) =0;
	virtual void removeTabPage(IMainTabPage *APage) =0;
protected:
	virtual void currentTabPageChanged(IMainTabPage *APage) =0;
	virtual void tabPageInserted(int AOrderId, IMainTabPage *APage) =0;
	virtual void tabPageRemoved(IMainTabPage *APage) =0;
};

class IMainCentralPage
{
public:
	virtual QWidget *instance() = 0;
	virtual void showCentralPage(bool AMinimized = false) =0;
	virtual QIcon centralPageIcon() const =0;
	virtual QString centralPageCaption() const =0;
protected:
	virtual void centralPageShow(bool AMinimized) =0;
	virtual void centralPageChanged() =0;
	virtual void centralPageDestroyed() =0;
};

class IMainCentralWidget
{
public:
	virtual QStackedWidget *instance() = 0;
	virtual QList<IMainCentralPage *> centralPages() const =0;
	virtual IMainCentralPage *currentCentralPage() const =0;
	virtual void setCurrentCentralPage(IMainCentralPage *APage) =0;
	virtual void appendCentralPage(IMainCentralPage *APage) =0;
	virtual void removeCentralPage(IMainCentralPage *APage) =0;
protected:
	virtual void currentCentralPageChanged() =0;
	virtual void currentCentralPageChanged(IMainCentralPage *APage) =0;
	virtual void centralPageAppended(IMainCentralPage *APage) =0;
	virtual void centralPageRemoved(IMainCentralPage *APage) =0;
};

class IMainWindow
{
public:
	virtual QMainWindow *instance() =0;
	virtual bool isActive() const =0;
	virtual void showWindow(bool AMinimized = false) =0;
	virtual void closeWindow() =0;
	// Menu Management
	virtual Menu *mainMenu() const =0;
	virtual MenuBarChanger *mainMenuBar() const =0;
	// Widgets Management
	virtual QList<QWidget *> widgets() const =0;
	virtual int widgetOrder(QWidget *AWidget) const =0;
	virtual QWidget *widgetByOrder(int AOrderId) const =0;
	virtual void insertWidget(int AOrderId, QWidget *AWidget, int AStretch=0) =0;
	virtual void removeWidget(QWidget *AWidget) =0;
	// Tool Bars Management
	virtual ToolBarChanger *topToolBarChanger() const =0;
	virtual ToolBarChanger *bottomToolBarChanger() const =0;
	virtual QList<ToolBarChanger *> toolBarChangers() const =0;
	virtual int toolBarChangerOrder(ToolBarChanger *AChanger) const =0;
	virtual ToolBarChanger *toolBarChangerByOrder(int AOrderId) const =0;
	virtual void insertToolBarChanger(int AOrderId, ToolBarChanger *AChanger) =0;
	virtual void removeToolBarChanger(ToolBarChanger *AChanger) =0;
	// Pages Management
	virtual IMainTabWidget *mainTabWidget() const =0;
	virtual IMainCentralWidget *mainCentralWidget() const =0;
	virtual bool isCentralWidgetVisible() const =0;
	virtual void setCentralWidgetVisible(bool AVisible) =0;
protected:
	virtual void widgetInserted(int AOrderId, QWidget *AWidget) =0;
	virtual void widgetRemoved(QWidget *AWidget) =0;
	virtual void toolBarChangerInserted(int AOrderId, ToolBarChanger *AChanger) =0;
	virtual void toolBarChangerRemoved(ToolBarChanger *AChanger) =0;
	virtual void centralWidgetVisibleChanged(bool AVisible) =0;
};

class IMainWindowPlugin
{
public:
	virtual QObject *instance() = 0;
	virtual IMainWindow *mainWindow() const = 0;
};

Q_DECLARE_INTERFACE(IMainTabPage,"Vacuum.Plugin.IMainTabPage/1.0")
Q_DECLARE_INTERFACE(IMainTabWidget,"Vacuum.Plugin.IMainTabWidget/1.0")
Q_DECLARE_INTERFACE(IMainCentralPage,"Vacuum.Plugin.IMainCentralPage/1.0")
Q_DECLARE_INTERFACE(IMainCentralWidget,"Vacuum.Plugin.IMainCentralWidget/1.0")
Q_DECLARE_INTERFACE(IMainWindow,"Vacuum.Plugin.IMainWindow/1.1")
Q_DECLARE_INTERFACE(IMainWindowPlugin,"Vacuum.Plugin.IMainWindowPlugin/1.1")

#endif // IMAINWINDOW_H
