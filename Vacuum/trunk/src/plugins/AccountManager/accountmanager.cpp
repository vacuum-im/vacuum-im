#include "accountmanager.h"

#include <QSet>
#include <QIcon>
#include <QTime>
#include <QMessageBox>

AccountManager::AccountManager()
{
  FPluginManager = NULL;
  FSettingsPlugin = NULL;
  FSettings = NULL;
  FMainWindowPlugin = NULL;
  FRostersViewPlugin = NULL;
  actAccountsSetup = NULL;
  srand(QTime::currentTime().msec());
}

AccountManager::~AccountManager()
{

}

//IPlugin
void AccountManager::pluginInfo(PluginInfo *APluginInfo)
{
  APluginInfo->author = tr("Potapov S.A. aka Lion");
  APluginInfo->description = tr("Creating and removing accounts");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = "Account manager";
  APluginInfo->uid = ACCOUNTMANAGER_UUID;
  APluginInfo->version = "0.1";
  APluginInfo->dependences.append(XMPPSTREAMS_UUID); 
  APluginInfo->dependences.append(SETTINGS_UUID);  
}

bool AccountManager::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  FPluginManager = APluginManager;

  IPlugin *plugin = FPluginManager->getPlugins("IXmppStreams").value(0,NULL);
  if (plugin)
    FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());

  plugin = FPluginManager->getPlugins("ISettingsPlugin").value(0,NULL);
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
   }
  }

  plugin = APluginManager->getPlugins("IMainWindowPlugin").value(0,NULL);
  if (plugin)
  {
    FMainWindowPlugin = qobject_cast<IMainWindowPlugin *>(plugin->instance());
    if (FMainWindowPlugin)
    {
      connect(FMainWindowPlugin->instance(),SIGNAL(mainWindowCreated(IMainWindow *)),
        SLOT(onMainWindowCreated(IMainWindow *)));
    }
  }

  plugin = APluginManager->getPlugins("IRostersViewPlugin").value(0,NULL);
  if (plugin)
  {
    FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
    if (FRostersViewPlugin)
    {
      connect(FRostersViewPlugin->instance(),SIGNAL(viewCreated(IRostersView *)),
        SLOT(onRostersViewCreated(IRostersView *)));
    }
  }

  return FXmppStreams!=NULL && FSettingsPlugin!=NULL;
}

bool AccountManager::initObjects()
{
  if (FSettingsPlugin)
  {
    FSettings = FSettingsPlugin->settingsForPlugin(ACCOUNTMANAGER_UUID);
    FSettingsPlugin->openOptionsNode(ON_ACCOUNTS,tr("Accounts"), tr("Creating and removing accounts"),QIcon());
    FSettingsPlugin->appendOptionsHolder(this);
  }
  return FSettings != NULL;
}


//IAccountManager
IAccount *AccountManager::addAccount(const QString &AName, const Jid &AStreamJid)
{
  IAccount *account = accountByStream(AStreamJid);
  if (!account)
  {
    IXmppStream *stream = FXmppStreams->newStream(AStreamJid);
    account = new Account(newId(),FSettings,stream,this);
    account->setName(AName);
    FAccounts.append((Account *)account);
    emit added(account);
    openAccountOptionsNode(account->accountId());
  }
  return account;
}

IAccount *AccountManager::addAccount(const QString &AAccountId, const QString &AName, 
                                     const Jid &AStreamJid)
{
  IAccount *account = accountById(AAccountId);
  if (!account && !AAccountId.isEmpty())
  {
    QString name = AName;
    if (name.isEmpty())
      name = FSettings->valueNS("account[]:name",AAccountId,AAccountId).toString();

    Jid streamJid = AStreamJid;
    if (!streamJid.isValid())
      streamJid = FSettings->valueNS("account[]:streamJid",AAccountId).toString();

    IXmppStream *stream = FXmppStreams->newStream(streamJid);
    account = new Account(AAccountId,FSettings,stream,this);
    account->setName(name);
    FAccounts.append((Account *)account);
    emit added(account);
    openAccountOptionsNode(AAccountId);
  }
  return account;
}

void AccountManager::showAccount(IAccount *AAccount)
{
  if (!FXmppStreams->isActive(AAccount->xmppStream()))
  {
    FXmppStreams->addStream(AAccount->xmppStream());
    emit shown(AAccount);
  }
}

void AccountManager::hideAccount(IAccount *AAccount)
{
  if (FXmppStreams->isActive(AAccount->xmppStream()))
  {
    AAccount->xmppStream()->close();
    FXmppStreams->removeStream(AAccount->xmppStream());
    emit hidden(AAccount);
  }
}

IAccount *AccountManager::accountById(const QString &AAcoountId) const
{
  Account *account;
  foreach(account, FAccounts)
    if (account->accountId() == AAcoountId)
      return account;
  return NULL;
}

IAccount *AccountManager::accountByName(const QString &AName) const
{
  Account *account;
  foreach(account, FAccounts)
    if (account->name() == AName)
      return account;
  return NULL;
}

