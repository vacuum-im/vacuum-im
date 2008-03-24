#include <QDebug>
#include "statusicons.h"

#include <QTimer>

#define SVN_DEFAULT_ICONFILE              "defaultIconFile"
#define SVN_RULES                         "rules"
#define SVN_USERRULES                     "rules:user[]"

#define ADR_RULE                          Action::DR_Parametr1
#define ADR_ICONSETNAME                   Action::DR_Parametr2

StatusIcons::StatusIcons()
{
  FPresencePlugin = NULL;
  FRosterPlugin = NULL;
  FRostersModel = NULL;
  FRostersViewPlugin = NULL;
  FSettingsPlugin = NULL;
  FSkinManager = NULL;

  FStatusIconset = NULL;
  FCustomIconMenu = NULL;
  FDefaultIconAction = NULL;
  FDataHolder = NULL;
  FStatusIconsChangedStarted = false;
}

StatusIcons::~StatusIcons()
{
  delete FCustomIconMenu;
}

void StatusIcons::pluginInfo(PluginInfo *APluginInfo)
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

  plugin = APluginManager->getPlugins("ISkinManager").value(0,NULL);
  if (plugin)
  {
    FSkinManager = qobject_cast<ISkinManager *>(plugin->instance());
    if (FSkinManager)
    {
      connect(FSkinManager->instance(),SIGNAL(skinChanged()),SLOT(onSkinChanged()));
    }
  }

  return true;
}

