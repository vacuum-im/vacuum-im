#ifndef ACCOUNTSOPTIONS_H
#define ACCOUNTSOPTIONS_H

#include <QMap>
#include <QWidget>
#include <QVBoxLayout>
#include <interfaces/iroster.h>
#include <interfaces/ipresence.h>
#include <interfaces/istatusicons.h>
#include <interfaces/iaccountmanager.h>
#include <interfaces/ioptionsmanager.h>
#include "accountitemwidget.h"
#include "ui_accountsoptionswidget.h"

class AccountManager;

class AccountsOptionsWidget :
	public QWidget,
	public IOptionsDialogWidget
{
	Q_OBJECT;
	Q_INTERFACES(IOptionsDialogWidget);
public:
	AccountsOptionsWidget(AccountManager *AManager, QWidget *AParent);
	~AccountsOptionsWidget();
	virtual QWidget* instance() { return this; }
public slots:
	virtual void apply();
	virtual void reset();
signals:
	void modified();
	void childApply();
	void childReset();
protected:
	void filterAccountItemWidgets() const;
	bool isInactiveAccountsHidden() const;
	void setInactiveAccounsHidden(bool AHidden);
	AccountItemWidget *accountItemWidgetAt(const QPoint &APos) const;
	AccountItemWidget *getAccountItemWidget(const QUuid &AAccountId);
	void updateAccountItemWidget(AccountItemWidget *AItem, IAccount *AAccount) const;
	void removeAccountItemWidget(const QUuid &AAccountId);
protected:
	void mousePressEvent(QMouseEvent *AEvent);
	void mouseMoveEvent(QMouseEvent *AEvent);
	void mouseReleaseEvent(QMouseEvent *AEvent);
	void dragEnterEvent(QDragEnterEvent *AEvent);
	void dragMoveEvent(QDragMoveEvent *AEvent);
protected slots:
	void onAddAccountLinkActivated();
	void onHideShowInactiveAccountsLinkActivated();
	void onRemoveButtonClicked(const QUuid &AAccountId);
	void onSettingsButtonClicked(const QUuid &AAccountId);
	void onAccountInserted(IAccount *AAccount);
	void onAccountOptionsChanged(IAccount *AAcount, const OptionsNode &ANode);
private:
	Ui::AccountsOptionsClass ui;
private:
	IStatusIcons *FStatusIcons;
	AccountManager *FAccountManager;
	IOptionsManager *FOptionsManager;
private:
	QPoint FDragStartPos;
	AccountItemWidget *FDragItem;
private:
	QVBoxLayout *FLayout;
	QMap<QUuid, AccountItemWidget *> FAccountItems;
};

#endif // ACCOUNTSOPTIONS_H
