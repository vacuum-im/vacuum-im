#ifndef INVITEUSERSMENU_H
#define INVITEUSERSMENU_H

#include <interfaces/imultiuserchat.h>
#include <utils/menu.h>

class InviteUsersMenu :
	public Menu
{
	Q_OBJECT;
public:
	InviteUsersMenu(IMultiUserChatWindow *AWindow, QWidget *AParent = NULL);
	~InviteUsersMenu();
public:
	QSize sizeHint() const;
signals:
	void inviteAccepted(const QMultiMap<Jid, Jid> &AAddresses);
protected slots:
	void onAboutToShow();
protected slots:
	void onInviteUsersWidgetAccepted(const QMultiMap<Jid, Jid> &AAddresses);
	void onInviteUsersWidgetRejected();
private:
	IMultiUserChatWindow *FMucWindow;
};

#endif // INVITEUSERSMENU_H
