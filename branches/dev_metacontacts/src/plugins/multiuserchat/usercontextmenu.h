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
	UserContextMenu(IMultiUserChatWindow *AMUCWindow, IMessageChatWindow *AChatWindow);
	~UserContextMenu();
protected slots:
	void onAboutToShow();
	void onAboutToHide();
	void onMultiUserPresence(IMultiUser *AUser, int AShow, const QString &AStatus);
	void onChatWindowAddressChanged(const Jid &AStreamBefore, const Jid &AContactBefore);
private:
	IMessageChatWindow *FChatWindow;
	IMultiUserChatWindow *FMUCWindow;
};

#endif // USERCONTEXTMENU_H
