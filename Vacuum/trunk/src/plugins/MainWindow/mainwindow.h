#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "interfaces/ipluginmanager.h"
#include "interfaces/imainwindow.h"
#include "utils/menu.h"

class MainWindow : 
  public IMainWindow
{
  Q_OBJECT;
  Q_INTERFACES(IMainWindow);

public:
  MainWindow(QWidget *AParent = NULL, Qt::WindowFlags AFlags = 0);
  ~MainWindow();

  bool init(IPluginManager *APluginManager);
  bool start();

  virtual QObject *instance() { return this; }

  //IMainWindow
  virtual QVBoxLayout *mainLayout() const { return FMainLayout; }
  virtual QStackedWidget *upperWidget() const { return FUpperWidget; }
  virtual QStackedWidget *middleWidget() const { return FMiddleWidget; }
  virtual QStackedWidget *bottomWidget() const { return FBottomWidget; }
  virtual QScrollArea *rostersArea() const { return FRostersArea; }
  virtual QVBoxLayout *rostersLayout() const { return FRostersLayout; }
  virtual QToolBar *mainToolBar() const { return FMainToolBar; }
  virtual Menu *mainMenu() const { return mnuMain; }
protected:
  void createLayouts();
  void createToolBars(); 
  void createActions();
  void connectActions();
protected:
  Menu *mnuMain;
  Menu *mnuAbout;
protected:
  Action *actQuit;
  Action *actAbout;
private:
  IPluginManager *FPluginManager;
private:
  QVBoxLayout     *FMainLayout;
  QStackedWidget  *FUpperWidget;
  QStackedWidget  *FMiddleWidget;
  QStackedWidget  *FBottomWidget;
  QScrollArea     *FRostersArea;
  QVBoxLayout     *FRostersLayout;
  QToolBar        *FMainToolBar;
};

#endif // MAINWINDOW_H
