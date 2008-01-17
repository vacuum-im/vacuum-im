#ifndef IMAINWINDOW_H
#define IMAINWINDOW_H

#include <QMainWindow>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QToolBar>
#include "../../utils/menu.h"
#include "../../utils/toolbarchanger.h"

#define MAINWINDOW_UUID "{A6F3D775-8464-4599-AB79-97BA1BAA6E96}"

class IMainWindow :
  public QMainWindow
{
public:
  IMainWindow(QWidget *AParent = NULL, Qt::WindowFlags AFlags = 0) : QMainWindow(AParent,AFlags) {} 
public:
  virtual QVBoxLayout *mainLayout() const =0;
  virtual QStackedWidget *upperWidget() const = 0;
  virtual QStackedWidget *rostersWidget() const = 0;
  virtual QStackedWidget *bottomWidget() const = 0;
  virtual ToolBarChanger *topToolBarChanger() const =0;
  virtual ToolBarChanger *leftToolBarChanger() const =0;
  virtual ToolBarChanger *bottomToolBarChanger() const =0;
  virtual Menu *mainMenu() const = 0;
};

class IMainWindowPlugin 
{
public:
  virtual QObject *instance() = 0;
  virtual IMainWindow *mainWindow() const = 0;
};

Q_DECLARE_INTERFACE(IMainWindow,"Vacuum.Plugin.IMainWindow/1.0")
Q_DECLARE_INTERFACE(IMainWindowPlugin,"Vacuum.Plugin.IMainWindowPlugin/1.0")

#endif
