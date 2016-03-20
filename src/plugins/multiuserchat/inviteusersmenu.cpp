#include "inviteusersmenu.h"

#include <QVBoxLayout>
#include <definitions/namespaces.h>
#include "inviteuserswidget.h"

InviteUsersMenu::InviteUsersMenu(IMessageWindow *AWindow, QWidget *AParent) : Menu(AParent)
{
	FWindow = AWindow;

	IMultiUserChatWindow *mucWindow = AWindow!=NULL ? qobject_cast<IMultiUserChatWindow *>(AWindow->instance()) : NULL;
	if (mucWindow)
	{
		connect(mucWindow->multiUserChat()->instance(),SIGNAL(stateChanged(int)),SLOT(onMultiUserChatStateChanged(int)));
		onMultiUserChatStateChanged(mucWindow->multiUserChat()->state());
	}

	IMessageChatWindow *chatWindow = AWindow!=NULL ? qobject_cast<IMessageChatWindow *>(AWindow->instance()) : NULL;
	if (chatWindow)
	{
		if (FDiscovery)
		{
			connect(FDiscovery->instance(),SIGNAL(discoInfoReceived(const IDiscoInfo &)),SLOT(onDiscoInfoChanged(const IDiscoInfo &)));
			connect(FDiscovery->instance(),SIGNAL(discoInfoRemoved(const IDiscoInfo &)),SLOT(onDiscoInfoChanged(const IDiscoInfo &)));
		}
		connect(chatWindow->address()->instance(),SIGNAL(addressChanged(const Jid &, const Jid &)),SLOT(onMessageWindowAddressChanged(const Jid &, const Jid &)));
		onMessageWindowAddressChanged(Jid::null,Jid::null);
	}

	setLayout(new QVBoxLayout());
	layout()->setMargin(0);

	connect(this,SIGNAL(aboutToShow()),SLOT(onAboutToShow()));
}

InviteUsersMenu::~InviteUsersMenu()
{

}

IMessageWindow *InviteUsersMenu::messageWindow() const
{
	return FWindow;
}

QSize InviteUsersMenu::sizeHint() const
{
	return layout()->sizeHint();
}

void InviteUsersMenu::onAboutToShow()
{
	InviteUsersWidget *widget = new InviteUsersWidget(FWindow,this);
	connect(widget,SIGNAL(inviteAccepted(const QMultiMap<Jid,Jid> &)),SLOT(onInviteUsersWidgetAccepted(const QMultiMap<Jid,Jid> &)));
	connect(widget,SIGNAL(inviteRejected()),SLOT(onInviteUsersWidgetRejected()));

	layout()->addWidget(widget);
	connect(this,SIGNAL(aboutToHide()),widget,SLOT(deleteLater()));
}

void InviteUsersMenu::onMultiUserChatStateChanged(int AState)
{
	menuAction()->setEnabled(AState == IMultiUserChat::Opened);
}

void InviteUsersMenu::onDiscoInfoChanged(const IDiscoInfo &AInfo)
{
	if (FWindow->streamJid()==AInfo.streamJid && FWindow->contactJid()==AInfo.contactJid)
		menuAction()->setEnabled(AInfo.features.contains(NS_MUC));
}

void InviteUsersMenu::onMessageWindowAddressChanged(const Jid &AStreamBefore, const Jid &AContactBefore)
{
	Q_UNUSED(AStreamBefore); Q_UNUSED(AContactBefore);
	if (FDiscovery)
		menuAction()->setEnabled(FDiscovery->checkDiscoFeature(FWindow->streamJid(),FWindow->contactJid(),NS_MUC));
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
