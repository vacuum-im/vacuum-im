#ifndef USERCONTEXTMENU_H
#define USERCONTEXTMENU_H

#include <definitions/multiuserdataroles.h>
#include <interfaces/imultiuserchat.h>
#include <interfaces/ipresence.h>
#include <utils/menu.h>

class UserContextMenu :
			public Menu
{
	Q_OBJECT;
public:
	UserContextMenu(IMultiUserChatWindow *AMUCWindow, IChatWindow *AChatWindow);
	~UserContextMenu();
protected slots:
	void onAboutToShow();
	void onAboutToHide();
	void onMultiUserPresence(IMultiUser *AUser, int AShow, const QString &AStatus);
	void onChatWindowContactJidChanged(const Jid &ABefore);
private:
	IChatWindow *FChatWindow;
	IMultiUserChatWindow *FMUCWindow;
};

#endif // USERCONTEXTMENU_H
