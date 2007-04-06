#ifndef MENU_H
#define MENU_H

#include <QMenu>
#include <QMultiMap>
#include <QPointer>
#include "utilsexport.h"
#include "action.h"

#define DEFAULT_ACTION_GROUP 500
#define NULL_ACTION_GROUP -1

class Action;

class UTILS_EXPORT Menu : 
  public QMenu
{
  Q_OBJECT;

public:
  Menu(QWidget *AParent = NULL);
  ~Menu();

  void addAction(Action *AAction, int AGroup = DEFAULT_ACTION_GROUP, bool ASort = false);
  void addMenuActions(const Menu *AMenu, int AGroup = DEFAULT_ACTION_GROUP, bool ASort = false);
  void removeAction(Action *AAction);
  int actionGroup(const Action *AAction) const;
  QList<Action *> actions(int AGroup = NULL_ACTION_GROUP) const;
  void clear();
  Action *menuAction();
  void setIcon(const QIcon &AIcon);
  void setTitle(const QString &ATitle);
signals:
  void addedAction(QAction *);
  void removedAction(QAction *);
  void menuDestroyed(Menu *);
protected slots:
  void onActionDestroyed(Action *AAction);
private:
  typedef QMultiMap<int,Action *> ActionList;
  ActionList FActions;
  QMap<int,QAction *> FSeparators;
  Action *FMenuAction;
};

#endif // MENU_H