IAccount *AccountManager::accountByStream(const Jid &AStreamJid) const
{
  Account *account;
  foreach(account, FAccounts)
    if (account->xmppStream()->jid() == AStreamJid)
      return account;
  return NULL;
}

void AccountManager::removeAccount(IAccount *AAccount)
{
  Account *account = qobject_cast<Account *>(AAccount->instance());
  if (account)
  {
    closeAccountOptionsNode(account->accountId());
    hideAccount(account);
    FAccounts.removeAt(FAccounts.indexOf(account));
    emit removed(account);
    FXmppStreams->destroyStream(account->streamJid());
    delete account;
  }
}

void AccountManager::destroyAccount(const QString &AAccountId)
{
  IAccount *account = accountById(AAccountId);
  if (account)
  {
    closeAccountOptionsNode(account->accountId());
    hideAccount(account);
    FAccounts.removeAt(FAccounts.indexOf((Account *)account));
    emit removed(account);
    emit destroyed(account);
    FXmppStreams->destroyStream(account->streamJid());
    account->clear();
    delete qobject_cast<Account *>(account->instance());
  }
}


//IOptionsHolder
QWidget *AccountManager::optionsWidget(const QString &ANode, int &AOrder)
{
  AOrder = OO_ACCOUNT_OPTIONS;
  QStringList nodeTree = ANode.split("::",QString::SkipEmptyParts);
  if (ANode == ON_ACCOUNTS)
  {
    if (FAccountManage.isNull())
    {
      FAccountManage = new AccountManage(NULL);
      connect(FAccountManage,SIGNAL(accountAdded(const QString &)),
        SLOT(onOptionsAccountAdded(const QString &)));
      connect(FAccountManage,SIGNAL(accountRemoved(const QString &)),
        SLOT(onOptionsAccountRemoved(const QString &)));
      foreach(IAccount *account,FAccounts)
      {
        FAccountManage->setAccount(account->accountId(),account->name(),
          account->streamJid().full(),account->isActive());
      }
    }
    return FAccountManage;
  }
  else if (nodeTree.count()==2 && nodeTree.at(0)==ON_ACCOUNTS)
  {
    QString accountId = nodeTree.at(1);
    AccountOptions *options = new AccountOptions(accountId);
    IAccount *account = accountById(accountId);
    if (account)
    {
      options->setOption(AccountOptions::AO_Name,account->name());
      options->setOption(AccountOptions::AO_StreamJid,account->streamJid().full());
      options->setOption(AccountOptions::AO_Password,account->password());
      options->setOption(AccountOptions::AO_DefLang,account->defaultLang());
    }
    else
    {
      options->setOption(AccountOptions::AO_Name,FAccountManage->accountName(accountId));
   }
    FAccountOptions.insert(accountId,options);
    return options;
  }
  return NULL;
}

QString AccountManager::newId() const
{
  QString id;
  while (id.isEmpty() || accountById(id))
    id = QString::number((rand()<<16)+rand(),36);
  return id;
}

void AccountManager::openAccountOptionsNode(const QString &AAccountId, const QString &AName)
{
  QString node = ON_ACCOUNTS+QString("::")+AAccountId;
  QString name = AName;
  if (AName.isEmpty())
  {
    IAccount *account = accountById(AAccountId);
    if (account)
      name = account->name();
  }
  if (!FAccountOptions.contains(AAccountId))
    FAccountOptions.insert(AAccountId,NULL);
  FSettingsPlugin->openOptionsNode(node,name,tr("Account details and connection options"),QIcon());
}

void AccountManager::closeAccountOptionsNode(const QString &AAccountId)
{
  QString node = ON_ACCOUNTS+QString("::")+AAccountId;
  FSettingsPlugin->closeOptionsNode(node);
  if (FAccountOptions.contains(AAccountId))
  {
    QPointer <AccountOptions> options = FAccountOptions.take(AAccountId);
    if (!options.isNull())
      delete options;
  }
}

void AccountManager::showAllActiveAccounts()
{
  IAccount *account;
  foreach (account, FAccounts)
    if (account->isActive())
      showAccount(account);
}

void AccountManager::onRostersViewCreated(IRostersView *ARostersView)
{
  if (FSettingsPlugin)
  {
    connect(ARostersView,SIGNAL(contextMenu(IRosterIndex *, Menu *)),
      SLOT(onRostersViewContextMenu(IRosterIndex *, Menu *)));
  }
}

void AccountManager::onMainWindowCreated(IMainWindow *AMainWindow)
{
  if (FSettingsPlugin)
  {
    actAccountsSetup = new Action(this);
    actAccountsSetup->setIcon(SYSTEM_ICONSETFILE,"psi/account");
    actAccountsSetup->setText(tr("Account setup..."));
    actAccountsSetup->setData(Action::DR_Parametr1,ON_ACCOUNTS);
    connect(actAccountsSetup,SIGNAL(triggered(bool)),FSettingsPlugin->instance(),SLOT(openOptionsDialogByAction(bool)));
    AMainWindow->mainMenu()->addAction(actAccountsSetup,AG_ACCOUNTMANAGER_MMENU,true);
  }
}

