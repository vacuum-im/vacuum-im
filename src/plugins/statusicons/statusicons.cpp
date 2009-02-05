#include "statusicons.h"

#include <QTimer>

#define SVN_DEFAULT_SUBSTORAGE            "defaultSubStorage"
#define SVN_RULES                         "rules"
#define SVN_USERRULES                     "rules:user[]"

#define ADR_RULE                          Action::DR_Parametr1
#define ADR_SUBSTORAGE                    Action::DR_Parametr2

StatusIcons::StatusIcons()
{
  FPresencePlugin = NULL;
  FRosterPlugin = NULL;
  FRostersModel = NULL;
  FRostersViewPlugin = NULL;
  FSettingsPlugin = NULL;

  FDefaultStorage = NULL;
  FCustomIconMenu = NULL;
  FDefaultIconAction = NULL;
  FDataHolder = NULL;
  FStatusIconsChangedStarted = false;
}

StatusIcons::~StatusIcons()
{
  delete FCustomIconMenu;
}

void StatusIcons::pluginInfo(IPluginInfo *APluginInfo)
{
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->description = tr("Assign status icon to contact depended on users rules");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = tr("Status Icons Manager"); 
  APluginInfo->uid = STATUSICONS_UUID;
  APluginInfo->version = "0.1";
}

bool StatusIcons::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *plugin = APluginManager->getPlugins("IPresencePlugin").value(0,NULL);
  if (plugin)
  {
    FPresencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance());
  }

  plugin = APluginManager->getPlugins("IRosterPlugin").value(0,NULL);
  if (plugin)
  {
    FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
  }

  plugin = APluginManager->getPlugins("IRostersModel").value(0,NULL);
  if (plugin)
  {
    FRostersModel = qobject_cast<IRostersModel *>(plugin->instance());
  }

  plugin = APluginManager->getPlugins("IRostersViewPlugin").value(0,NULL);
  if (plugin)
  {
    FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
    if (FRostersViewPlugin)
    {
      connect(FRostersViewPlugin->rostersView(),SIGNAL(contextMenu(IRosterIndex *,Menu*)),
        SLOT(onRostersViewContextMenu(IRosterIndex *,Menu *)));
    }
  }

  plugin = APluginManager->getPlugins("ISettingsPlugin").value(0,NULL);
  if (plugin)
  {
    FSettingsPlugin = qobject_cast<ISettingsPlugin *>(plugin->instance());
    if (FSettingsPlugin)
    {
      connect(FSettingsPlugin->instance(),SIGNAL(settingsOpened()),SLOT(onSettingsOpened()));
      connect(FSettingsPlugin->instance(),SIGNAL(settingsClosed()),SLOT(onSettingsClosed()));
      connect(FSettingsPlugin->instance(),SIGNAL(optionsDialogAccepted()),SLOT(onOptionsAccepted()));
      connect(FSettingsPlugin->instance(),SIGNAL(optionsDialogRejected()),SLOT(onOptionsRejected()));
    }
  }

  return true;
}

bool StatusIcons::initObjects()
{
  FCustomIconMenu = new Menu;
  FCustomIconMenu->setTitle(tr("Status icon"));

  if (FRostersModel)
  {
    FDataHolder = new RosterIndexDataHolder(this);
    FRostersModel->insertDefaultDataHolder(FDataHolder);
  }

  if (FSettingsPlugin)
  {
    FSettingsPlugin->openOptionsNode(ON_STATUSICONS,tr("Status icons"),tr("Configure status icons"),MNI_STATUSICONS_OPTIONS);
    FSettingsPlugin->insertOptionsHolder(this);
  }

  FDefaultStorage = IconStorage::staticStorage(RSR_STORAGE_STATUSICONS);
  connect(FDefaultStorage,SIGNAL(storageChanged()),SLOT(onDefaultStorageChanged()));
  loadStorages();

  return true;
}

