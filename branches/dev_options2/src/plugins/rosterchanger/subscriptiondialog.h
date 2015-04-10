#ifndef SUBSCRIPTIONDIALOG_H
#define SUBSCRIPTIONDIALOG_H

#include <interfaces/ipluginmanager.h>
#include <interfaces/inotifications.h>
#include <interfaces/imessageprocessor.h>
#include <interfaces/irosterchanger.h>
#include <interfaces/irostermanager.h>
#include <interfaces/ivcardmanager.h>
#include "ui_subscriptiondialog.h"

class SubscriptionDialog :
	public QDialog,
	public ISubscriptionDialog
{
	Q_OBJECT;
	Q_INTERFACES(ISubscriptionDialog);
public:
	SubscriptionDialog(IRosterChanger *ARosterChanger, IPluginManager *APluginManager, const Jid &AStreamJid, const Jid &AContactJid, const QString &ANotify, const QString &AMessage, QWidget *AParent = NULL);
	~SubscriptionDialog();
	//ISubscriptionDialog
	virtual QDialog *instance() { return this; }
	virtual Jid streamJid() const;
	virtual Jid contactJid() const;
	virtual QVBoxLayout *actionsLayout() const;
	virtual ToolBarChanger *toolBarChanger() const;
signals:
	void dialogDestroyed();
protected:
	void initialize(IPluginManager *APluginManager);
protected slots:
	void onDialogAccepted();
	void onDialogRejected();
	void onToolBarActionTriggered(bool);
private:
	Ui::SubscriptionDialogClass ui;
private:
	IRoster *FRoster;
	IVCardManager *FVCardManager;
	IRosterChanger *FRosterChanger;
	INotifications *FNotifications;
	IMessageProcessor *FMessageProcessor;
private:
	Action *FShowChat;
	Action *FSendMessage;
	Action *FShowVCard;
private:
	Jid FStreamJid;
	Jid FContactJid;
	ToolBarChanger *FToolBarChanger;
};

#endif // SUBSCRIPTIONDIALOG_H
