#include "usercontextmenu.h"

UserContextMenu::UserContextMenu(IMultiUserChatWindow *AMUCWindow, IMessageChatWindow *AChatWindow) : Menu(AChatWindow->menuBarWidget()->menuBarChanger()->menuBar())
{
	FChatWindow = AChatWindow;
	FMUCWindow = AMUCWindow;
	setTitle(FChatWindow->contactJid().resource());

	connect(this,SIGNAL(aboutToShow()),SLOT(onAboutToShow()));
	connect(this,SIGNAL(aboutToHide()),SLOT(onAboutToHide()));
	connect(FMUCWindow->multiUserChat()->instance(),SIGNAL(userPresence(IMultiUser *, int, const QString &)),SLOT(onMultiUserPresence(IMultiUser *, int, const QString &)));
	connect(FChatWindow->address()->instance(),SIGNAL(addressChanged(const Jid &,const Jid &)),SLOT(onChatWindowAddressChanged(const Jid &,const Jid &)));
}

UserContextMenu::~UserContextMenu()
{

}

void UserContextMenu::onAboutToShow()
{
	IMultiUser *user = FMUCWindow->multiUserChat()->userByNick(FChatWindow->contactJid().resource());
	FMUCWindow->contextMenuForUser(user,this);
}

void UserContextMenu::onAboutToHide()
{
	clear();
}

void UserContextMenu::onMultiUserPresence(IMultiUser *AUser, int AShow, const QString &/*AStatus*/)
{
	if (AUser->nickName() == FChatWindow->contactJid().resource())
		menuAction()->setVisible(AShow!=IPresence::Offline && AShow!=IPresence::Error);
}

void UserContextMenu::onChatWindowAddressChanged(const Jid &AStreamBefore, const Jid &AContactBefore)
{
	Q_UNUSED(AStreamBefore); Q_UNUSED(AContactBefore);
	setTitle(FChatWindow->contactJid().resource());
}
