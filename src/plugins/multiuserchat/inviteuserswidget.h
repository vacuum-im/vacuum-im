#ifndef INVITEUSERSWIDGET_H
#define INVITEUSERSWIDGET_H

#include <QWidget>
#include <QSortFilterProxyModel>
#include <interfaces/imultiuserchat.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/iservicediscovery.h>
#include "ui_inviteuserswidget.h"

class InviteUsersWidget : 
	public QWidget
{
	Q_OBJECT;
public:
	InviteUsersWidget(IMessageWindow *AWindow, QWidget *AParent = NULL);
	~InviteUsersWidget();
public:
	QSize sizeHint() const;
signals:
	void inviteAccepted(const QMultiMap<Jid, Jid> &AAddresses);
	void inviteRejected();
protected slots:
	void onDialogButtonsAccepted();
	void onDialogButtonsRejected();
private:
	Ui::InviteUsersWidgetClass ui;
private:
	IMessageReceiversWidget *FReceiversWidget;
};

#endif // INVITEUSERSWIDGET_H
