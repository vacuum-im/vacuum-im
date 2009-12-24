#ifndef TABWINDOW_H
#define TABWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <definations/resources.h>
#include <definations/menuicons.h>
#include <definations/actiongroups.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/isettings.h>
#include <utils/widgetmanager.h>
#include "ui_tabwindow.h"

class TabWindow : 
  public QMainWindow,
  public ITabWindow
{
  Q_OBJECT;
  Q_INTERFACES(ITabWindow);
public:
  TabWindow(IMessageWidgets *AMessageWidgets, const QUuid &AWindowId);
  ~TabWindow();
  virtual QMainWindow *instance() { return this; }
  virtual void showWindow();
  virtual QUuid windowId() const;
  virtual QString windowName() const;
  virtual Menu *windowMenu() const;
  virtual void addPage(ITabWindowPage *APage);
  virtual bool hasPage(ITabWindowPage *APage) const;
  virtual ITabWindowPage *currentPage() const;
  virtual void setCurrentPage(ITabWindowPage *APage);
  virtual void detachPage(ITabWindowPage *APage);
  virtual void removePage(ITabWindowPage *APage);
  virtual void clear();
signals:
  virtual void pageAdded(ITabWindowPage *APage);
  virtual void currentPageChanged(ITabWindowPage *APage);
  virtual void pageRemoved(ITabWindowPage *APage);
  virtual void pageDetached(ITabWindowPage *APage);
  virtual void windowChanged();
  virtual void windowDestroyed();
protected:
  void initialize();
  void createActions();
  void saveWindowState();
  void loadWindowState();
  void updateWindow();
  void updateTab(int AIndex);
protected slots:
  void onTabChanged(int AIndex);
  void onTabCloseRequested(int AIndex);
  void onTabPageShow();
  void onTabPageClose();
  void onTabPageChanged();
  void onTabPageDestroyed();
  void onTabWindowAppended(const QUuid &AWindowId, const QString &AName);
  void onTabWindowNameChanged(const QUuid &AWindowId, const QString &AName);
  void onDefaultTabWindowChanged(const QUuid &AWindowId);
  void onTabWindowDeleted(const QUuid &AWindowId);
  void onActionTriggered(bool);
private:
  Ui::TabWindowClass ui;
private:
  IMessageWidgets *FMessageWidgets;
  ISettings *FSettings;
private:
  Menu *FWindowMenu;
  Menu *FJoinMenu;
  Action *FCloseTab;
  Action *FNextTab;
  Action *FPrevTab;
  Action *FNewTab;
  Action *FDetachWindow;
  Action *FShowCloseButtons;
  Action *FSetAsDefault;
  Action *FRenameWindow;
  Action *FDeleteWindow;
private:
  QUuid FWindowId;
};

#endif // TABWINDOW_H
