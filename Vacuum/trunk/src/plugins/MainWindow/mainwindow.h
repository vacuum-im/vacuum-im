#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "../../interfaces/imainwindow.h"
#include "../../utils/menu.h"

class MainWindow : 
  virtual public QMainWindow,
  public IMainWindow
{
  Q_OBJECT;
  Q_INTERFACES(IMainWindow);

public:
  MainWindow(Qt::WindowFlags AFlags = 0);
  ~MainWindow();

  virtual QObject *instance() { return this; }

  //IMainWindow
  virtual QVBoxLayout *mainLayout() const { return FMainLayout; }
  virtual QStackedWidget *upperWidget() const { return FUpperWidget; }
  virtual QStackedWidget *rostersWidget() const { return FRostersWidget; }
  virtual QStackedWidget *bottomWidget() const { return FBottomWidget; }
  virtual QToolBar *topToolBar() const { return FTopToolBar; }
  virtual QToolBar *bottomToolBar() const { return FBottomToolBar; }
  virtual Menu *mainMenu() const { return mnuMain; }
protected:
  void createLayouts();
  void createToolBars(); 
  void createMenus();
protected:
  virtual void closeEvent(QCloseEvent *AEvent);
private:
  Menu *mnuMain;
private:
  QVBoxLayout     *FMainLayout;
  QStackedWidget  *FUpperWidget;
  QStackedWidget  *FRostersWidget;
  QStackedWidget  *FBottomWidget;
  QToolBar        *FTopToolBar;
  QToolBar        *FBottomToolBar;
};

#endif // MAINWINDOW_H
