#ifndef CHATWINDOWMENU_H
#define CHATWINDOWMENU_H

#include <interfaces/imessagearchiver.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/idataforms.h>
#include <interfaces/isessionnegotiation.h>
#include <interfaces/iservicediscovery.h>

class ChatWindowMenu :
	public Menu
{
	Q_OBJECT;
public:
	ChatWindowMenu(IMessageArchiver *AArchiver, IMessageToolBarWidget *AToolBarWidget, QWidget *AParent);
	~ChatWindowMenu();
	Jid streamJid() const;
	Jid contactJid() const;
protected:
	bool isOTRStanzaSession(const IStanzaSession &ASession) const;
	void restoreSessionPrefs(const Jid &AContactJid);
	void createActions();
	void updateMenu();
protected slots:
	void onActionTriggered(bool);
	void onArchivePrefsChanged(const Jid &AStreamJid);
	void onArchiveRequestCompleted(const QString &AId);
	void onArchiveRequestFailed(const QString &AId, const XmppError &AError);
	void onToolBarWidgetAddressChanged(const Jid &AStreamBefore, const Jid &AContactBefore);
private:
	IMessageArchiver *FArchiver;
	IMessageToolBarWidget *FToolBarWidget;
private:
	Action *FEnableArchiving;
	Action *FDisableArchiving;
private:
	QString FSaveRequest;
};

#endif // CHATWINDOWMENU_H
