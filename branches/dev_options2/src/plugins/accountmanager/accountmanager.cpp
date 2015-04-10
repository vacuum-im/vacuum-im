#include "accountmanager.h"

#include <QLineEdit>
#include <QComboBox>
#include <definitions/actiongroups.h>
#include <definitions/optionnodes.h>
#include <definitions/optionvalues.h>
#include <definitions/optionnodeorders.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/rosterindexkinds.h>
#include <definitions/rosterindexroles.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/version.h>
#include <utils/action.h>
#include <utils/logger.h>
#include "account.h"
#include "accountsoptionswidget.h"

#define ADR_ACCOUNT_ID              Action::DR_Parametr1

AccountManager::AccountManager()
{
	FXmppStreamManager = NULL;
	FOptionsManager = NULL;
	FRostersViewPlugin = NULL;
}

AccountManager::~AccountManager()
{

}

//IPlugin
void AccountManager::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Account Manager");
	APluginInfo->description = tr("Allows to create and manage Jabber accounts");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(XMPPSTREAMS_UUID);
}

bool AccountManager::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);

	IPlugin *plugin = APluginManager->pluginInterface("IXmppStreamManager").value(0,NULL);
	if (plugin)
	{
		FXmppStreamManager = qobject_cast<IXmppStreamManager *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IOptionsManager").value(0,NULL);
	if (plugin)
	{
		FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());
		if (FOptionsManager)
		{
			connect(FOptionsManager->instance(),SIGNAL(profileOpened(const QString &)),SLOT(onProfileOpened(const QString &)));
			connect(FOptionsManager->instance(),SIGNAL(profileClosed(const QString &)),SLOT(onProfileClosed(const QString &)));
		}
	}

	plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
	if (plugin)
	{
		FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
		if (FRostersViewPlugin)
		{
			connect(FRostersViewPlugin->rostersView()->instance(),SIGNAL(indexContextMenu(const QList<IRosterIndex *> &, quint32, Menu *)), 
				SLOT(onRostersViewIndexContextMenu(const QList<IRosterIndex *> &, quint32, Menu *)));
		}
	}

	connect(Options::instance(),SIGNAL(optionsOpened()),SLOT(onOptionsOpened()));
	connect(Options::instance(),SIGNAL(optionsClosed()),SLOT(onOptionsClosed()));
	connect(Options::instance(),SIGNAL(optionsChanged(const OptionsNode &)),SLOT(onOptionsChanged(const OptionsNode &)));

	return FXmppStreamManager!=NULL;
}

bool AccountManager::initSettings()
{
	Options::setDefaultValue(OPV_ACCOUNT_DEFAULTRESOURCE, QString(CLIENT_NAME));

	Options::setDefaultValue(OPV_ACCOUNT_ACTIVE, true);
	Options::setDefaultValue(OPV_ACCOUNT_STREAMJID, QString());
	Options::setDefaultValue(OPV_ACCOUNT_RESOURCE, QString(CLIENT_NAME));
	Options::setDefaultValue(OPV_ACCOUNT_PASSWORD, QByteArray());
	Options::setDefaultValue(OPV_ACCOUNT_REQUIREENCRYPTION, true);

	if (FOptionsManager)
	{
		IOptionsDialogNode accountsNode = { ONO_ACCOUNTS, OPN_ACCOUNTS, MNI_ACCOUNT_LIST, tr("Accounts") };
		FOptionsManager->insertOptionsDialogNode(accountsNode);
		FOptionsManager->insertOptionsDialogHolder(this);
	}
	return true;
}