QWidget *StatusIcons::optionsWidget(const QString &ANode, int &AOrder)
{
  if (ANode == ON_STATUSICONS)
  {
    AOrder = OO_STATUSICONS;
    FIconsOptionWidget = new IconsOptionsWidget(this);
    return FIconsOptionWidget;
  }
  return NULL;
}

QString StatusIcons::defaultSubStorage() const
{
  return FDefaultStorage!=NULL ? FDefaultStorage->subStorage() : STORAGE_SHARED_DIR;
}

void StatusIcons::setDefaultSubStorage(const QString &ASubStorage)
{
  if (FDefaultStorage && FDefaultStorage->subStorage()!=ASubStorage)
  {
    if (IconStorage::availSubStorages(RSR_STORAGE_STATUSICONS).contains(ASubStorage))
      FDefaultStorage->setSubStorage(ASubStorage);
    else
      FDefaultStorage->setSubStorage(STORAGE_SHARED_DIR);
  }
}

void StatusIcons::insertRule(const QString &APattern, const QString &ASubStorage, RuleType ARuleType)
{
  if (APattern.isEmpty() || ASubStorage.isEmpty() || !QRegExp(APattern).isValid())
    return;

  switch(ARuleType)
  {
  case UserRule:
    FUserRules.insert(APattern,ASubStorage);
    break;
  case DefaultRule:
    FDefaultRules.insert(APattern,ASubStorage);
    break;
  }

  FJid2Storage.clear();
  emit ruleInserted(APattern,ASubStorage,ARuleType);
  startStatusIconsChanged();
}

QStringList StatusIcons::rules(RuleType ARuleType) const
{
  switch(ARuleType)
  {
  case UserRule:
    return FUserRules.keys();
  case DefaultRule:
    return FDefaultRules.keys();
  }
  return QList<QString>();
}

QString StatusIcons::ruleSubStorage(const QString &APattern, RuleType ARuleType) const
{
  switch(ARuleType)
  {
  case UserRule:
    return FUserRules.value(APattern,STORAGE_SHARED_DIR);
  case DefaultRule:
    return FDefaultRules.value(APattern,STORAGE_SHARED_DIR);
  }
  return FDefaultStorage!=NULL ? FDefaultStorage->subStorage() : STORAGE_SHARED_DIR;
}

void StatusIcons::removeRule(const QString &APattern, RuleType ARuleType)
{
  switch(ARuleType)
  {
  case UserRule:
    FUserRules.remove(APattern);
    break;
  case DefaultRule:
    FDefaultRules.remove(APattern);
    break;
  }
  FJid2Storage.clear();
  emit ruleRemoved(APattern,ARuleType);
  startStatusIconsChanged();
}

QIcon StatusIcons::iconByJid(const Jid &AStreamJid, const Jid &AContactJid) const
{
  IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->getPresence(AStreamJid) : NULL;
  int show = presence!=NULL ? presence->presenceItem(AContactJid).show : IPresence::Offline;

  bool ask = false;
  QString subscription = SUBSCRIPTION_NONE;
  IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AStreamJid) : NULL;
  if (roster)
  {
    IRosterItem ritem = roster->rosterItem(AContactJid);
    if (ritem.isValid)
    {
      subscription = ritem.subscription;
      ask = !ritem.ask.isEmpty();
    }
  }

  return iconByJidStatus(AContactJid,show,subscription,ask);
}

QIcon StatusIcons::iconByStatus(int AShow, const QString &ASubscription, bool AAsk) const
{
  return FDefaultStorage!=NULL ? FDefaultStorage->getIcon(iconKeyByStatus(AShow,ASubscription,AAsk)) : QIcon();
}

