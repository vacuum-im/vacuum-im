#ifndef MENU_H
#define MENU_H

#include <QMenu>
#include <QMultiMap>
#include <QPointer>
#include "utilsexport.h"
#include "action.h"

class UTILS_EXPORT Menu : 
  public QMenu
{
  Q_OBJECT;

public:
  Menu(int AOrder, const QString &AMenuId, QWidget *AParent = NULL);
  ~Menu();

  int order() const { return FOrder; }
  const QString &menuId() const { return FMenuId; }
  void addContext(const ActionContext &AContext);
  void addAction(Action *AAction);
  void addMenuActions(const Menu *AMenu);
  void removeAction(Action *AAction);
  void clear();
  Action *menuAction();
signals:
  void setContext(Menu *);
  void addedAction(QAction *);
  void removedAction(QAction *);
protected slots:
  void onAboutToShow();
protected:
  void clearContext();
  void populateContext();
private:
  typedef QMultiMap<int,QPointer<Action>> ActionList;
  ActionList::iterator findAction(const Action *AAction); 
  void clearNullActions();
  QAction *findOrderSeparator(int AOrder) const;
private:
  int FOrder;
  QString FMenuId;
  Action *FMenuAction;
  ActionList FActions;
  ActionContext FContext;
};

#endif // MENU_H
