#include "accountmanager.h"

#include <QSet>
#include <QIcon>
#include <QTime>
#include <QMessageBox>
#include <QTextDocument>

#define SVN_ACCOUNT                 "account[]"
#define SVN_ACCOUNT_ACTIVE          SVN_ACCOUNT":"AVN_ACTIVE
#define SVN_ACCOUNT_STREAM          SVN_ACCOUNT":"AVN_STREAM_JID

#define ADR_OPTIONS_NODE            Action::DR_Parametr1

AccountManager::AccountManager()
{
  FSettingsPlugin = NULL;
  FSettings = NULL;
  FMainWindowPlugin = NULL;
  FRostersViewPlugin = NULL;
  FAccountSetup = NULL;
  srand(QTime::currentTime().msec());
}

AccountManager::~AccountManager()
{

}

//IPlugin
void AccountManager::pluginInfo(IPluginInfo *APluginInfo)
{
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->description = tr("Creating and removing accounts");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = tr("Account manager");
  APluginInfo->uid = ACCOUNTMANAGER_UUID;
  APluginInfo->version = "0.1";
  APluginInfo->dependences.append(XMPPSTREAMS_UUID); 
  APluginInfo->dependences.append(SETTINGS_UUID);  
}

bool AccountManager::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *plugin = APluginManager->getPlugins("IXmppStreams").value(0,NULL);
  if (plugin)
    FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());

  plugin = APluginManager->getPlugins("ISettingsPlugin").value(0,NULL);
  if (plugin)
  {
    FSettingsPlugin = qobject_cast<ISettingsPlugin *>(plugin->instance());
    if (FSettingsPlugin)
    {
      connect(FSettingsPlugin->instance(),SIGNAL(profileOpened(const QString &)),SLOT(onProfileOpened(const QString &)));
      connect(FSettingsPlugin->instance(),SIGNAL(profileClosed(const QString &)),SLOT(onProfileClosed(const QString &)));
      connect(FSettingsPlugin->instance(),SIGNAL(settingsOpened()),SLOT(onSettingsOpened()));
      connect(FSettingsPlugin->instance(),SIGNAL(settingsClosed()),SLOT(onSettingsClosed()));
      connect(FSettingsPlugin->instance(),SIGNAL(optionsDialogAccepted()),SLOT(onOptionsDialogAccepted()));
      connect(FSettingsPlugin->instance(),SIGNAL(optionsDialogRejected()),SLOT(onOptionsDialogRejected()));
      connect(FSettingsPlugin->instance(),SIGNAL(optionsDialogClosed()),SLOT(onOptionsDialogClosed()));
    }
  }

  plugin = APluginManager->getPlugins("IMainWindowPlugin").value(0,NULL);
  if (plugin)
  {
    FMainWindowPlugin = qobject_cast<IMainWindowPlugin *>(plugin->instance());
  }

  plugin = APluginManager->getPlugins("IRostersViewPlugin").value(0,NULL);
  if (plugin)
  {
    FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
    if (FRostersViewPlugin)
    {
      connect(FRostersViewPlugin->rostersView()->instance(),SIGNAL(contextMenu(IRosterIndex *, Menu *)),
        SLOT(onRostersViewContextMenu(IRosterIndex *, Menu *)));
    }
  }

  return FXmppStreams!=NULL && FSettingsPlugin!=NULL;
}

bool AccountManager::initObjects()
{
  FSettings = FSettingsPlugin->settingsForPlugin(ACCOUNTMANAGER_UUID);
  FSettingsPlugin->openOptionsNode(ON_ACCOUNTS,tr("Accounts"),tr("Creating and removing accounts"),MNI_ACCOUNT_LIST,ONO_ACCOUNTS);
  FSettingsPlugin->insertOptionsHolder(this);
  return true;
}

