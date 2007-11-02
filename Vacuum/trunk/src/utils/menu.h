#ifndef MENU_H
#define MENU_H

#include <QMenu>
#include <QMultiMap>
#include <QPointer>
#include "../../definations/actiongroups.h"
#include "utilsexport.h"
#include "action.h"
#include "skin.h"

class Action;

class UTILS_EXPORT Menu : 
  public QMenu
{
  Q_OBJECT;

public:
  Menu(QWidget *AParent = NULL);
  ~Menu();

  //QMenu
  Action *menuAction();
  bool isEmpty() const { return FActions.isEmpty(); }
  void addAction(Action *AAction, int AGroup = AG_DEFAULT, bool ASort = false);
  void removeAction(Action *AAction);
  void clear();
  void setIcon(const QIcon &AIcon);
  void setIcon(const QString &AIconsetFile, const QString &AIconName);
  void setTitle(const QString &ATitle);
  //Menu
  void addMenuActions(const Menu *AMenu, int AGroup = AG_DEFAULT, bool ASort = false);
  int actionGroup(const Action *AAction) const;
  QAction *nextGroupSeparator(int AGroup) const;
  QList<Action *> actions(int AGroup = AG_NULL) const;
  QList<Action *> findActions(const QMultiHash<int, QVariant> AData, bool ASearchInSubMenu = false) const;
signals:
  void actionInserted(QAction *ABefour, Action *AAction);
  void separatorInserted(Action *ABefour, QAction *ASeparator);
  void separatorRemoved(QAction *ASeparator);
  void actionRemoved(Action *AAction);
  void menuDestroyed(Menu *AMenu);
protected slots:
  void onActionDestroyed(Action *AAction);
  void onIconsetChanged();
private:
  Action *FMenuAction;
  SkinIconset *FIconset;
private:
  typedef QMultiMap<int,Action *> ActionList;
  ActionList FActions;
  QString FIconName;
  QMap<int,QAction *> FSeparators;
};

#endif // MENU_H