QMultiMap<int, IOptionsDialogWidget *> AccountManager::optionsDialogWidgets(const QString &ANodeId, QWidget *AParent)
{
	QMultiMap<int, IOptionsDialogWidget *> widgets;
	if (ANodeId.startsWith(OPN_ACCOUNTS))
	{
		QStringList nodeTree = ANodeId.split(".",QString::SkipEmptyParts);
		if (ANodeId == OPN_ACCOUNTS)
		{
			widgets.insertMulti(OHO_ACCOUNTS_ACCOUNTS, FOptionsManager->newOptionsDialogHeader(tr("Accounts"),AParent));
			widgets.insertMulti(OWO_ACCOUNTS_ACCOUNTS, new AccountsOptionsWidget(this,AParent));

			widgets.insertMulti(OHO_ACCOUNTS_COMMON, FOptionsManager->newOptionsDialogHeader(tr("Common account settings"),AParent));
			
			QComboBox *resourceCombox = newResourceComboBox(QUuid(),AParent);
			widgets.insertMulti(OWO_ACCOUNTS_DEFAULTRESOURCE,FOptionsManager->newOptionsDialogWidget(Options::node(OPV_ACCOUNT_DEFAULTRESOURCE),tr("Default resource:"),resourceCombox,AParent));
		}
		else if (nodeTree.count()==3 && nodeTree.at(0)==OPN_ACCOUNTS && nodeTree.at(2)=="Parameters")
		{
			OptionsNode options = Options::node(OPV_ACCOUNT_ITEM,nodeTree.at(1));

			widgets.insertMulti(OHO_ACCOUNTS_PARAMS_ACCOUNT,FOptionsManager->newOptionsDialogHeader(tr("Account"),AParent));
			widgets.insertMulti(OWO_ACCOUNTS_PARAMS_NAME,FOptionsManager->newOptionsDialogWidget(options.node("name"),tr("Name:"),AParent));
			widgets.insertMulti(OWO_ACCOUNTS_PARAMS_PASSWORD,FOptionsManager->newOptionsDialogWidget(options.node("password"),tr("Password:"),AParent));

			QComboBox *resourceCombox = newResourceComboBox(nodeTree.at(1),AParent);
			widgets.insertMulti(OWO_ACCOUNTS_PARAMS_RESOURCE,FOptionsManager->newOptionsDialogWidget(options.node("resource"),tr("Resource:"),resourceCombox,AParent));
		}
		else if (nodeTree.count()==3 && nodeTree.at(0)==OPN_ACCOUNTS && nodeTree.at(2)=="Additional")
		{
			OptionsNode options = Options::node(OPV_ACCOUNT_ITEM,nodeTree.at(1));

			widgets.insertMulti(OHO_ACCOUNTS_ADDITIONAL_SETTINGS, FOptionsManager->newOptionsDialogHeader(tr("Additional settings"),AParent));
			widgets.insertMulti(OWO_ACCOUNTS_ADDITIONAL_REQUIRESECURE,FOptionsManager->newOptionsDialogWidget(options.node("require-encryption"),tr("Require secure connection to server"),AParent));
		}

	}
	return widgets;
}

QList<IAccount *> AccountManager::accounts() const
{
	return FAccounts.values();
}

IAccount *AccountManager::findAccountById(const QUuid &AAcoountId) const
{
	return FAccounts.value(AAcoountId);
}

IAccount *AccountManager::findAccountByStream(const Jid &AStreamJid) const
{
	foreach(IAccount *account, FAccounts)
	{
		if (account->accountJid().pBare() == AStreamJid.pBare())
			return account;
		// XMPP stream node may be changed on login
		if (account->streamJid().pBare() == AStreamJid.pBare())
			return account;
	}
	return NULL;
}

IAccount *AccountManager::createAccount(const Jid &AAccountJid, const QString &AName)
{
	if (AAccountJid.isValid() && !AAccountJid.node().isEmpty() && findAccountByStream(AAccountJid)==NULL)
	{
		QUuid accountId = QUuid::createUuid();
		LOG_DEBUG(QString("Creating account, stream=%1, id=%2").arg(AAccountJid.pFull(), accountId.toString()));

		OptionsNode options = Options::node(OPV_ACCOUNT_ITEM,accountId.toString());
		options.setValue(AName,"name");
		options.setValue(AAccountJid.bare(),"streamJid");
		options.setValue(AAccountJid.resource(),"resource");

		return insertAccount(options);
	}
	else if (!AAccountJid.isValid() || AAccountJid.node().isEmpty())
	{
		REPORT_ERROR("Failed to create account: Invalid parameters");
	}
	else
	{
		LOG_ERROR(QString("Failed to create account, stream=%1: Account JID already exists").arg(AAccountJid.pFull()));
	}
	return NULL;
}

void AccountManager::destroyAccount(const QUuid &AAccountId)
{
	IAccount *account = FAccounts.value(AAccountId);
	if (account)
	{
		LOG_DEBUG(QString("Destroying account, stream=%1, id=%2").arg(account->accountJid().pFull(),AAccountId.toString()));

		account->setActive(false);
		removeAccount(AAccountId);
		Options::node(OPV_ACCOUNT_ROOT).removeChilds("account",AAccountId.toString());
		emit accountDestroyed(AAccountId);
	}
	else
	{
		REPORT_ERROR("Failed to destroy account: Account not found");
	}
}