QWidget *AccountManager::optionsWidget(const QString &ANode, int &AOrder)
{
  AOrder = OWO_ACCOUNT_OPTIONS;
  QStringList nodeTree = ANode.split("::",QString::SkipEmptyParts);
  if (ANode == ON_ACCOUNTS)
  {
    if (FAccountSetup == NULL)
    {
      FAccountSetup = new AccountManage(NULL);
      connect(FAccountSetup,SIGNAL(accountAdded(const QString &)),
        SLOT(onOptionsAccountAdded(const QString &)));
      connect(FAccountSetup,SIGNAL(accountShow(const QString &)),
        SLOT(onOptionsAccountShow(const QString &)));
      connect(FAccountSetup,SIGNAL(accountRemoved(const QString &)),
        SLOT(onOptionsAccountRemoved(const QString &)));
      foreach(IAccount *account,FAccounts)
        FAccountSetup->setAccount(account->accountId(),account->name(),account->streamJid().full(),account->isActive());
    }
    return FAccountSetup;
  }
  else if (nodeTree.count()==2 && nodeTree.at(0)==ON_ACCOUNTS)
  {
    QString accountId = nodeTree.at(1);
    AccountOptions *options = new AccountOptions(accountId);
    IAccount *account = accountById(accountId);
    if (account)
    {
      options->setOption(AccountOptions::AO_Name,account->name());
      options->setOption(AccountOptions::AO_StreamJid,account->value(AVN_STREAM_JID));
      options->setOption(AccountOptions::AO_Password,account->password());
      options->setOption(AccountOptions::AO_DefLang,account->defaultLang());
    }
    else
    {
      options->setOption(AccountOptions::AO_Name,FAccountSetup->accountName(accountId));
    }
    FAccountOptions.insert(accountId,options);
    return options;
  }
  return NULL;
}

IAccount *AccountManager::insertAccount(const QString &AAccountId)
{
  if (!FAccounts.contains(AAccountId))
  {
    Account *account = new Account(FXmppStreams,FSettings,AAccountId.isEmpty() ? newId() : AAccountId,this);
    connect(account,SIGNAL(changed(const QString &, const QVariant &)),SLOT(onAccountChanged(const QString &, const QVariant &)));
    FAccounts.insert(AAccountId,account);
    openAccountOptionsNode(AAccountId);
    emit inserted(account);
    return account;
  }
  return FAccounts.value(AAccountId);
}

QList<IAccount *> AccountManager::accounts() const
{
  QList<IAccount *> accounts;
  foreach(Account *account, FAccounts)
    accounts.append(account);
  return accounts;
}

IAccount *AccountManager::accountById(const QString &AAcoountId) const
{
  return FAccounts.value(AAcoountId);
}

IAccount *AccountManager::accountByStream(const Jid &AStreamJid) const
{
  foreach(Account *account, FAccounts)
    if (account->streamJid() == AStreamJid)
      return account;
  return NULL;
}

void AccountManager::showAccount(const QString &AAccountId)
{
  Account *account = FAccounts.value(AAccountId);
  if (account)
    account->setActive(true);
}

void AccountManager::hideAccount(const QString &AAccountId)
{
  Account *account = FAccounts.value(AAccountId);
  if (account)
    account->setActive(false);
}

void AccountManager::removeAccount(const QString &AAccountId)
{
  Account *account = FAccounts.value(AAccountId);
  if (account)
  {
    hideAccount(AAccountId);
    emit removed(account);
    closeAccountOptionsNode(AAccountId);
    FAccounts.remove(AAccountId);
    delete account;
  }
}

void AccountManager::destroyAccount(const QString &AAccountId)
{
  Account *account = FAccounts.value(AAccountId);
  if (account)
  {
    hideAccount(AAccountId);
    removeAccount(AAccountId);
    FSettings->deleteValueNS(SVN_ACCOUNT,AAccountId);
    emit destroyed(AAccountId);
  }
}


QString AccountManager::newId() const
{
  QString id;
  while (id.isEmpty() || FAccounts.contains(id))
    id = QString::number((rand()<<16)+rand(),36);
  return id;
}

void AccountManager::openAccountOptionsNode(const QString &AAccountId, const QString &AName)
{
  QString node = ON_ACCOUNTS"::"+AAccountId;
  QString name = AName;
  if (name.isEmpty())
    name = FAccounts.contains(AAccountId) ? FAccounts.value(AAccountId)->name() : tr("<Empty>");
  FSettingsPlugin->openOptionsNode(node,name,tr("Account details and connection options"),MNI_ACCOUNT,ONO_ACCOUNTS);
}

void AccountManager::closeAccountOptionsNode(const QString &AAccountId)
{
  QString node = ON_ACCOUNTS"::"+AAccountId;
  FSettingsPlugin->closeOptionsNode(node);
  if (FAccountOptions.contains(AAccountId))
  {
    AccountOptions *options = FAccountOptions.take(AAccountId);
    delete options;
  }
}

void AccountManager::onAccountChanged(const QString &AName, const QVariant &AValue)
{
  Account *account = qobject_cast<Account *>(sender());
  if (account)
  {
    if (AName == AVN_ACTIVE)
      AValue.toBool() ? emit shown(account) : emit hidden(account);
    if (AName == AVN_NAME)
      openAccountOptionsNode(account->accountId(),account->name());
  }
}

