#ifndef ACCOUNTOPTIONS_H
#define ACCOUNTOPTIONS_H

#include <QWidget>
#include <interfaces/iaccountmanager.h>
#include <interfaces/ioptionsmanager.h>
#include "ui_accountoptions.h"

class AccountOptions :
	public QWidget,
	public IOptionsDialogWidget
{
	Q_OBJECT;
	Q_INTERFACES(IOptionsDialogWidget);
public:
	AccountOptions(IAccountManager *AManager, const QUuid &AAccountId, QWidget *AParent);
	~AccountOptions();
	virtual QWidget* instance() { return this; }
public slots:
	virtual void apply();
	virtual void reset();
signals:
	void modified();
	void childApply();
	void childReset();
private:
	Ui::AccountOptionsClass ui;
private:
	IAccountManager *FManager;
private:
	QUuid FAccountId;
	IAccount *FAccount;
};

#endif // ACCOUNTOPTIONS_H
