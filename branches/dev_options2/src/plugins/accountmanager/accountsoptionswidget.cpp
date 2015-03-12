#include "accountsoptionswidget.h"

#include <QDrag>
#include <QPainter>
#include <QMimeData>
#include <QMouseEvent>
#include <QMessageBox>
#include <QTextDocument>
#include <definitions/optionnodes.h>
#include <definitions/optionvalues.h>
#include <utils/pluginhelper.h>
#include <utils/options.h>
#include "accountmanager.h"
#include "createaccountwizard.h"

AccountsOptionsWidget::AccountsOptionsWidget(AccountManager *AManager, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	setAcceptDrops(true);
	
	FDragItem = NULL;

	FAccountManager = AManager;
	FStatusIcons = PluginHelper::pluginInstance<IStatusIcons>();
	FOptionsManager = PluginHelper::pluginInstance<IOptionsManager>();

	FLayout = new QVBoxLayout(ui.wdtAccounts);
	FLayout->setMargin(0);

	ui.lblAddAccount->setText(QString("<a href='add_account'>%1</a>").arg(tr("Add Account...")));
	connect(ui.lblAddAccount,SIGNAL(linkActivated(const QString &)),SLOT(onAddAccountLinkActivated()));

	connect(ui.lblHideShowInactive,SIGNAL(linkActivated(const QString &)),SLOT(onHideShowInactiveAccountsLinkActivated()));

	connect(FAccountManager->instance(),SIGNAL(accountInserted(IAccount *)),SLOT(onAccountInserted(IAccount *)));
	connect(FAccountManager->instance(),SIGNAL(accountOptionsChanged(IAccount *, const OptionsNode &)),SLOT(onAccountOptionsChanged(IAccount *, const OptionsNode &)));

	reset();
}

AccountsOptionsWidget::~AccountsOptionsWidget()
{

}

void AccountsOptionsWidget::apply()
{
	QList<IAccount *> curAccounts;
	for (QMap<QUuid, AccountItemWidget *>::const_iterator it = FAccountItems.constBegin(); it!=FAccountItems.constEnd(); ++it)
	{
		IAccount *account = FAccountManager->findAccountById(it.key());
		if (account)
		{
			AccountItemWidget *item = it.value();
			account->optionsNode().setValue(FLayout->indexOf(item),"order");
			account->optionsNode().setValue(item->isActive(),"active");
			account->setActive(item->isActive());
			item->setActive(account->isActive());
			curAccounts.append(account);
		}
	}

	foreach(IAccount *account, FAccountManager->accounts())
		if (!curAccounts.contains(account))
			FAccountManager->destroyAccount(account->accountId());

	emit childApply();
}

void AccountsOptionsWidget::reset()
{
	QList<QUuid> curAccounts;
	QMap<int, AccountItemWidget *> sortedItems;
	foreach(IAccount *account, FAccountManager->accounts())
	{
		AccountItemWidget *item = getAccountItemWidget(account->accountId());
		updateAccountItemWidget(item,account);

		curAccounts.append(account->accountId());
		sortedItems.insert(account->optionsNode().value("order").toInt(), item);
	}

	foreach(const QUuid &accountId, FAccountItems.keys())
		if (!curAccounts.contains(accountId))
			removeAccountItemWidget(accountId);

	int index = 0;
	foreach(AccountItemWidget *item, sortedItems)
		FLayout->insertWidget(index++, item);

	setInactiveAccounsHidden(isInactiveAccountsHidden());

	emit childReset();
}

void AccountsOptionsWidget::filterAccountItemWidgets() const
{
	bool hidden = isInactiveAccountsHidden();
	foreach(AccountItemWidget *item, FAccountItems)
		item->setVisible(!hidden || item->isActive());
	ui.lblHideShowInactive->setText(QString("<a href='hide-show'>%1</a>").arg(hidden ? tr("Show inactive accounts") : tr("Hide inactive accounts")));
}

bool AccountsOptionsWidget::isInactiveAccountsHidden() const
{
	return Options::fileValue("accounts.accountsoptions.hide-inactive-accounts").toBool();
}

void AccountsOptionsWidget::setInactiveAccounsHidden(bool AHidden)
{
	Options::setFileValue(AHidden,"accounts.accountsoptions.hide-inactive-accounts");
	filterAccountItemWidgets();
}

AccountItemWidget *AccountsOptionsWidget::accountItemWidgetAt(const QPoint &APos) const
{
	QWidget *widget = childAt(APos);
	while (widget!=NULL && widget->parentWidget()!=ui.wdtAccounts)
		widget = widget->parentWidget();
	return qobject_cast<AccountItemWidget *>(widget);
}

