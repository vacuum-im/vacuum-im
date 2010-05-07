#include "accountmanager.h"

#define ADR_ACCOUNT_ID              Action::DR_Parametr1

AccountManager::AccountManager()
{
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

bool AccountManager::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
	IPlugin *plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
	if (plugin)
		FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());

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
			connect(FRostersViewPlugin->rostersView()->instance(),SIGNAL(indexContextMenu(IRosterIndex *, Menu *)),
			        SLOT(onRosterIndexContextMenu(IRosterIndex *, Menu *)));
		}
	}

	connect(Options::instance(),SIGNAL(optionsOpened()),SLOT(onOptionsOpened()));
	connect(Options::instance(),SIGNAL(optionsClosed()),SLOT(onOptionsClosed()));

	return FXmppStreams!=NULL;
}

bool AccountManager::initSettings()
{
	if (FOptionsManager)
	{
		IOptionsDialogNode miscNode = { ONO_ACCOUNTS, OPN_ACCOUNTS, tr("Accounts"),tr("Creating and removing accounts"), MNI_ACCOUNT_LIST };
		FOptionsManager->insertOptionsDialogNode(miscNode);
		FOptionsManager->insertOptionsHolder(this);
	}

	return true;
}

IOptionsWidget *AccountManager::optionsWidget(const QString &ANode, int &AOrder, QWidget *AParent)
{
	if (ANode.startsWith(OPN_ACCOUNTS))
	{
		QStringList nodeTree = ANode.split(".",QString::SkipEmptyParts);
		if (ANode==OPN_ACCOUNTS || (nodeTree.count()==2 && nodeTree.at(0)==OPN_ACCOUNTS))
		{
			AOrder = OWO_ACCOUNT_OPTIONS;

			if (ANode==OPN_ACCOUNTS)
				return new AccountsOptions(this,AParent);
			else
				return new AccountOptions(this,nodeTree.at(1),AParent);
		}
	}
	return NULL;
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
		openAccountOptionsNode(AAccountId,account->name());
		emit appended(account);
		return account;
	}
	return FAccounts.value(AAccountId);
}

void AccountManager::showAccount(const QUuid &AAccountId)
{
	IAccount *account = FAccounts.value(AAccountId);
	if (account)
		account->setActive(true);
}

void AccountManager::hideAccount(const QUuid &AAccountId)
{
	IAccount *account = FAccounts.value(AAccountId);
	if (account)
		account->setActive(false);
}

void AccountManager::removeAccount(const QUuid &AAccountId)
{
	IAccount *account = FAccounts.value(AAccountId);
	if (account)
	{
		hideAccount(AAccountId);
		closeAccountOptionsNode(AAccountId);
		emit removed(account);
		FAccounts.remove(AAccountId);
		delete account->instance();
	}
}

void AccountManager::destroyAccount(const QUuid &AAccountId)
{
	IAccount *account = FAccounts.value(AAccountId);
	if (account)
	{
		hideAccount(AAccountId);
		removeAccount(AAccountId);
		Options::node(OPV_ACCOUNT_ROOT).removeChilds("account",AAccountId.toString());
		emit destroyed(AAccountId);
	}
}

void AccountManager::showAccountOptionsDialog(const QUuid &AAccountId)
{
	if (FOptionsManager)
	{
		FOptionsManager->showOptionsDialog(OPN_ACCOUNTS "." + AAccountId.toString());
	}
}

void AccountManager::openAccountOptionsNode(const QUuid &AAccountId, const QString &AName)
{
	if (FOptionsManager)
	{
		QString node = OPN_ACCOUNTS "." + AAccountId.toString();
		IOptionsDialogNode dnode = { ONO_ACCOUNTS, node, AName, tr("Account options"), MNI_ACCOUNT };
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
	foreach(QString id, Options::node(OPV_ACCOUNT_ROOT).childNSpaces("account"))
	appendAccount(id);
}

void AccountManager::onOptionsClosed()
{
	foreach(QUuid id, FAccounts.keys())
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

void AccountManager::onRosterIndexContextMenu(IRosterIndex *AIndex, Menu *AMenu)
{
	if (AIndex->data(RDR_TYPE).toInt() == RIT_STREAM_ROOT)
	{
		QString streamJid = AIndex->data(RDR_STREAM_JID).toString();
		IAccount *account = accountByStream(streamJid);
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
