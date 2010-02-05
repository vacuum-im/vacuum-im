#ifndef USERCONTEXTMENU_H
#define USERCONTEXTMENU_H

#include <definations/rosterindextyperole.h>
#include <definations/rosterlabelorders.h>
#include <interfaces/irostersview.h>
#include <interfaces/irostersmodel.h>
#include <interfaces/imessagewidgets.h>
#include <utils/menu.h>

class UserContextMenu : 
  public Menu
{
  Q_OBJECT;
public:
  UserContextMenu(IRostersModel *AModel, IRostersView *AView, IChatWindow *AWindow);
  ~UserContextMenu();
protected:
  bool isAcceptedIndex(IRosterIndex *AIndex);
  void updateMenu();
protected slots:
  void onAboutToShow();
  void onAboutToHide();
  void onRosterIndexInserted(IRosterIndex *AIndex);
  void onRosterIndexDataChanged(IRosterIndex *AIndex, int ARole);
  void onRosterIndexRemoved(IRosterIndex *AIndex);
  void onChatWindowContactJidChanged(const Jid &ABefour);
private:
  IRosterIndex *FRosterIndex;
  IRostersModel *FRostersModel;
  IRostersView *FRostersView;
  IChatWindow *FChatWindow;
};

#endif // USERCONTEXTMENU_H
