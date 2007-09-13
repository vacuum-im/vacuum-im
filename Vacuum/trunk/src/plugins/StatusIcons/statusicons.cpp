#include "statusicons.h"

#include <QTimer>

#define SVN_DEFAULT_ICONFILE              "defaultIconFile"
#define SVN_RULES                         "rules"
#define SVN_JIDRULES                      "rules:jid[]"
#define SVN_CUSTOMRULES                   "rules:custom[]"
#define SVN_SERVICERULES                  "rules:service[]"

StatusIcons::StatusIcons()
{
  FPresencePlugin = NULL;
  FRosterPlugin = NULL;
  FRostersModelPlugin = NULL;
  FRostersViewPlugin = NULL;
  FSettingsPlugin = NULL;

  FDataHolder = NULL;
  FRepaintStarted = false;
  FDefaultIconFile = STATUS_ICONSETFILE;
}

StatusIcons::~StatusIcons()
{

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

bool StatusIcons::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
  AInitOrder = IO_STATUSICONS;

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

  plugin = APluginManager->getPlugins("IRostersModelPlugin").value(0,NULL);
  if (plugin)
  {
    FRostersModelPlugin = qobject_cast<IRostersModelPlugin *>(plugin->instance());
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

  return true;
}

bool StatusIcons::initObjects()
{
  FStatusIconset.openFile(STATUS_ICONSETFILE);
  if (FRostersModelPlugin && FRostersModelPlugin->rostersModel())
  {
    FDataHolder = new RosterIndexDataHolder(this);
    FRostersModelPlugin->rostersModel()->insertDefaultDataHolder(FDataHolder);
  }

  if (FSettingsPlugin)
  {
    FSettingsPlugin->openOptionsNode(ON_STATUSICONS,tr("Status icons"), tr("Configure status icons"),QIcon());
    FSettingsPlugin->appendOptionsHolder(this);
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
    FDefaultIconFile = AIconFile;
    FStatusIconset.openFile(AIconFile);
    FJid2IconFile.clear();
    repaintRostersView();
    emit defaultIconFileChanged(AIconFile);
  }
}

void StatusIcons::insertRule(const QString &APattern, const QString &AIconFile, RuleType ARuleType)
{
  if (APattern.isEmpty() || AIconFile.isEmpty() || !QRegExp(APattern).isValid())
    return;

  switch(ARuleType)
  {
  case JidRule:
    {
      Jid jid(APattern);
      if (jid.isValid())
        FJidRules.insert(jid.pBare(),AIconFile);
      break;
    }
  case CustomRule:
    {
      FCustomRules.insert(APattern,AIconFile);
      break;
    }
  case ServiceRule:
    {
      FServiceRules.insert(APattern,AIconFile);
      break;
    }
  }
  FJid2IconFile.clear();
  repaintRostersView();
  emit ruleInserted(APattern,AIconFile,ARuleType);
}

QStringList StatusIcons::rules(RuleType ARuleType) const
{
  switch(ARuleType)
  {
  case JidRule:
    return FJidRules.keys();
  case CustomRule:
    return FCustomRules.keys();
  case ServiceRule:
    return FServiceRules.keys();
  }
  return QList<QString>();
}

QString StatusIcons::ruleIconFile(const QString &APattern, RuleType ARuleType) const
{
  switch(ARuleType)
  {
  case JidRule:
    return FJidRules.value(APattern);
  case CustomRule:
    return FCustomRules.value(APattern);
  case ServiceRule:
    return FServiceRules.value(APattern);
  }
  return QString();
}

void StatusIcons::removeRule(const QString &APattern, RuleType ARuleType)
{
  switch(ARuleType)
  {
  case JidRule:
    {
      Jid jid(APattern);
      if (jid.isValid())
        FJidRules.remove(jid.pBare());
      break;
    }
  case CustomRule:
    {
      FCustomRules.remove(APattern);
      break;
    }
  case ServiceRule:
    {
      FServiceRules.remove(APattern);
      break;
    }
  }
  FJid2IconFile.clear();
  repaintRostersView();
  emit ruleRemoved(APattern,ARuleType);
}

QIcon StatusIcons::iconByJid(const Jid &AStreamJid, const Jid &AJid) const
{
  int show = IPresence::Offline;
  QString subs;
  bool ask = false;

  IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->getPresence(AStreamJid) : NULL;
  if (presence)
  {
    IPresenceItem *presenceItem = presence->item(AJid);
    if (presenceItem)
      show = presenceItem->show();
  }

  IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AStreamJid) : NULL;
  if (roster)
  {
    IRosterItem *rosterItem = roster->item(AJid);
    if (rosterItem)
    {
      subs = rosterItem->subscription();
      ask = !rosterItem->ask().isEmpty();
    }
  }

  return iconByJidStatus(AJid,show,subs,ask);
}

