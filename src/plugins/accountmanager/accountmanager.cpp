#include "accountmanager.h"

#define SVN_ACCOUNT                 "account[]"
#define SVN_ACCOUNT_ACTIVE          SVN_ACCOUNT":"AVN_ACTIVE
#define SVN_ACCOUNT_STREAM          SVN_ACCOUNT":"AVN_STREAM_JID

#define ADR_ACCOUNT_ID              Action::DR_Parametr1

AccountManager::AccountManager()
{
  FSettingsPlugin = NULL;
  FSettings = NULL;
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
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->dependences.append(XMPPSTREAMS_UUID); 
  APluginInfo->dependences.append(SETTINGS_UUID);  
}

bool AccountManager::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
  if (plugin)
    FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());

  plugin = APluginManager->pluginInterface("ISettingsPlugin").value(0,NULL);
  if (plugin)
  {
    FSettingsPlugin = qobject_cast<ISettingsPlugin *>(plugin->instance());
    if (FSettingsPlugin)
    {
      connect(FSettingsPlugin->instance(),SIGNAL(profileOpened(const QString &)),SLOT(onProfileOpened(const QString &)));
      connect(FSettingsPlugin->instance(),SIGNAL(profileClosed(const QString &)),SLOT(onProfileClosed(const QString &)));
      connect(FSettingsPlugin->instance(),SIGNAL(settingsOpened()),SLOT(onSettingsOpened()));
      connect(FSettingsPlugin->instance(),SIGNAL(settingsClosed()),SLOT(onSettingsClosed()));
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

  return FXmppStreams!=NULL && FSettingsPlugin!=NULL;
}

bool AccountManager::initObjects()
{
  if (FSettingsPlugin)
  {
    FSettings = FSettingsPlugin->settingsForPlugin(ACCOUNTMANAGER_UUID);
    FSettingsPlugin->openOptionsNode(ON_ACCOUNTS,tr("Accounts"),tr("Creating and removing accounts"),MNI_ACCOUNT_LIST,ONO_ACCOUNTS);
    FSettingsPlugin->insertOptionsHolder(this);
  }
  return true;
}

QWidget *AccountManager::optionsWidget(const QString &ANode, int &AOrder)
{
  if (ANode.startsWith(ON_ACCOUNTS))
  {
    AOrder = OWO_ACCOUNT_OPTIONS;
    QStringList nodeTree = ANode.split("::",QString::SkipEmptyParts);
    
    bool accountsWidget = ANode == ON_ACCOUNTS;
    bool accountWidget = !accountsWidget && nodeTree.count()==2 && nodeTree.at(0)==ON_ACCOUNTS;
    
    if (FAccountsOptions.isNull() && (accountsWidget || accountWidget))
    {
      FAccountsOptions = new AccountsOptions(this,NULL);
      connect(FSettingsPlugin->instance(),SIGNAL(optionsDialogAccepted()),FAccountsOptions,SLOT(apply()));
      connect(FSettingsPlugin->instance(),SIGNAL(optionsDialogRejected()),FAccountsOptions,SLOT(reject()));
      connect(FSettingsPlugin->instance(),SIGNAL(optionsDialogClosed()),FAccountsOptions,SLOT(deleteLater()));
      connect(FAccountsOptions,SIGNAL(optionsAccepted()),SIGNAL(optionsAccepted()));
      connect(FAccountsOptions,SIGNAL(optionsRejected()),SIGNAL(optionsRejected()));
    }

    if (accountsWidget)
      return FAccountsOptions;
    else if (accountWidget)
      return FAccountsOptions->accountOptions(nodeTree.at(1));
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
    Account *account = new Account(FXmppStreams,FSettings,AAccountId,this);
    connect(account,SIGNAL(changed(const QString &, const QVariant &)),SLOT(onAccountChanged(const QString &, const QVariant &)));
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
    if (FSettings)
      FSettings->deleteValueNS(SVN_ACCOUNT,AAccountId.toString());
    emit destroyed(AAccountId);
  }
}

void AccountManager::openAccountOptionsDialog(const QUuid &AAccountId)
{
  if (FSettingsPlugin)
  {
    QString optionsNode = ON_ACCOUNTS"::"+AAccountId.toString();
    FSettingsPlugin->openOptionsDialog(optionsNode);
  }
}

void AccountManager::openAccountOptionsNode(const QUuid &AAccountId, const QString &AName)
{
  if (FSettingsPlugin)
  {
    QString node = ON_ACCOUNTS"::"+AAccountId.toString();
    FSettingsPlugin->openOptionsNode(node,AName,tr("Account options"),MNI_ACCOUNT,ONO_ACCOUNTS);
  }
}

void AccountManager::closeAccountOptionsNode(const QUuid &AAccountId)
{
  if (FSettingsPlugin)
  {
    QString node = ON_ACCOUNTS"::"+AAccountId.toString();
    FSettingsPlugin->closeOptionsNode(node);
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
      openAccountOptionsNode(account->accountId(),AValue.toString());
  }
}

void AccountManager::onOpenAccountOptions(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
    openAccountOptionsDialog(action->data(ADR_ACCOUNT_ID).toString());
}

void AccountManager::onProfileOpened(const QString &/*AProfile*/)
{
  foreach(IAccount *account, FAccounts)
    account->setActive(FSettings->valueNS(SVN_ACCOUNT_ACTIVE,account->accountId().toString(),false).toBool());
}

void AccountManager::onProfileClosed(const QString &/*AProfile*/)
{
  foreach(IAccount *account, FAccounts)
  {
    FSettings->setValueNS(SVN_ACCOUNT_ACTIVE,account->accountId().toString(),account->isActive());
    account->setActive(false);
  }
}

void AccountManager::onSettingsOpened()
{
  //Заменим старый идентификатор аккаунта на новый
  QDomElement accountElem = FSettingsPlugin->pluginNode(pluginUuid()).firstChildElement("account");
  while (!accountElem.isNull())
  {
    QString oldNS = accountElem.attribute("ns");
    if (!oldNS.isEmpty() && QUuid(oldNS).isNull())
    {
      accountElem.setAttribute("ns",QUuid::createUuid().toString());

      //Перекодируем пароль
      QDomElement passElem = accountElem.firstChildElement("password");
      if (!passElem.isNull())
        passElem.setAttribute("value",QString(qCompress(FSettings->encript(FSettings->decript(qUncompress(QByteArray::fromBase64(passElem.attribute("value").toLatin1())),oldNS.toUtf8()),accountElem.attribute("ns").toUtf8())).toBase64()));

      //Заменим старый идентификатор и в настройках DefaultConnection
      QDomElement conElem = FSettingsPlugin->pluginNode("{68F9B5F2-5898-43f8-9DD1-19F37E9779AC}").firstChildElement("connection");
      while (!conElem.isNull())
      {
        if (conElem.attribute("ns") == oldNS)
        {
          conElem.setAttribute("ns",accountElem.attribute("ns"));
          break;
        }
        conElem = conElem.nextSiblingElement("connection");
      }
    }
    accountElem = accountElem.nextSiblingElement("account");
  }

  foreach(QString id, FSettings->values(SVN_ACCOUNT).keys())
    appendAccount(id);
}

void AccountManager::onSettingsClosed()
{
  foreach(QUuid id, FAccounts.keys())
    removeAccount(id);
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
      connect(action,SIGNAL(triggered(bool)),SLOT(onOpenAccountOptions(bool)));
      AMenu->addAction(action,AG_RVCM_ACCOUNTMANAGER,true);
    }
  }
}

Q_EXPORT_PLUGIN2(plg_accountmanager, AccountManager)