QIcon StatusIcons::iconByJidStatus(const Jid &AContactJid, int AShow, const QString &ASubscription, bool AAsk) const
{
  QString substorage = subStorageByJid(AContactJid);
  QString iconKey = iconKeyByStatus(AShow,ASubscription,AAsk);
  IconStorage *storage = FStorages.value(substorage,FDefaultStorage);
  return storage!=NULL ? storage->getIcon(iconKey) : QIcon();
}

QString StatusIcons::subStorageByJid(const Jid &AContactJid) const
{
  if (!FJid2Storage.contains(AContactJid))
  {
    QRegExp regExp;
    regExp.setCaseSensitivity(Qt::CaseInsensitive);

    QString substorage;
    foreach (QString pattern, FUserRules.keys())
    {
      regExp.setPattern(pattern);
      if (AContactJid.pFull().contains(regExp))
      {
        substorage = FUserRules.value(pattern);
        break;
      }
    }

    if (substorage.isEmpty())
    {
      foreach (QString pattern, FDefaultRules.keys())
      {
        regExp.setPattern(pattern);
        if (AContactJid.pFull().contains(regExp))
        {
          substorage = FDefaultRules.value(pattern);
          break;
        }
      }
    }

    if (substorage.isEmpty())
    {
      substorage = FDefaultStorage!=NULL ? FDefaultStorage->subStorage() : STORAGE_SHARED_DIR;
    }

    FJid2Storage.insert(AContactJid,substorage);
    return substorage;
  }
  return FJid2Storage.value(AContactJid);
}

QString StatusIcons::iconKeyByStatus(int AShow, const QString &ASubscription, bool AAsk) const
{
  switch (AShow)
  {
  case IPresence::Offline: 
    if (AAsk) 
      return STI_ASK;
    if (ASubscription==SUBSCRIPTION_NONE) 
      return STI_NOAUTH;
    return STI_OFFLINE;
  case IPresence::Online: 
    return STI_ONLINE;
  case IPresence::Chat: 
    return STI_CHAT;
  case IPresence::Away: 
    return STI_AWAY;
  case IPresence::ExtendedAway: 
    return STI_XAWAY;
  case IPresence::DoNotDisturb: 
    return STI_DND;
  case IPresence::Invisible: 
    return STI_INVISIBLE;
  default: 
    return STI_ERROR;
  }
}

void StatusIcons::loadStorages()
{
  clearStorages();

  FDefaultIconAction = new Action(FCustomIconMenu);
  FDefaultIconAction->setText(tr("Default"));
  FDefaultIconAction->setCheckable(true);
  connect(FDefaultIconAction,SIGNAL(triggered(bool)),SLOT(onSetCustomIconset(bool)));
  FCustomIconMenu->addAction(FDefaultIconAction,AG_DEFAULT-1,true);

  QStringList storages = FileStorage::availSubStorages(RSR_STORAGE_STATUSICONS);
  foreach(QString substorage, storages)
  {
    IconStorage *storage = new IconStorage(RSR_STORAGE_STATUSICONS,substorage,this);
    FStorages.insert(substorage,storage);
    
    QString pattern = storage->option(STO_PATTERN);
    if (!pattern.isEmpty())
    {
      insertRule(pattern,substorage,IStatusIcons::DefaultRule);
      FStatusRules += pattern;
    }

    QString name = storage->option(STORAGE_NAME);
    Action *action = new Action(FCustomIconMenu);
    action->setCheckable(true);
    action->setIcon(storage->getIcon(iconKeyByStatus(IPresence::Online,"",false)));
    action->setText(!name.isEmpty() ? name : substorage);
    action->setData(ADR_SUBSTORAGE,substorage);
    connect(action,SIGNAL(triggered(bool)),SLOT(onSetCustomIconset(bool)));
    FCustomIconActions.insert(substorage,action);
    FCustomIconMenu->addAction(action,AG_DEFAULT,true);
  }
}

void StatusIcons::clearStorages()
{
  FCustomIconMenu->clear();
  FCustomIconActions.clear();
  foreach(QString rule, FStatusRules)
    removeRule(rule,IStatusIcons::DefaultRule);
  FStatusRules.clear();
  qDeleteAll(FStorages);
}

