#ifndef TOOLBARCHANGER_H
#define TOOLBARCHANGER_H

#include <QToolBar>
#include "utilsexport.h"
#include "menu.h"

class UTILS_EXPORT ToolBarChanger : 
  public QObject
{
  Q_OBJECT;
public:
  ToolBarChanger(QToolBar *AToolBar);
  ~ToolBarChanger();
  QToolBar *toolBar() const { return FToolBar; }
  void addAction(Action *AAction, int AGroup = AG_DEFAULT, bool ASort = false);
  QAction *addWidget(QWidget *AWidget, int AGroup = AG_DEFAULT);
  int actionGroup(const Action *AAction) const;
  QList<Action *> actions(int AGroup = AG_NULL) const;
  QList<Action *> findActions(const QMultiHash<int, QVariant> AData, bool ASearchInSubMenu = false) const;
  void removeAction(Action *AAction);
  void clear();
signals:
  void actionInserted(QAction *ABefour, Action *AAction);
  void separatorInserted(Action *ABefour, QAction *ASeparator);
  void actionRemoved(Action *AAction);
  void toolBarChangerDestroyed(ToolBarChanger *AToolBarChanger);
protected slots:
  void onActionInserted(QAction *ABefour, Action *AAction);
  void onSeparatorInserted(Action *ABefour, QAction *ASeparator);
  void onActionRemoved(Action *AAction);
private:
  QToolBar *FToolBar;
  Menu *FToolBarMenu;
};

#endif // TOOLBARCHANGER_H
