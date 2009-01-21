#ifndef TABWINDOW_H
#define TABWINDOW_H

#include <QMainWindow>
#include <QToolButton>
#include "../../definations/resources.h"
#include "../../definations/menuicons.h"
#include "../../interfaces/imessenger.h"
#include "../../interfaces/isettings.h"
#include "../../utils/action.h"
#include "../../utils/menu.h"
#include "ui_tabwindow.h"

class TabWindow : 
  public ITabWindow
{
  Q_OBJECT;
  Q_INTERFACES(ITabWindow);
public:
  TabWindow(IMessenger *AMessenger, int AWindowId);
  ~TabWindow();
  virtual void showWindow();
  virtual int windowId() const { return FWindowId; }
  virtual void addWidget(ITabWidget *AWidget);
  virtual bool hasWidget(ITabWidget *AWidget) const;
  virtual ITabWidget *currentWidget() const;
  virtual void setCurrentWidget(ITabWidget *AWidget);
  virtual void removeWidget(ITabWidget *AWidget);
  virtual void clear();
signals:
  virtual void widgetAdded(ITabWidget *AWidget);
  virtual void currentChanged(ITabWidget *AWidget);
  virtual void widgetRemoved(ITabWidget *AWidget);
  virtual void windowChanged();
  virtual void windowDestroyed();
protected:
  void initialize();
  void saveWindowState();
  void loadWindowState();
  void updateWindow();
  void updateTab(int AIndex);
protected:
  virtual void closeEvent(QCloseEvent *AEvent);
protected slots:
  void onTabChanged(int AIndex);
  void onCloseAction(bool);
  void onTabWidgetShow();
  void onTabWidgetClose();
  void onTabWidgetChanged();
  void onTabWidgetDestroyed();
  void onTabWindowCreated(ITabWindow *AWindow);
  void onTabWindowChanged();
  void onTabWindowDestroyed(ITabWindow *AWindow);
  void onTabWindowAction(bool);
  void onNewTabWindowAction(bool);
  void onDetachWindowAction(bool);
private:
  Ui::TabWindowClass ui;
private:
  IMessenger *FMessenger;
  ISettings *FSettings;
private:
  QToolButton *FCloseButton;
  Menu *FTabMenu;
  Action *FCloseAction;
  Action *FNewTabAction;
  Action *FDetachWindowAction;
  QHash<int,Action *> FTabActions;
private:
  int FWindowId;
};

#endif // TABWINDOW_H
