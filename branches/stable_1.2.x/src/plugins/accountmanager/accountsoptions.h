#ifndef ACCOUNTSOPTIONS_H
#define ACCOUNTSOPTIONS_H

#include <QMap>
#include <QWidget>
#include <definitions/optionvalues.h>
#include <interfaces/iaccountmanager.h>
#include <interfaces/ioptionsmanager.h>
#include <utils/options.h>
#include "ui_accountsoptions.h"
#include "accountmanager.h"

class AccountManager;

class AccountsOptions :
			public QWidget,
			public IOptionsWidget
{
	Q_OBJECT;
	Q_INTERFACES(IOptionsWidget);
public:
	AccountsOptions(AccountManager *AManager, QWidget *AParent);
	~AccountsOptions();
	virtual QWidget* instance() { return this; }
public slots:
	virtual void apply();
	virtual void reset();
signals:
	void modified();
	void childApply();
	void childReset();
protected:
	QTreeWidgetItem *appendAccount(const QUuid &AAccountId, const QString &AName);
	void removeAccount(const QUuid &AAccountId);
protected slots:
	void onAddButtonClicked(bool);
	void onRemoveButtonClicked(bool);
	void onItemDoubleClicked(QTreeWidgetItem *AItem, int AColumn);
	void onAccountOptionsChanged(IAccount *AAcount, const OptionsNode &ANode);
private:
	Ui::AccountsOptionsClass ui;
private:
	AccountManager *FManager;
private:
	QList<QUuid> FPendingAccounts;
	QMap<QUuid, QTreeWidgetItem *> FAccountItems;
};

#endif // ACCOUNTSOPTIONS_H
