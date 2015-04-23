#ifndef ACCOUNTITEMWIDGET_H
#define ACCOUNTITEMWIDGET_H

#include <QUuid>
#include <QWidget>
#include <utils/jid.h>
#include "ui_accountitemwidget.h"

class AccountItemWidget :
	public QWidget
{
	Q_OBJECT;
public:
	AccountItemWidget(const QUuid &AAccountId, QWidget *AParent = NULL);
	~AccountItemWidget();
	QUuid accountId() const;
	bool isActive() const;
	void setActive(bool AActive);
	void setIcon(const QIcon &AIcon);
	QString name() const;
	void setName(const QString &AName);
	Jid accountJid() const;
	void setAccountJid(const Jid &AAccountJid);
signals:
	void modified();
	void removeClicked(const QUuid &AAccountId);
	void settingsClicked(const QUuid &AAccountId);
protected:
	void enterEvent(QEvent *AEvent);
	void leaveEvent(QEvent *AEvent);
protected slots:
	void onRemoveButtonClicked();
	void onSettingsLinkActivated();
private:
	Ui::AccountItemWidget ui;
private:
	QString FName;
	Jid FAccountJid;
	QUuid FAccountId;
};

#endif // ACCOUNTITEMWIDGET_H
