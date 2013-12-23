#include "accountmanager.h"

#include <definitions/actiongroups.h>
#include <definitions/optionnodes.h>
#include <definitions/optionvalues.h>
#include <definitions/optionnodeorders.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/rosterindexkinds.h>
#include <definitions/rosterindexroles.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <utils/action.h>
#include <utils/logger.h>

#define ADR_ACCOUNT_ID              Action::DR_Parametr1

AccountManager::AccountManager()
{
	FXmppStreams = NULL;
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

	IPlugin *plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
	if (plugin)
	{
		FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
		if (FXmppStreams == NULL)
			LOG_WARNING("Failed to load required interface: IXmppStreams");
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

	return FXmppStreams!=NULL;
}

bool AccountManager::initSettings()
{
	Options::setDefaultValue(OPV_ACCOUNT_REQUIREENCRYPTION, true);
	if (FOptionsManager)
	{
		IOptionsDialogNode accountsNode = { ONO_ACCOUNTS, OPN_ACCOUNTS, tr("Accounts"), MNI_ACCOUNT_LIST };
		FOptionsManager->insertOptionsDialogNode(accountsNode);
		FOptionsManager->insertOptionsHolder(this);
	}
	return true;
}

QMultiMap<int, IOptionsWidget *> AccountManager::optionsWidgets(const QString &ANodeId, QWidget *AParent)
{
	QMultiMap<int, IOptionsWidget *> widgets;
	if (ANodeId.startsWith(OPN_ACCOUNTS))
	{
		QStringList nodeTree = ANodeId.split(".",QString::SkipEmptyParts);
		if (ANodeId == OPN_ACCOUNTS)
		{
			widgets.insertMulti(OWO_ACCOUNT_OPTIONS, new AccountsOptions(this,AParent));
		}
		else if (nodeTree.count()==2 && nodeTree.at(0)==OPN_ACCOUNTS)
		{
			OptionsNode aoptions = Options::node(OPV_ACCOUNT_ITEM,nodeTree.at(1));
			widgets.insertMulti(OWO_ACCOUNT_OPTIONS,new AccountOptions(this,nodeTree.at(1),AParent));
			widgets.insertMulti(OWO_ACCOUNT_REQUIRE_ENCRYPTION,FOptionsManager->optionsNodeWidget(aoptions.node("require-encryption"),tr("Require a secure connection"),AParent));
		}
	}
	return widgets;
}

QList<IAccount *> AccountManager::accounts() const
{
	return FAccounts.values();
}

IAccount *AccountManager::accountById(const QUuid &AAcoountId) const
{
	return FAccounts.value(AAcoountId);
}

IAccount *AccountManager::accountByStream(const Jid &AStreamJid) const
{
	foreach(IAccount *account, FAccounts)
	{
		if (account->xmppStream() && account->xmppStream()->streamJid()==AStreamJid)
			return account;
		else if (account->streamJid() == AStreamJid)
			return account;
	}
	return NULL;
}

IAccount *AccountManager::appendAccount(const QUuid &AAccountId)
{
	if (!AAccountId.isNull() && !FAccounts.contains(AAccountId))
	{
		Account *account = new Account(FXmppStreams,Options::node(OPV_ACCOUNT_ITEM,AAccountId.toString()),this);
		connect(account,SIGNAL(activeChanged(bool)),SLOT(onAccountActiveChanged(bool)));
		connect(account,SIGNAL(optionsChanged(const OptionsNode &)),SLOT(onAccountOptionsChanged(const OptionsNode &)));
		FAccounts.insert(AAccountId,account);

		LOG_STRM_DEBUG(account->streamJid(),QString("Appending account=%1").arg(account->name()));
		openAccountOptionsNode(AAccountId,account->name());
		emit appended(account);
	}
	else
	{
		REPORT_ERROR("Failed to append account: Invalid params");
	}
	return FAccounts.value(AAccountId);
}

void AccountManager::showAccount(const QUuid &AAccountId)
{
	IAccount *account = FAccounts.value(AAccountId);
	if (account)
		account->setActive(true);
	else
		REPORT_ERROR("Failed to show account: Account not found");
}

void AccountManager::hideAccount(const QUuid &AAccountId)
{
	IAccount *account = FAccounts.value(AAccountId);
	if (account)
		account->setActive(false);
	else
		REPORT_ERROR("Failed to hide account: Account not found");
}

void AccountManager::removeAccount(const QUuid &AAccountId)
{
	IAccount *account = FAccounts.value(AAccountId);
	if (account)
	{
		LOG_STRM_DEBUG(account->streamJid(),QString("Removing account=%1").arg(account->name()));
		hideAccount(AAccountId);
		closeAccountOptionsNode(AAccountId);
		emit removed(account);
		FAccounts.remove(AAccountId);
		delete account->instance();
	}
	else
	{
		REPORT_ERROR("Failed to remove account: Account not found");
	}
}

void AccountManager::destroyAccount(const QUuid &AAccountId)
{
	IAccount *account = FAccounts.value(AAccountId);
	if (account)
	{
		LOG_STRM_DEBUG(account->streamJid(),QString("Destroying account=%1").arg(account->name()));
		hideAccount(AAccountId);
		removeAccount(AAccountId);
		Options::node(OPV_ACCOUNT_ROOT).removeChilds("account",AAccountId.toString());
		emit destroyed(AAccountId);
	}
	else
	{
		REPORT_ERROR("Failed to destroy account: Account not found");
	}
}

void AccountManager::showAccountOptionsDialog(const QUuid &AAccountId)
{
	if (FOptionsManager)
		FOptionsManager->showOptionsDialog(OPN_ACCOUNTS "." + AAccountId.toString());
}

void AccountManager::openAccountOptionsNode(const QUuid &AAccountId, const QString &AName)
{
	if (FOptionsManager)
	{
		QString node = OPN_ACCOUNTS "." + AAccountId.toString();
		IOptionsDialogNode dnode = { ONO_ACCOUNTS, node, AName, MNI_ACCOUNT };
		FOptionsManager->insertOptionsDialogNode(dnode);
	}
}

void AccountManager::closeAccountOptionsNode(const QUuid &AAccountId)
{
	if (FOptionsManager)
	{
		QString node = OPN_ACCOUNTS "." + AAccountId.toString();
		FOptionsManager->removeOptionsDialogNode(node);
	}
}

void AccountManager::onProfileOpened(const QString &AProfile)
{
	Q_UNUSED(AProfile);
	foreach(IAccount *account, FAccounts)
		account->setActive(Options::node(OPV_ACCOUNT_ITEM,account->accountId()).value("active").toBool());
}

void AccountManager::onProfileClosed(const QString &AProfile)
{
	Q_UNUSED(AProfile);
	foreach(IAccount *account, FAccounts)
	{
		Options::node(OPV_ACCOUNT_ITEM,account->accountId()).setValue(account->isActive(),"active");
		account->setActive(false);
	}
}

void AccountManager::onOptionsOpened()
{
	foreach(const QString &id, Options::node(OPV_ACCOUNT_ROOT).childNSpaces("account"))
		appendAccount(id);
}

void AccountManager::onOptionsClosed()
{
	foreach(const QUuid &id, FAccounts.keys())
		removeAccount(id);
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
	{
		if (AActive)
			emit shown(account);
		else
			emit hidden(account);
	}
}

void AccountManager::onAccountOptionsChanged(const OptionsNode &ANode)
{
	Account *account = qobject_cast<Account *>(sender());
	if (account)
	{
		if (account->optionsNode().childPath(ANode) == "name")
			openAccountOptionsNode(account->accountId(),ANode.value().toString());
		emit changed(account, ANode);
	}
}

void AccountManager::onRostersViewIndexContextMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu)
{
	if (ALabelId==AdvancedDelegateItem::DisplayId && AIndexes.count()==1 && AIndexes.first()->kind()==RIK_STREAM_ROOT)
	{
		IAccount *account = accountByStream(AIndexes.first()->data(RDR_STREAM_JID).toString());
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

Q_EXPORT_PLUGIN2(plg_accountmanager, AccountManager)
