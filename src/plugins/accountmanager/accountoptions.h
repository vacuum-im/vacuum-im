#ifndef ACCOUNTOPTIONS_H
#define ACCOUNTOPTIONS_H

#include <QWidget>
#include <definations/version.h>
#include <definations/optionvalues.h>
#include <interfaces/iaccountmanager.h>
#include <interfaces/ioptionsmanager.h>
#include <utils/jid.h>
#include <utils/options.h>
#include "ui_accountoptions.h"

class AccountOptions :
			public QWidget,
			public IOptionsWidget
{
	Q_OBJECT;
	Q_INTERFACES(IOptionsWidget);
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