AccountItemWidget *AccountsOptionsWidget::getAccountItemWidget(const QUuid &AAccountId)
{
	AccountItemWidget *item = FAccountItems.value(AAccountId);
	if (item == NULL)
	{
		item = new AccountItemWidget(AAccountId,ui.wdtAccounts);
		connect(item,SIGNAL(modified()),SIGNAL(modified()));
		connect(item,SIGNAL(removeClicked(const QUuid &)),SLOT(onRemoveButtonClicked(const QUuid &)));
		connect(item,SIGNAL(settingsClicked(const QUuid &)),SLOT(onSettingsButtonClicked(const QUuid &)));

		FLayout->addWidget(item);
		FAccountItems.insert(AAccountId,item);
	}
	return item;
}

void AccountsOptionsWidget::updateAccountItemWidget(AccountItemWidget *AItem, IAccount *AAccount) const
{
	AItem->setName(AAccount->name());
	AItem->setAccountJid(AAccount->accountJid());
	AItem->setActive(AAccount->optionsNode().value("active").toBool());
	AItem->setIcon(FStatusIcons!=NULL ? FStatusIcons->iconByJidStatus(AItem->accountJid(),IPresence::Online,SUBSCRIPTION_BOTH,false) : QIcon());
}

void AccountsOptionsWidget::removeAccountItemWidget(const QUuid &AAccountId)
{
	delete FAccountItems.take(AAccountId);
}

void AccountsOptionsWidget::mousePressEvent(QMouseEvent *AEvent)
{
	if (AEvent->button() == Qt::LeftButton)
	{
		AccountItemWidget *item = accountItemWidgetAt(AEvent->pos());
		if (item)
		{
			FDragItem = item;
			FDragStartPos = AEvent->pos();
		}
	}
}

void AccountsOptionsWidget::mouseMoveEvent(QMouseEvent *AEvent)
{
	if (FDragItem!=NULL && (AEvent->buttons() & Qt::LeftButton)>0 && (FDragStartPos-AEvent->pos()).manhattanLength()>=QApplication::startDragDistance())
	{
		QDrag *drag = new QDrag(this);
		drag->setMimeData(new QMimeData());
		drag->exec(Qt::MoveAction);
	}
}

void AccountsOptionsWidget::mouseReleaseEvent(QMouseEvent *AEvent)
{
	if (AEvent->button() == Qt::LeftButton)
		FDragItem = NULL;
}

void AccountsOptionsWidget::dragEnterEvent(QDragEnterEvent *AEvent)
{
	if (AEvent->source() == this)
		AEvent->acceptProposedAction();
	else
		AEvent->ignore();
}

void AccountsOptionsWidget::dragMoveEvent(QDragMoveEvent *AEvent)
{
	if (FDragItem!=NULL && AEvent->source()==this)
	{
		AccountItemWidget *item = accountItemWidgetAt(AEvent->pos());
		if (item!=NULL && item!=FDragItem)
		{
			FLayout->insertWidget(FLayout->indexOf(item),FDragItem);
			emit modified();
		}
	}
}

void AccountsOptionsWidget::onAddAccountLinkActivated()
{
	QWizard *wizard = new CreateAccountWizard(this);
	wizard->show();
}

void AccountsOptionsWidget::onHideShowInactiveAccountsLinkActivated()
{
	setInactiveAccounsHidden(!isInactiveAccountsHidden());
}

void AccountsOptionsWidget::onRemoveButtonClicked(const QUuid &AAccountId)
{
	AccountItemWidget *item = FAccountItems.value(AAccountId);
	if (item)
	{
		QMessageBox::StandardButton res = QMessageBox::warning(this, tr("Remove Account"),
			tr("You are assured that wish to remove an account <b>%1</b>?<br>All settings will be lost.").arg(Qt::escape(item->name())),
			QMessageBox::Ok | QMessageBox::Cancel);
		
		if (res == QMessageBox::Ok)
		{
			removeAccountItemWidget(AAccountId);
			emit modified();
		}
	}
}

void AccountsOptionsWidget::onSettingsButtonClicked(const QUuid &AAccountId)
{
	QString rootId = OPN_ACCOUNTS"."+AAccountId.toString();
	FOptionsManager->showOptionsDialog(QString::null, rootId, window());
}

void AccountsOptionsWidget::onAccountInserted(IAccount *AAccount)
{
	if (!FAccountItems.contains(AAccount->accountId()))
	{
		AccountItemWidget *item = getAccountItemWidget(AAccount->accountId());
		updateAccountItemWidget(item,AAccount);
		FLayout->addWidget(item);
	}
}

void AccountsOptionsWidget::onAccountOptionsChanged(IAccount *AAcount, const OptionsNode &ANode)
{
	AccountItemWidget *item = FAccountItems.value(AAcount->accountId());
	if (item)
	{
		if (AAcount->optionsNode().childPath(ANode) == "name")
			updateAccountItemWidget(item,AAcount);
		else if (AAcount->optionsNode().childPath(ANode) == "streamJid")
			updateAccountItemWidget(item,AAcount);
	}
}
