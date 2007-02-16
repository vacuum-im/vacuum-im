#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/imainwindow.h"
#include "../../interfaces/isettings.h"
#include "../../utils/menu.h"

class MainWindow : 
  virtual public QMainWindow,
  public IMainWindow
{
  Q_OBJECT;
  Q_INTERFACES(IMainWindow);

public:
  MainWindow(QWidget *AParent = NULL, Qt::WindowFlags AFlags = 0);
  ~MainWindow();

  bool init(IPluginManager *APluginManager, ISettings *ASettings);
  bool start();

  virtual QObject *instance() { return this; }

  //IMainWindow
  virtual QVBoxLayout *mainLayout() const { return FMainLayout; }
  virtual QStackedWidget *upperWidget() const { return FUpperWidget; }
  virtual QStackedWidget *rostersWidget() const { return FRostersWidget; }
  virtual QStackedWidget *bottomWidget() const { return FBottomWidget; }
  virtual QToolBar *mainToolBar() const { return FMainToolBar; }
  virtual Menu *mainMenu() const { return mnuMain; }
protected:
  void createLayouts();
  void createToolBars(); 
  void createMenus();
  void createActions();
  void connectActions();
protected slots:
  void onSettingsOpened();
  void onSettingsClosed();
protected:
  virtual void closeEvent(QCloseEvent *AEvent);
protected:
  Menu *mnuMain;
protected:
  Action *actQuit;
private:
  IPluginManager *FPluginManager;
  ISettings *FSettings;
private:
  QVBoxLayout     *FMainLayout;
  QStackedWidget  *FUpperWidget;
  QStackedWidget  *FRostersWidget;
  QStackedWidget  *FBottomWidget;
  QToolBar        *FMainToolBar;
};

#endif // MAINWINDOW_H
