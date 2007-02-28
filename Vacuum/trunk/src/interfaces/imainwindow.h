#ifndef IMAINWINDOW_H
#define IMAINWINDOW_H

#include <QMainWindow>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QToolBar>
#include "../../utils/menu.h"

class IMainWindow :
  virtual public QMainWindow
{
public:
  virtual QObject *instance() = 0;
  virtual QVBoxLayout *mainLayout() const =0;
  virtual QStackedWidget *upperWidget() const = 0;
  virtual QStackedWidget *rostersWidget() const = 0;
  virtual QStackedWidget *bottomWidget() const = 0;
  virtual QToolBar *topToolBar() const =0;
  virtual QToolBar *bottomToolBar() const =0;
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