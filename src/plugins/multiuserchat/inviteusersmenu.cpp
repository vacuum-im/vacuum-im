#include "inviteusersmenu.h"

#include <QVBoxLayout>
#include "inviteuserswidget.h"

InviteUsersMenu::InviteUsersMenu(IMultiUserChatWindow *AWindow, QWidget *AParent) : Menu(AParent)
{
	FMucWindow = AWindow;

	setLayout(new QVBoxLayout());
	layout()->setMargin(0);

	connect(this,SIGNAL(aboutToShow()),SLOT(onAboutToShow()));
}

InviteUsersMenu::~InviteUsersMenu()
{

}

QSize InviteUsersMenu::sizeHint() const
{
	return layout()->sizeHint();
}

void InviteUsersMenu::onAboutToShow()
{
	InviteUsersWidget *widget = new InviteUsersWidget(FMucWindow,this);
	connect(widget,SIGNAL(inviteAccepted(const QMultiMap<Jid,Jid> &)),SLOT(onInviteUsersWidgetAccepted(const QMultiMap<Jid,Jid> &)));
	connect(widget,SIGNAL(inviteRejected()),SLOT(onInviteUsersWidgetRejected()));

	layout()->addWidget(widget);
	connect(this,SIGNAL(aboutToHide()),widget,SLOT(deleteLater()));
}

void InviteUsersMenu::onInviteUsersWidgetAccepted(const QMultiMap<Jid, Jid> &AAddresses)
{
	emit inviteAccepted(AAddresses);
	close();
}

void InviteUsersMenu::onInviteUsersWidgetRejected()
{
	close();
}
