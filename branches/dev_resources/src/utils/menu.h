#ifndef MENU_H
#define MENU_H

#include <QMenu>
#include <QMultiMap>
#include "utilsexport.h"
#include "action.h"
#include "iconstorage.h"

#define AG_NULL                   -1
#define AG_DEFAULT                500

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
  void setIcon(const QString &AStorageName, const QString &AIconKey, int AIconIndex = 0);
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
private:
  Action *FMenuAction;
  IconStorage *FIconStorage;
private:
  QMultiMap<int,Action *> FActions;
  QMap<int,QAction *> FSeparators;
};

#endif // MENU_H
