#include "statusicons.h"

#include <QTimer>

#define SVN_DEFAULT_ICONFILE              "defaultIconFile"
#define SVN_RULES                         "rules"
#define SVN_USERRULES                     "rules:user[]"

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
  FStatusIconset = Skin::getSkinIconset(STATUS_ICONSETFILE);
  connect(FStatusIconset,SIGNAL(iconsetChanged()),SLOT(onStatusIconsetChanged()));

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

  loadIconFilesRules();

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
    FStatusIconset->openFile(AIconFile);
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
  case UserRule:
    FUserRules.insert(APattern,AIconFile);
    break;
  case DefaultRule:
    FDefaultRules.insert(APattern,AIconFile);
    break;
  }

  FJid2IconFile.clear();
  repaintRostersView();
  emit ruleInserted(APattern,AIconFile,ARuleType);
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
  repaintRostersView();
  emit ruleRemoved(APattern,ARuleType);
}

QIcon StatusIcons::iconByJid(const Jid &AStreamJid, const Jid &AJid) const
{

  int show = IPresence::Offline;
  IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->getPresence(AStreamJid) : NULL;
  if (presence)
  {
    IPresenceItem *presenceItem = presence->item(AJid);
    if (presenceItem)
      show = presenceItem->show();
  }

  QString subs;
  bool ask = false;
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
  QIcon icon = Skin::getSkinIconset(iconFile)->iconByName(iconName);

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
  return FStatusIconset->iconByName(iconNameByStatus(AShow,ASubscription,AAsk));
}

void StatusIcons::repaintRostersView()
{
  if (!FRepaintStarted)
  {
    QTimer::singleShot(0,this,SLOT(onRepaintRostersView()));
    FRepaintStarted = true;
  }
}

void StatusIcons::loadIconFilesRules()
{
  clearIconFilesRules();

  QStringList iconFiles = Skin::skinFiles("iconset","status","*.jisp");
  for (int i = 0; i < iconFiles.count(); ++i)
    iconFiles[i].prepend("status/");

  foreach(QString iconFile, iconFiles)
  {
    UnzipFile iconset(Skin::skinsDirectory()+"/"+Skin::skin()+"/iconset/"+iconFile);
    if (iconset.isValid())
    {
      QDomDocument doc;
      doc.setContent(iconset.fileData("statusicons.xml"));
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
    }
  }
}

void StatusIcons::clearIconFilesRules()
{
  foreach(QString rule, FIconFilesRules)
    removeRule(rule,IStatusIcons::DefaultRule);
  FIconFilesRules.clear();
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
  repaintRostersView();
}

Q_EXPORT_PLUGIN2(StatusIconsPlugin, StatusIcons)
