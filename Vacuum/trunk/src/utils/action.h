#ifndef ACTION_H
#define ACTION_H

#include <QAction>
#include <QHash>
#include <QVariant>
#include "utilsexport.h"
#include "menu.h"

class Menu;

class UTILS_EXPORT Action : 
  public QAction
{
  Q_OBJECT;

public:
  enum DataRoles {
    DR_Parametr1,
    DR_Parametr2,
    DR_Parametr3,
    DR_Parametr4,
    DR_StreamJid,
    DR_UserDefined = 64,
    DR_UserDynamic = DR_UserDefined + 1048576 
  }; 

public:
  Action(QObject *AParent = NULL);
  ~Action();

  void setData(int ARole, const QVariant &AData);
  void setData(const QHash<int,QVariant> &AData);
  QVariant data(int ARole) const;
  void setMenu(Menu *AMenu);
  Menu *menu() const { return FMenu; }
  static int newRole();
signals:
  void actionDestroyed(Action *);
protected slots:
  void onMenuDestroyed(Menu *AMenu);
private:
  static int FNewRole;
  Menu *FMenu;
  QHash<int,QVariant> FData;
};

#endif // ACTION_H