QIcon StatusIcons::iconByJidStatus(const Jid &AJid, int AShow, const QString &ASubscription, bool AAsk) const
{
  QString iconFile = iconFileByJid(AJid);
  QString iconName = iconNameByStatus(AShow,ASubscription,AAsk);
  Iconset iconset = Skin::getIconset(iconFile);
  if (!iconset.isValid())
    iconset = Skin::getDefIconset(iconFile);
  if (iconset.isValid())
    return iconset.iconByName(iconName);
  else
    return FStatusIconset.iconByName(iconName);
}

QString StatusIcons::iconFileByJid(const Jid &AJid) const
{
  QString full = AJid.pFull();
  if (FJid2IconFile.contains(full))
    return FJid2IconFile.value(full);

  QRegExp regExp;
  regExp.setCaseSensitivity(Qt::CaseInsensitive);

  QString bare = AJid.pBare();
  QStringList patterns = FJidRules.keys();
  foreach (QString pattern, patterns)
  {
    regExp.setPattern(pattern);
    if (bare.contains(regExp))
    {
      QString iconFile = FJidRules.value(pattern);
      FJid2IconFile.insert(full,iconFile);
      return iconFile;
    }
  }

  patterns = FCustomRules.keys();
  foreach (QString pattern, patterns)
  {
    regExp.setPattern(pattern);
    if (full.contains(regExp))
    {
      QString iconFile = FCustomRules.value(pattern);
      FJid2IconFile.insert(full,iconFile);
      return iconFile;
    }
  }

  QString domane = AJid.domane();
  patterns = FServiceRules.keys();
  foreach (QString pattern, patterns)
  {
    regExp.setPattern(pattern);
    if (domane.contains(regExp))
    {
      QString iconFile = FServiceRules.value(pattern);
      FJid2IconFile.insert(full,iconFile);
      return iconFile;
    }
  }

  FJid2IconFile.insert(full,FDefaultIconFile);  
  return FDefaultIconFile;
}

QString StatusIcons::iconNameByStatus(int AShow, const QString &ASubscription, bool AAsk) const
{
  if (AAsk)
    return "status/ask";

  if (ASubscription == "none")
    return "status/noauth";

  switch (AShow)
  {
  case IPresence::Offline: 
    return ("status/offline");
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
  return FStatusIconset.iconByName(iconNameByStatus(AShow,ASubscription,AAsk));
}

void StatusIcons::repaintRostersView()
{
  if (!FRepaintStarted)
  {
    QTimer::singleShot(0,this,SLOT(onRepaintRostersView()));
    FRepaintStarted = true;
  }
}

void StatusIcons::onRepaintRostersView()
{
  if (FRostersViewPlugin && FRostersViewPlugin->rostersView())
    FRostersViewPlugin->rostersView()->viewport()->repaint();
  FRepaintStarted = false;
}

void StatusIcons::onSettingsOpened()
{
  ISettings *settings = FSettingsPlugin->settingsForPlugin(STATUSICONS_UUID);
  setDefaultIconFile(settings->value(SVN_DEFAULT_ICONFILE,STATUS_ICONSETFILE).toString());

  QHash<QString,QVariant> rules = settings->values(SVN_JIDRULES);
  QHash<QString,QVariant>::const_iterator it = rules.constBegin();
  for (; it != rules.constEnd(); it++)
    insertRule(it.key(),it.value().toString(),JidRule);

  rules = settings->values(SVN_CUSTOMRULES);
  it = rules.constBegin();
  for (; it != rules.constEnd(); it++)
    insertRule(it.key(),it.value().toString(),CustomRule);

  rules = settings->values(SVN_SERVICERULES);
  it = rules.constBegin();
  for (; it != rules.constEnd(); it++)
    insertRule(it.key(),it.value().toString(),ServiceRule);
}

void StatusIcons::onSettingsClosed()
{
  ISettings *settings = FSettingsPlugin->settingsForPlugin(STATUSICONS_UUID);
  settings->setValue(SVN_DEFAULT_ICONFILE,defaultIconFile());
  
  settings->deleteValue(SVN_RULES);
  
  QList<QString> patterns = FJidRules.keys();
  foreach(QString pattern, patterns)
  {
    settings->setValueNS(SVN_JIDRULES,pattern,FJidRules.value(pattern));
    removeRule(pattern,JidRule);
  }
  
  patterns = FCustomRules.keys();
  foreach(QString pattern, patterns)
  {
    settings->setValueNS(SVN_CUSTOMRULES,pattern,FCustomRules.value(pattern));
    removeRule(pattern,CustomRule);
  }
  
  patterns = FServiceRules.keys();
  foreach(QString pattern, patterns)
  {
    settings->setValueNS(SVN_SERVICERULES,pattern,FServiceRules.value(pattern));
    removeRule(pattern,ServiceRule);
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

Q_EXPORT_PLUGIN2(StatusIconsPlugin, StatusIcons)
