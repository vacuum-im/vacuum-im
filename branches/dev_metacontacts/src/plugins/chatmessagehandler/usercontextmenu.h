#ifndef USERCONTEXTMENU_H
#define USERCONTEXTMENU_H

#include <definitions/rosterindexkinds.h>
#include <definitions/rosterindexroles.h>
#include <interfaces/irostersview.h>
#include <interfaces/irostersmodel.h>
#include <interfaces/imessagewidgets.h>
#include <utils/menu.h>

class UserContextMenu :
			public Menu
{
	Q_OBJECT;
public:
	UserContextMenu(IRostersModel *AModel, IRostersView *AView, IMessageChatWindow *AWindow);
	~UserContextMenu();
protected:
	bool isAcceptedIndex(IRosterIndex *AIndex);
	void updateMenu();
protected slots:
	void onAboutToShow();
	void onAboutToHide();
	void onRosterIndexInserted(IRosterIndex *AIndex);
	void onRosterIndexDataChanged(IRosterIndex *AIndex, int ARole);
	void onRosterIndexDestroyed(IRosterIndex *AIndex);
	void onChatWindowAddressChanged(const Jid &AStreamBefore, const Jid &AContactBefore);
private:
	IRosterIndex *FRosterIndex;
	IRostersModel *FRostersModel;
	IRostersView *FRostersView;
	IMessageChatWindow *FChatWindow;
};

#endif // USERCONTEXTMENU_H
