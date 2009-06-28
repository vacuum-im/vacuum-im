#ifndef STATUSBARCHANGER_H
#define STATUSBARCHANGER_H

#include <QEvent>
#include <QMultiMap>
#include <QStatusBar>
#include "utilsexport.h"

#define SBG_NULL        -1
#define SBG_DEFAULT     500

class UTILS_EXPORT StatusBarChanger : 
  public QObject
{
  Q_OBJECT;
public:
  StatusBarChanger(QStatusBar *AStatusBar);
  ~StatusBarChanger();
  bool manageVisibitily() const;
  void setManageVisibility(bool AManageVisibility);
  QStatusBar *statusBar() const;
  int widgetGroup(QWidget *AWidget) const;
  QList<QWidget *> groupWidgets(int AGroup = SBG_NULL) const;
  void insertWidget(QWidget *AWidget, int AGroup = SBG_DEFAULT, bool APermanent = false, int AStretch = 0);
  void removeWidget(QWidget *AWidget);
  void clear();
signals:
  void widgetInserted(QWidget *ABefour, QWidget *AWidget, int AGroup,bool APermanent, int AStretch);
  void widgetRemoved(QWidget *AWidget);
  void statusBarChangerDestroyed(StatusBarChanger *AStatusBarChanger);
protected slots:
  void updateVisible();
protected:
  virtual bool eventFilter(QObject *AObject, QEvent *AEvent);
protected slots:
  void onWidgetDestroyed(QObject *AObject);
  void onChangeVisible();
private:
  bool FIntVisible;
  bool FExtVisible;
  bool FManageVisibility;
  int FChangingIntVisible;
  bool FVisibleTimerStarted;
  QStatusBar *FStatusBar;
  QMultiMap<int, QWidget *> FWidgets;
};

#endif // STATUSBARCHANGER_H
