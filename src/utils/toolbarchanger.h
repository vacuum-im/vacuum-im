#ifndef TOOLBARCHANGER_H
#define TOOLBARCHANGER_H

#include <QEvent>
#include <QToolBar>
#include <QToolButton>
#include "utilsexport.h"
#include "menu.h"

class UTILS_EXPORT ToolBarChanger : 
  public QObject
{
  Q_OBJECT;
public:
  ToolBarChanger(QToolBar *AToolBar);
  ~ToolBarChanger();
  bool isEmpty() const;
  bool separatorsVisible() const;
  void setSeparatorsVisible(bool ASeparatorsVisible);
  bool manageVisibitily() const;
  void setManageVisibility(bool AManageVisibility);
  QToolBar *toolBar() const;
  int actionGroup(const Action *AAction) const;
  QList<Action *> groupActions(int AGroup = AG_NULL) const;
  QList<Action *> findActions(const QMultiHash<int, QVariant> AData, bool ASearchInSubMenu = false) const;
  QAction *widgetAction(QWidget *AWidget) const;
  void addAction(Action *AAction, int AGroup = AG_DEFAULT, bool ASort = false);
  QToolButton *addToolButton(Action *AAction, int AGroup = AG_DEFAULT, bool ASort = false);
  void removeAction(Action *AAction);
  QAction *addWidget(QWidget *AWidget, int AGroup = AG_DEFAULT);
  void removeWidget(QWidget *AWidget);
  void clear();
signals:
  void actionInserted(QAction *ABefour, Action *AAction, int AGroup, bool ASort);
  void actionRemoved(Action *AAction);
  void widgetInserted(QAction *ABefour, QWidget *AWidget, QAction *AAction, int AGroup);
  void widgetRemoved(QWidget *AWidget, QAction *AAction);
  void separatorInserted(Action *ABefour, QAction *ASeparator);
  void separatorRemoved(QAction *ASeparator);
  void toolBarChangerDestroyed(ToolBarChanger *AToolBarChanger);
protected:
  void updateVisible();
protected:
  virtual bool eventFilter(QObject *AObject, QEvent *AEvent);
protected slots:
  void onActionInserted(QAction *ABefour, Action *AAction, int AGroup, bool ASort);
  void onActionRemoved(Action *AAction);
  void onSeparatorInserted(Action *ABefour, QAction *ASeparator);
  void onSeparatorRemoved(QAction *ASeparator);
  void onWidgetDestroyed(QObject *AObject);
  void onChangeVisible();
private:
  bool FSeparatorsVisible;
  bool FManageVisibility;
  bool FVisibleTimerStarted;
  int FChangingIntVisible;
  bool FIntVisible;
  bool FExtVisible;
  QToolBar *FToolBar;
  Menu *FToolBarMenu;
  QHash<QWidget *, QAction *> FWidgetActions;
  QHash<QAction *, QAction *> FBarSepByMenuSep;
  QHash<Action *, QToolButton *> FActionButtons;
};

#endif // TOOLBARCHANGER_H
