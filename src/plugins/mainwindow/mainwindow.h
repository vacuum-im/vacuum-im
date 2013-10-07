#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <interfaces/imainwindow.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>

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
	virtual Menu *mainMenu() const;
	virtual QVBoxLayout *mainLayout() const;
	virtual QStackedWidget *upperWidget() const;
	virtual QStackedWidget *rostersWidget() const;
	virtual QStackedWidget *bottomWidget() const;
	virtual ToolBarChanger *topToolBarChanger() const;
	virtual ToolBarChanger *leftToolBarChanger() const;
	virtual ToolBarChanger *bottomToolBarChanger() const;
public:
	virtual QMenu *createPopupMenu();
protected:
	void createLayouts();
	void createToolBars();
	void createMenus();
protected slots:
	void onStackedWidgetRemoved(int AIndex);
private:
	Menu           *FMainMenu;
	ToolBarChanger *FTopToolBarChanger;
	ToolBarChanger *FLeftToolBarChanger;
	ToolBarChanger *FBottomToolBarChanger;
private:
	QVBoxLayout    *FMainLayout;
	QStackedWidget *FUpperWidget;
	QStackedWidget *FRostersWidget;
	QStackedWidget *FBottomWidget;
};

#endif // MAINWINDOW_H