void StatusIcons::startStatusIconsChanged()
{
  if (!FStatusIconsChangedStarted)
  {
    QTimer::singleShot(0,this,SLOT(onStatusIconsChangedTimer()));
    FStatusIconsChangedStarted = true;
  }
}

void StatusIcons::onStatusIconsChangedTimer()
{
  emit statusIconsChanged();
  FStatusIconsChangedStarted = false;
}

void StatusIcons::onRostersViewContextMenu(IRosterIndex *AIndex, Menu *AMenu)
{
  if (AIndex->type() == RIT_Contact || AIndex->type() == RIT_Agent)
  {
    QString rule = AIndex->data(RDR_BareJid).toString();
    QString substorage = ruleSubStorage(rule,IStatusIcons::UserRule);
    FDefaultIconAction->setIcon(iconByStatus(IPresence::Online,SUBSCRIPTION_BOTH,false));
    FDefaultIconAction->setData(ADR_RULE,rule);
    FDefaultIconAction->setChecked(FDefaultStorage!=NULL && FDefaultStorage->subStorage()==substorage);
    foreach(Action *action,FCustomIconActions)
    {
      action->setData(ADR_RULE,AIndex->data(RDR_BareJid));
      action->setChecked(action->data(ADR_SUBSTORAGE).toString() == substorage);
    }
    FCustomIconMenu->setIcon(iconByJidStatus(AIndex->data(RDR_Jid).toString(),IPresence::Online,SUBSCRIPTION_BOTH,false));
    AMenu->addAction(FCustomIconMenu->menuAction(),AG_STATUSICONS_ROSTER,true);
  }
}

void StatusIcons::onSettingsOpened()
{
  ISettings *settings = FSettingsPlugin->settingsForPlugin(STATUSICONS_UUID);
  setDefaultSubStorage(settings->value(SVN_DEFAULT_SUBSTORAGE,STORAGE_SHARED_DIR).toString());

  QHash<QString,QVariant> rules = settings->values(SVN_USERRULES);
  for (QHash<QString,QVariant>::const_iterator it = rules.constBegin(); it != rules.constEnd(); it++)
    insertRule(it.key(),it.value().toString(),UserRule);
}

void StatusIcons::onSettingsClosed()
{
  ISettings *settings = FSettingsPlugin->settingsForPlugin(STATUSICONS_UUID);
  settings->setValue(SVN_DEFAULT_SUBSTORAGE,defaultSubStorage());
  
  settings->deleteValue(SVN_RULES);  
  foreach(QString pattern, FUserRules.keys())
  {
    settings->setValueNS(SVN_USERRULES,pattern,FUserRules.value(pattern));
    removeRule(pattern,UserRule);
  }
}

void StatusIcons::onOptionsAccepted()
{
  if (!FIconsOptionWidget.isNull())
    FIconsOptionWidget->apply();
  emit optionsAccepted();
}

void StatusIcons::onOptionsRejected()
{
  emit optionsRejected();
}

void StatusIcons::onDefaultStorageChanged()
{
  IconStorage *storage = qobject_cast<IconStorage*>(sender());
  if (storage)
  {
    FJid2Storage.clear();
    emit defaultStorageChanged(storage->subStorage());
    emit defaultIconsChanged();
    startStatusIconsChanged();
  }
}

void StatusIcons::onSetCustomIconset(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    QString rule = action->data(ADR_RULE).toString();
    QString substorage = action->data(ADR_SUBSTORAGE).toString();
    if (substorage.isEmpty())
      removeRule(rule,IStatusIcons::UserRule);
    else
      insertRule(rule,substorage,IStatusIcons::UserRule);
  }
}

Q_EXPORT_PLUGIN2(StatusIconsPlugin, StatusIcons)