bool StatusIcons::initObjects()
{
  FCustomIconMenu = new Menu;
  FCustomIconMenu->setTitle(tr("Status icon"));

  setDefaultIconFile(STATUS_ICONSETFILE);
  
  if (!FSkinManager)
    loadIconFilesRules();

  if (FRostersModel)
  {
    FDataHolder = new RosterIndexDataHolder(this);
    FRostersModel->insertDefaultDataHolder(FDataHolder);
  }

  if (FRostersViewPlugin && FRostersViewPlugin->rostersView())
  {
    connect(FRostersViewPlugin->rostersView(),SIGNAL(contextMenu(IRosterIndex *,Menu*)),
      SLOT(onRostersViewContextMenu(IRosterIndex *,Menu *)));
  }

  if (FSettingsPlugin)
  {
    FSettingsPlugin->openOptionsNode(ON_STATUSICONS,tr("Status icons"), tr("Configure status icons"),QIcon());
    FSettingsPlugin->insertOptionsHolder(this);
  }

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

void StatusIcons::setDefaultIconFile(const QString &AIconFile)
{
  if (FDefaultIconFile != AIconFile && Skin::getIconset(AIconFile).isValid())
  {
    if (FStatusIconset)
      disconnect(FStatusIconset,SIGNAL(iconsetChanged()),this,SLOT(onStatusIconsetChanged()));
    FDefaultIconFile = AIconFile;
    FStatusIconset = Skin::getSkinIconset(AIconFile);
    connect(FStatusIconset,SIGNAL(iconsetChanged()),SLOT(onStatusIconsetChanged()));
    FJid2IconFile.clear();
    emit defaultIconFileChanged(AIconFile);
    emit defaultIconsChanged();
    startStatusIconsChanged();
  }
}

void StatusIcons::insertRule(const QString &APattern, const QString &AIconFile, RuleType ARuleType)
{
  if (APattern.isEmpty() || AIconFile.isEmpty() || !QRegExp(APattern).isValid())
    return;

  switch(ARuleType)
  {
  case UserRule:
    FUserRules.insert(APattern,AIconFile);
    break;
  case DefaultRule:
    FDefaultRules.insert(APattern,AIconFile);
    break;
  }

  FJid2IconFile.clear();
  emit ruleInserted(APattern,AIconFile,ARuleType);
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

QString StatusIcons::ruleIconFile(const QString &APattern, RuleType ARuleType) const
{
  switch(ARuleType)
  {
  case UserRule:
    return FUserRules.value(APattern);
  case DefaultRule:
    return FDefaultRules.value(APattern);
  }
  return QString();
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
  FJid2IconFile.clear();
  emit ruleRemoved(APattern,ARuleType);
  startStatusIconsChanged();
}

QIcon StatusIcons::iconByJid(const Jid &AStreamJid, const Jid &AJid) const
{

  int show = IPresence::Offline;
  IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->getPresence(AStreamJid) : NULL;
  if (presence)
    show = presence->presenceItem(AJid).show;

  QString subs;
  bool ask = false;
  IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AStreamJid) : NULL;
  if (roster)
  {
    IRosterItem ritem = roster->rosterItem(AJid);
    if (ritem.isValid)
    {
      subs = ritem.subscription;
      ask = !ritem.ask.isEmpty();
    }
  }

  return iconByJidStatus(AJid,show,subs,ask);
}

QIcon StatusIcons::iconByJidStatus(const Jid &AJid, int AShow, const QString &ASubscription, bool AAsk) const
{
  QString iconFile = iconFileByJid(AJid);
  QString iconName = iconNameByStatus(AShow,ASubscription,AAsk);
  SkinIconset *iconset = Skin::getSkinIconset(iconFile);
  QIcon icon = iconset->iconByName(iconName);
  return !icon.isNull() ? icon : FStatusIconset->iconByName(iconName);
}

QString StatusIcons::iconFileByJid(const Jid &AJid) const
{
  QString full = AJid.pFull();
  if (FJid2IconFile.contains(full))
    return FJid2IconFile.value(full);

  QRegExp regExp;
  regExp.setCaseSensitivity(Qt::CaseInsensitive);

  QStringList patterns = FUserRules.keys();
  foreach (QString pattern, patterns)
  {
    regExp.setPattern(pattern);
    if (full.contains(regExp))
    {
      QString iconFile = FUserRules.value(pattern);
      FJid2IconFile.insert(full,iconFile);
      return iconFile;
    }
  }

  patterns = FDefaultRules.keys();
  foreach (QString pattern, patterns)
  {
    regExp.setPattern(pattern);
    if (full.contains(regExp))
    {
      QString iconFile = FDefaultRules.value(pattern);
      FJid2IconFile.insert(full,iconFile);
      return iconFile;
    }
  }

  FJid2IconFile.insert(full,FDefaultIconFile);  
  return FDefaultIconFile;
}

QString StatusIcons::iconNameByStatus(int AShow, const QString &ASubscription, bool AAsk) const
{
  switch (AShow)
  {
  case IPresence::Offline: 
    {
      if (AAsk) 
        return "status/ask";
      if (ASubscription == "none") 
        return "status/noauth";
      return ("status/offline");
    }
  case IPresence::Online: 
    return ("status/online");
  case IPresence::Chat: 
    return ("status/chat");
  case IPresence::Away: 
    return ("status/away");
  case IPresence::ExtendedAway: 
    return ("status/xa");
  case IPresence::DoNotDistrib: 
    return ("status/dnd");
  case IPresence::Invisible: 
    return ("status/invisible");
  default: 
    return "status/error";
  }
}

QIcon StatusIcons::iconByStatus(int AShow, const QString &ASubscription, bool AAsk) const
{
  return FStatusIconset->iconByName(iconNameByStatus(AShow,ASubscription,AAsk));
}

void StatusIcons::startStatusIconsChanged()
{
  if (!FStatusIconsChangedStarted)
  {
    QTimer::singleShot(0,this,SLOT(onStatusIconsChangedTimer()));
    FStatusIconsChangedStarted = true;
  }
}

void StatusIcons::loadIconFilesRules()
{
  clearIconFilesRules();

  FDefaultIconAction = new Action(FCustomIconMenu);
  FDefaultIconAction->setText(tr(DEFAULT_SKIN_NAME));
  FDefaultIconAction->setCheckable(true);
  connect(FDefaultIconAction,SIGNAL(triggered(bool)),SLOT(onSetCustomIconset(bool)));
  FCustomIconMenu->addAction(FDefaultIconAction,AG_DEFAULT-1,true);

  QStringList iconFiles = Skin::skinFilesWithDef(SKIN_TYPE_ICONSET,"status","*.jisp");
  for (int i = 0; i < iconFiles.count(); ++i)
    iconFiles[i].prepend("status/");

  foreach(QString iconFile, iconFiles)
  {
    SkinIconset *iconset = Skin::getSkinIconset(iconFile); //(Skin::skinsDirectory()+"/"+Skin::skin()+"/iconset/"+iconFile);
    if (iconset->isValid())
    {
      QDomDocument doc;
      doc.setContent(iconset->fileData("statusicons.xml"));
      QDomElement elem = doc.firstChildElement().firstChildElement("rule");
      while(!elem.isNull())
      {
        QString pattern = elem.attribute("pattern");
        if (!pattern.isEmpty())
        {
          insertRule(pattern,iconFile,IStatusIcons::DefaultRule);
          FIconFilesRules += pattern;
        }
        elem = elem.nextSiblingElement("rule");
      }

      Action *action = new Action(FCustomIconMenu);
      action->setIcon(iconset->iconByName(iconNameByStatus(IPresence::Online,"",false)));
      action->setText(iconset->info().name);
      action->setData(ADR_ICONSETNAME,iconFile);
      action->setCheckable(true);
      connect(action,SIGNAL(triggered(bool)),SLOT(onSetCustomIconset(bool)));
      FCustomIconActions.insert(iconFile,action);
      FCustomIconMenu->addAction(action,AG_DEFAULT,true);
    }
  }
}

void StatusIcons::clearIconFilesRules()
{
  FCustomIconMenu->clear();
  FCustomIconActions.clear();
  foreach(QString rule, FIconFilesRules)
    removeRule(rule,IStatusIcons::DefaultRule);
  FIconFilesRules.clear();
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
    QString iconFile = ruleIconFile(rule,IStatusIcons::UserRule);
    FDefaultIconAction->setIcon(iconByStatus(IPresence::Online,"",false));
    FDefaultIconAction->setData(ADR_RULE,rule);
    FDefaultIconAction->setChecked(iconFile.isEmpty());
    foreach(Action *action,FCustomIconActions)
    {
      action->setData(ADR_RULE,AIndex->data(RDR_BareJid));
      action->setChecked(action->data(ADR_ICONSETNAME).toString() == iconFile);
    }
    AMenu->addAction(FCustomIconMenu->menuAction(),AG_STATUSICONS_ROSTER,true);
  }
}

