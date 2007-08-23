#ifndef ACTION_H
#define ACTION_H

#include <QAction>
#include <QHash>
#include <QVariant>
#include "utilsexport.h"
#include "menu.h"
#include "skin.h"

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
    DR_SortString,
    DR_UserDefined = 64,
    DR_UserDynamic = DR_UserDefined + 1048576 
  }; 

public:
  Action(QObject *AParent = NULL);
  ~Action();
  //QAction
  Menu *menu() const { return FMenu; }
  void setIcon(const QIcon &AIcon);
  void setIcon(const QString &AIconsetFile, const QString &AIconName);
  void setMenu(Menu *AMenu);
  //Action
  void setData(int ARole, const QVariant &AData);
  void setData(const QHash<int,QVariant> &AData);
  QVariant data(int ARole) const;
  static int newRole();
signals:
  void actionDestroyed(Action *);
protected slots:
  void onMenuDestroyed(Menu *AMenu);
  void onSkinChanged();
private:
  static int FNewRole;
  Menu *FMenu;
  QHash<int,QVariant> FData;
  SkinIconset *FIconset;
  QString FIconName;
};

#endif // ACTION_H