IAccount *AccountManager::insertAccount(const OptionsNode &AOptions)
{
	Jid streamJid = AOptions.value("streamJid").toString();
	if (streamJid.isValid() && !streamJid.node().isEmpty() && findAccountByStream(streamJid)==NULL)
	{
		Account *account = new Account(FXmppStreamManager,AOptions,this);
		connect(account,SIGNAL(activeChanged(bool)),SLOT(onAccountActiveChanged(bool)));
		connect(account,SIGNAL(optionsChanged(const OptionsNode &)),SLOT(onAccountOptionsChanged(const OptionsNode &)));
		FAccounts.insert(account->accountId(),account);

		LOG_DEBUG(QString("Inserting account, stream=%1, id=%2").arg(account->accountJid().pFull(), account->accountId().toString()));
		openAccountOptionsNode(account->accountId());
		emit accountInserted(account);

		return account;
	}
	else if (!streamJid.isValid() || streamJid.node().isEmpty())
	{
		REPORT_ERROR("Failed to insert account: Invalid parameters");
	}
	return NULL;
}

void AccountManager::removeAccount(const QUuid &AAccountId)
{
	IAccount *account = FAccounts.value(AAccountId);
	if (account)
	{
		LOG_DEBUG(QString("Removing account, stream=%1, id=%2").arg(account->accountJid().pFull(), AAccountId.toString()));

		account->setActive(false);
		closeAccountOptionsNode(AAccountId);
		emit accountRemoved(account);

		FAccounts.remove(AAccountId);
		delete account->instance();
	}
	else if (AAccountId.isNull())
	{
		REPORT_ERROR("Failed to remove account: Invalid parameters");
	}
}

void AccountManager::openAccountOptionsNode(const QUuid &AAccountId)
{
	if (FOptionsManager)
	{
		QString paramsNodeId = QString(OPN_ACCOUNTS_PARAMS).replace("[id]", AAccountId.toString());
		IOptionsDialogNode paramsNode = { ONO_ACCOUNTS_PARAMS, paramsNodeId, MNI_ACCOUNT_CHANGE, tr("Parameters") };
		FOptionsManager->insertOptionsDialogNode(paramsNode);

		QString additionalNodeId = QString(OPN_ACCOUNTS_ADDITIONAL).replace("[id]", AAccountId.toString());
		IOptionsDialogNode additionalNode = { ONO_ACCOUNTS_ADDITIONAL, additionalNodeId, MNI_OPTIONS_DIALOG, tr("Additional") };
		FOptionsManager->insertOptionsDialogNode(additionalNode);
	}
}

void AccountManager::closeAccountOptionsNode(const QUuid &AAccountId)
{
	if (FOptionsManager)
	{
		QString paramsNodeId = QString(OPN_ACCOUNTS_PARAMS).replace("[id]", AAccountId.toString());
		FOptionsManager->removeOptionsDialogNode(paramsNodeId);

		QString additionalNodeId = QString(OPN_ACCOUNTS_ADDITIONAL).replace("[id]", AAccountId.toString());
		FOptionsManager->removeOptionsDialogNode(additionalNodeId);
	}
}

void AccountManager::showAccountOptionsDialog(const QUuid &AAccountId, QWidget *AParent)
{
	if (FOptionsManager)
	{
		QString rootId = OPN_ACCOUNTS"."+AAccountId.toString();
		FOptionsManager->showOptionsDialog(QString::null, rootId, AParent);
	}
}