void StatusIcons::onSettingsOpened()
{
  ISettings *settings = FSettingsPlugin->settingsForPlugin(STATUSICONS_UUID);
  setDefaultIconFile(settings->value(SVN_DEFAULT_ICONFILE,STATUS_ICONSETFILE).toString());

  QHash<QString,QVariant> rules = settings->values(SVN_USERRULES);
  QHash<QString,QVariant>::const_iterator it = rules.constBegin();
  for (; it != rules.constEnd(); it++)
    insertRule(it.key(),it.value().toString(),UserRule);

}

void StatusIcons::onSettingsClosed()
{
  ISettings *settings = FSettingsPlugin->settingsForPlugin(STATUSICONS_UUID);
  settings->setValue(SVN_DEFAULT_ICONFILE,defaultIconFile());
  
  settings->deleteValue(SVN_RULES);
  
  QStringList patterns = FUserRules.keys();
  foreach(QString pattern, patterns)
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

void StatusIcons::onStatusIconsetChanged()
{
  emit defaultIconsChanged();
  startStatusIconsChanged();
}

void StatusIcons::onSetCustomIconset(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    QString rule = action->data(ADR_RULE).toString();
    QString iconFile = action->data(ADR_ICONSETNAME).toString();
    if (iconFile.isEmpty())
      removeRule(rule,IStatusIcons::UserRule);
    else
      insertRule(rule,iconFile,IStatusIcons::UserRule);
  }
}

void StatusIcons::onSkinChanged()
{
  loadIconFilesRules();
}

Q_EXPORT_PLUGIN2(StatusIconsPlugin, StatusIcons)
