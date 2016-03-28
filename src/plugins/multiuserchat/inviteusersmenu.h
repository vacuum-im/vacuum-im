#ifndef INVITEUSERSMENU_H
#define INVITEUSERSMENU_H

#include <interfaces/imultiuserchat.h>
#include <interfaces/iservicediscovery.h>
#include <utils/pluginhelper.h>
#include <utils/menu.h>

class InviteUsersMenu :
	public Menu
{
	Q_OBJECT;
public:
	InviteUsersMenu(IMessageWindow *AWindow, QWidget *AParent=NULL);
	~InviteUsersMenu();
	IMessageWindow *messageWindow() const;
public:
	QSize sizeHint() const;
signals:
	void inviteAccepted(const QMultiMap<Jid, Jid> &AAddresses);
protected slots:
	void onAboutToShow();
	void onMultiUserChatStateChanged(int AState);
	void onDiscoInfoChanged(const IDiscoInfo &AInfo);
	void onMessageWindowAddressChanged(const Jid &AStreamBefore, const Jid &AContactBefore);
protected slots:
	void onInviteUsersWidgetAccepted(const QMultiMap<Jid, Jid> &AAddresses);
	void onInviteUsersWidgetRejected();
private:
	IMessageWindow *FWindow;
	PluginPointer<IServiceDiscovery> FDiscovery;
};

#endif // INVITEUSERSMENU_H