QComboBox *AccountManager::newResourceComboBox(const QUuid &AAccountId, QWidget *AParent) const
{
	QComboBox *combox = new QComboBox(AParent);
	combox->addItem(CLIENT_NAME, QString(CLIENT_NAME));
	combox->addItem(tr("Home"), tr("Home"));
	combox->addItem(tr("Work"), tr("Work"));
	combox->addItem(tr("Notebook"), tr("Notebook"));
	
	combox->setEditable(true);
	connect(combox->lineEdit(),SIGNAL(editingFinished()),SLOT(onResourceComboBoxEditFinished()));

	QString defValue = Options::node(OPV_ACCOUNT_DEFAULTRESOURCE).value().toString();
	int defIndex = combox->findData(defValue);
	if (defIndex < 0)
	{
		combox->addItem(defValue,defValue);
		defIndex = combox->count()-1;
	}

	if (!AAccountId.isNull())
		combox->setItemText(defIndex, combox->itemText(defIndex) + " " + tr("(default)"));

	foreach (IAccount *account, FAccounts)
	{
		QString accValue = account->optionsNode().value("resource").toString();
		if (combox->findData(accValue) < 0)
		{
			QString caption = !accValue.isEmpty() ? accValue : tr("<Empty>");
			combox->addItem(caption,accValue);
		}
	}

	return combox;
}

void AccountManager::onOptionsOpened()
{
	onOptionsChanged(Options::node(OPV_ACCOUNT_DEFAULTRESOURCE));

	OptionsNode accountRoot = Options::node(OPV_ACCOUNT_ROOT);
	foreach(const QString &id, accountRoot.childNSpaces("account"))
	{
		if (!id.isEmpty())
		{
			if(QUuid(id).isNull() || insertAccount(accountRoot.node("account",id))==NULL)
				accountRoot.removeChilds("account",id);
		}
	}
}

void AccountManager::onOptionsClosed()
{
	foreach(const QUuid &id, FAccounts.keys())
		removeAccount(id);
}

void AccountManager::onOptionsChanged(const OptionsNode &ANode)
{
	if (ANode.path() == OPV_ACCOUNT_DEFAULTRESOURCE)
		Options::setDefaultValue(OPV_ACCOUNT_RESOURCE, ANode.value());
}

void AccountManager::onProfileOpened(const QString &AProfile)
{
	Q_UNUSED(AProfile);
	foreach(IAccount *account, FAccounts)
		account->setActive(account->optionsNode().value("active").toBool());
}

void AccountManager::onProfileClosed(const QString &AProfile)
{
	Q_UNUSED(AProfile);
	foreach(IAccount *account, FAccounts)
		account->setActive(false);
}

void AccountManager::onShowAccountOptions(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
		showAccountOptionsDialog(action->data(ADR_ACCOUNT_ID).toString());
}

void AccountManager::onAccountActiveChanged(bool AActive)
{
	IAccount *account = qobject_cast<IAccount *>(sender());
	if (account)
		emit accountActiveChanged(account,AActive);
}

void AccountManager::onAccountOptionsChanged(const OptionsNode &ANode)
{
	Account *account = qobject_cast<Account *>(sender());
	if (account)
		emit accountOptionsChanged(account, ANode);
}

void AccountManager::onRostersViewIndexContextMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu)
{
	if (ALabelId==AdvancedDelegateItem::DisplayId && AIndexes.count()==1 && AIndexes.first()->kind()==RIK_STREAM_ROOT)
	{
		IAccount *account = findAccountByStream(Jid(AIndexes.first()->data(RDR_STREAM_JID).toString()));
		if (account)
		{
			Action *action = new Action(AMenu);
			action->setIcon(RSR_STORAGE_MENUICONS,MNI_ACCOUNT_CHANGE);
			action->setText(tr("Modify account"));
			action->setData(ADR_ACCOUNT_ID,account->accountId().toString());
			connect(action,SIGNAL(triggered(bool)),SLOT(onShowAccountOptions(bool)));
			AMenu->addAction(action,AG_RVCM_ACCOUNTMANAGER,true);
		}
	}
}

void AccountManager::onResourceComboBoxEditFinished()
{
	QLineEdit *editor = qobject_cast<QLineEdit *>(sender());
	QComboBox *combox = editor!=NULL ? qobject_cast<QComboBox *>(editor->parentWidget()) : NULL;
	if (combox!=NULL && combox->itemText(combox->currentIndex()) != editor->text())
	{
		int valIndex = combox->findData(editor->text());
		if (valIndex < 0)
		{
			QString caption = !editor->text().isEmpty() ? editor->text() : tr("<Empty>");
			combox->addItem(caption, editor->text());
			combox->setCurrentIndex(combox->count()-1);
		}
		else if (valIndex != combox->currentIndex())
		{
			combox->setCurrentIndex(valIndex);
		}
	}
}

Q_EXPORT_PLUGIN2(plg_accountmanager, AccountManager)
