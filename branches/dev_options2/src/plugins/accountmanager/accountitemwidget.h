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
	Jid streamJid() const;
	void setStreamJid(const Jid &AStreamJid);
signals:
	void modified();
	void removeClicked(const QUuid &AAccountId);
	void settingsClicked(const QUuid &AAccountId);
protected slots:
	void onRemoveButtonClicked();
	void onSettingsLinkActivated();
private:
	Ui::AccountItemWidget ui;
private:
	QUuid FAccountId;
	QString FName;
	Jid FStreamJid;
};

#endif // ACCOUNTITEMWIDGET_H