void AccountManager::onOptionsAccountAdded(const QString &AName)
{
  QString id = newId();
  FAccountSetup->setAccount(id,AName,"",true);
  openAccountOptionsNode(id,AName);
  FSettingsPlugin->openOptionsDialog(ON_ACCOUNTS"::"+id);
}

void AccountManager::onOptionsAccountShow(const QString &AAccountId)
{
  if (!AAccountId.isEmpty())
    FSettingsPlugin->openOptionsDialog(ON_ACCOUNTS"::"+AAccountId);
}

void AccountManager::onOptionsAccountRemoved(const QString &AAccountId)
{
  FAccountSetup->removeAccount(AAccountId);
  closeAccountOptionsNode(AAccountId);
}

void AccountManager::onOpenOptionsDialogByAction(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (FSettingsPlugin && action)
  {
    QString optionsNode = action->data(ADR_OPTIONS_NODE).toString();
    FSettingsPlugin->openOptionsDialog(optionsNode);
  }
}

void AccountManager::onOptionsDialogAccepted()
{
  QSet<QString> curAccounts = FAccounts.keys().toSet();
  QSet<QString> allAccounts = FAccountOptions.keys().toSet();
  QSet<QString> oldAccounts = curAccounts - allAccounts;

  foreach(QString id,oldAccounts)
    destroyAccount(id);

  foreach(QString id, allAccounts)
  {
    QPointer<AccountOptions> options = FAccountOptions.value(id);
    Jid streamJid = options->option(AccountOptions::AO_StreamJid).toString();
    QString name = options->option(AccountOptions::AO_Name).toString();
    if (name.isEmpty())
      name= streamJid.full();

    IAccount *account = insertAccount(id);
    account->setName(name);
    account->setStreamJid(streamJid);
    account->setPassword(options->option(AccountOptions::AO_Password).toString());
    account->setDefaultLang(options->option(AccountOptions::AO_DefLang).toString());
    account->setActive(FAccountSetup->isAccountActive(id));
    if (!account->isValid() && FAccountSetup->isAccountActive(id))
    {
      QMessageBox::warning(NULL,tr("Not valid account"),tr("Account %1 is not valid, change its Jabber ID").arg(Qt::escape(name)));
    }
    FSettings->setValueNS(SVN_ACCOUNT_ACTIVE,id,account->isActive());
    FAccountSetup->setAccount(id,name,streamJid.full(),account->isActive());
  }
  emit optionsAccepted();
}

void AccountManager::onOptionsDialogRejected()
{
  QSet<QString> curAccounts = FAccounts.keys().toSet();
  QSet<QString> allAccounts = FAccountOptions.keys().toSet();
  QSet<QString> newAccounts = allAccounts - curAccounts;
  QSet<QString> oldAccounts = curAccounts - allAccounts;

  foreach(QString id,newAccounts)
    closeAccountOptionsNode(id);

  foreach(QString id,oldAccounts)
    openAccountOptionsNode(id);

  emit optionsRejected();
}

void AccountManager::onOptionsDialogClosed()
{
  FAccountSetup = NULL;
  FAccountOptions.clear();
}

void AccountManager::onProfileOpened(const QString &/*AProfile*/)
{
  QList<QString> acoounts = FAccounts.keys();
  foreach(QString id, acoounts)
    if (FSettings->valueNS(SVN_ACCOUNT_ACTIVE,id).toBool())
      showAccount(id);
}

void AccountManager::onProfileClosed(const QString &/*AProfile*/)
{
  QList<QString> acoounts = FAccounts.keys();
  foreach(QString id, acoounts)
    hideAccount(id);
}

void AccountManager::onSettingsOpened()
{
  QList<QString> acoounts = FSettings->values(SVN_ACCOUNT).keys();
  foreach(QString id,acoounts)
    insertAccount(id);
}

void AccountManager::onSettingsClosed()
{
  QList<QString> acoounts = FAccounts.keys();
  foreach(QString id,acoounts)
    removeAccount(id);
}

void AccountManager::onRostersViewContextMenu(IRosterIndex *AIndex, Menu *AMenu)
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
      action->setData(ADR_OPTIONS_NODE,ON_ACCOUNTS"::"+account->accountId());
      connect(action,SIGNAL(triggered(bool)),SLOT(onOpenOptionsDialogByAction(bool)));
      AMenu->addAction(action,AG_RVCM_ACCOUNTMANAGER,true);
    }
  }
}

Q_EXPORT_PLUGIN2(AccountManagerPlugin, AccountManager)