void AccountManager::onOptionsAccountAdded(const QString &AName)
{
  QString id = newId();
  FAccountManage->setAccount(id,AName,QString(),Qt::Unchecked);
  openAccountOptionsNode(id,AName);
  FSettingsPlugin->openOptionsDialog(ON_ACCOUNTS+QString("::")+id);
}

void AccountManager::onOptionsAccountRemoved(const QString &AAccountId)
{
  FAccountManage->removeAccount(AAccountId);
  closeAccountOptionsNode(AAccountId);
}

void AccountManager::onOptionsDialogAccepted()
{
  IAccount *account;
  QSet<QString> curAccounts;
  foreach(account,FAccounts)
    curAccounts += account->accountId(); 

  QSet<QString> allAccounts = FAccountOptions.keys().toSet();
  QSet<QString> newAccounts = allAccounts - curAccounts;
  QSet<QString> oldAccounts = curAccounts - allAccounts;

  foreach(QString id,oldAccounts)
    destroyAccount(id);

  foreach(QString id,allAccounts)
  {
    QPointer<AccountOptions> options = FAccountOptions.value(id);
    Jid streamJid = options->option(AccountOptions::AO_StreamJid).toString();
    QString name = options->option(AccountOptions::AO_Name).toString();
    if (name.isEmpty())
      name= streamJid.hFull();

    bool canApply = true;
    QString warningMessage = tr("'%1' account changes cannot by applied:<br><br>").arg(name);
    if (!streamJid.isValid() || streamJid.node().isEmpty())
    {
      canApply = false;
      warningMessage += tr(" - jabber ID is not valid<br>");
    }
    account = accountByStream(streamJid);
    if (account && account->accountId()!=id)
    {
      canApply = false;
      warningMessage += tr(" - jabber ID '%1' already exists<br>").arg(streamJid.hFull());
    }

    if (canApply)
    {
      if (!newAccounts.contains(id))
      {
        account = accountById(id);
        account->setName(name);
        account->setStreamJid(streamJid);
      }
      else
        account = addAccount(id,name,streamJid);
      account->setPassword(options->option(AccountOptions::AO_Password).toString());
      account->setDefaultLang(options->option(AccountOptions::AO_DefLang).toString());
      account->setActive(FAccountManage->accountActive(id));
      FAccountManage->setAccount(id,name,streamJid.full(),account->isActive());
      openAccountOptionsNode(id,name);
      if (account->isActive())
        showAccount(account);
      else
        hideAccount(account);
    } 
    else
    {
      if (newAccounts.contains(id))
      {
        FAccountManage->removeAccount(id);
        closeAccountOptionsNode(id);
      }
      QMessageBox::warning(NULL,tr("Account options canceled"),warningMessage);
    }
  }
  emit optionsAccepted();
}

void AccountManager::onOptionsDialogRejected()
{
  IAccount *account;
  QSet<QString> curAccounts;
  foreach(account,FAccounts)
    curAccounts += account->accountId(); 

  QSet<QString> allAccounts = FAccountOptions.keys().toSet();
  QSet<QString> newAccounts = allAccounts - curAccounts;
  QSet<QString> oldAccounts = curAccounts - allAccounts;

  foreach(QString id,newAccounts)
    closeAccountOptionsNode(id);

  foreach(QString id,oldAccounts)
    openAccountOptionsNode(id,"");

  emit optionsRejected();
}

void AccountManager::onProfileOpened(const QString &/*AProfile*/)
{
  showAllActiveAccounts();
}

void AccountManager::onProfileClosed(const QString &/*AProfile*/)
{
  while (FAccounts.count() > 0)
    removeAccount(FAccounts.at(0));
}

void AccountManager::onSettingsOpened()
{
  QList<QString> acoountsId = FSettings->values("account[]").keys();
  foreach(QString id,acoountsId)
    addAccount(id);
}

void AccountManager::onSettingsClosed()
{

}

void AccountManager::onRostersViewContextMenu(IRosterIndex *AIndex, Menu *AMenu)
{
  if (AIndex->data(RDR_Type).toInt() == RIT_StreamRoot)
  {
    QString streamJid = AIndex->data(RDR_StreamJid).toString();
    IAccount *account = accountByStream(streamJid);
    if (account)
    {
      Action *modify = new Action(AMenu);
      modify->setIcon(SYSTEM_ICONSETFILE,"psi/account");
      modify->setText(tr("Modify account..."));
      modify->setData(Action::DR_Parametr1,ON_ACCOUNTS+QString("::")+account->accountId());
      connect(modify,SIGNAL(triggered(bool)),FSettingsPlugin->instance(),SLOT(openOptionsDialogByAction(bool)));
      AMenu->addAction(modify,AG_ACCOUNTMANAGER_ROSTER,true);
    }
  }
}

Q_EXPORT_PLUGIN2(AccountManagerPlugin, AccountManager)
