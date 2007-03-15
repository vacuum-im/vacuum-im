#ifndef MENU_H
#define MENU_H

#include <QMenu>
#include <QMultiMap>
#include <QPointer>
#include "utilsexport.h"
#include "action.h"

class Action;

class UTILS_EXPORT Menu : 
  public QMenu
{
  Q_OBJECT;

public:
  Menu(int AOrder, QWidget *AParent = NULL);
  ~Menu();

  int order() const { return FOrder; }
  void addAction(Action *AAction);
  void addMenuActions(const Menu *AMenu);
  void removeAction(Action *AAction);
  void clear();
  Action *menuAction();
  void setIcon(const QIcon &AIcon);
  void setTitle(const QString &ATitle);
signals:
  void addedAction(QAction *);
  void removedAction(QAction *);
protected slots:
  void onMenuActionTriggered(bool);
private:
  typedef QMultiMap<int,QPointer<Action>> ActionList;
  ActionList::iterator findAction(const Action *AAction); 
  void clearNullActions();
  QAction *findOrderSeparator(int AOrder) const;
private:
  int FOrder;
  Action *FMenuAction;
  ActionList FActions;
};

#endif // MENU_H
