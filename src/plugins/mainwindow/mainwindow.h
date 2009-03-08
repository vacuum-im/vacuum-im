#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "../../interfaces/imainwindow.h"
#include "../../definations/resources.h"
#include "../../definations/menuicons.h"

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
  virtual QVBoxLayout *mainLayout() const { return FMainLayout; }
  virtual QStackedWidget *upperWidget() const { return FUpperWidget; }
  virtual QStackedWidget *rostersWidget() const { return FRostersWidget; }
  virtual QStackedWidget *bottomWidget() const { return FBottomWidget; }
  virtual ToolBarChanger *topToolBarChanger() const { return FTopToolBarChanger; }
  virtual ToolBarChanger *leftToolBarChanger() const { return FLeftToolBarChanger; }
  virtual ToolBarChanger *bottomToolBarChanger() const { return FBottomToolBarChanger; }
  virtual Menu *mainMenu() const { return FMainMenu; }
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
