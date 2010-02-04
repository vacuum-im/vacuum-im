#include "autostatus.h"

#include <QCursor>

#define SVN_RULE            "rules:rule[]"
#define SVN_RULE_ENABLED    SVN_RULE":enabled"
#define SVN_RULE_TIME       SVN_RULE":time"
#define SVN_RULE_SHOW       SVN_RULE":show"
#define SVN_RULE_TEXT       SVN_RULE":text"
#define SVN_RULE_PRIORITY   SVN_RULE":priority"

#define IDLE_TIMER_TIMEOUT  1000

AutoStatus::AutoStatus()
{
  FStatusChanger = NULL;
  FSettingsPlugin = NULL;
  
  FRuleId = 1;
  FActiveRule = 0;
  FAutoStatusId = STATUS_NULL_ID;
  FLastStatusId = STATUS_ONLINE;
  FLastCursorPos = QCursor::pos();
  FLastCursorTime = QDateTime::currentDateTime();

  FIdleTimer.setSingleShot(false);
  connect(&FIdleTimer,SIGNAL(timeout()),SLOT(onIdleTimerTimeout()));
}

AutoStatus::~AutoStatus()
{

}

void AutoStatus::pluginInfo(IPluginInfo *APluginInfo)
{
  APluginInfo->name = tr("Auto Status"); 
  APluginInfo->description = tr("Allows to change the status in accordance with the time of inactivity");
  APluginInfo->version = "1.0";
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->homePage = "http://www.vacuum-im.org";
  APluginInfo->dependences.append(STATUSCHANGER_UUID);
}

bool AutoStatus::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *plugin = APluginManager->pluginInterface("IStatusChanger").value(0,NULL);
  if (plugin)
  {
    FStatusChanger = qobject_cast<IStatusChanger *>(plugin->instance());
  }

  plugin = APluginManager->pluginInterface("ISettingsPlugin").value(0,NULL);
  if (plugin)
  {
    FSettingsPlugin = qobject_cast<ISettingsPlugin *>(plugin->instance());
    if (FSettingsPlugin)
    {
      connect(FSettingsPlugin->instance(),SIGNAL(settingsOpened()),SLOT(onSettingsOpened()));
      connect(FSettingsPlugin->instance(),SIGNAL(profileClosed(const QString &)),SLOT(onProfileClosed(const QString &)));
      connect(FSettingsPlugin->instance(),SIGNAL(settingsClosed()),SLOT(onSettingsClosed()));
    }
  }

  return FStatusChanger;
}

bool AutoStatus::initObjects()
{
  if (FSettingsPlugin)
  {
    FSettingsPlugin->openOptionsNode(ON_AUTO_STATUS,tr("Auto status"),tr("Edit auto status rules"),MNI_AUTOSTATUS,ONO_AUTO_STATUS);
    FSettingsPlugin->insertOptionsHolder(this);
  }
  return true;
}

bool AutoStatus::startPlugin()
{
  FIdleTimer.start(IDLE_TIMER_TIMEOUT);
  return true;
}

QWidget *AutoStatus::optionsWidget(const QString &ANode, int &/*AOrder*/)
{
  if (ANode == ON_AUTO_STATUS)
  {
    StatusOptionsWidget *widget = new StatusOptionsWidget(this,FStatusChanger,NULL);
    connect(widget,SIGNAL(optionsAccepted()),SIGNAL(optionsAccepted()));
    connect(FSettingsPlugin->instance(),SIGNAL(optionsDialogAccepted()),widget,SLOT(apply()));
    connect(FSettingsPlugin->instance(),SIGNAL(optionsDialogRejected()),SIGNAL(optionsRejected()));
    return widget;
  }
  return NULL;
}

int AutoStatus::idleSeconds() const
{
  return FLastCursorTime.secsTo(QDateTime::currentDateTime());
}

int AutoStatus::activeRule() const
{
  return FActiveRule;
}

int AutoStatus::insertRule(const IAutoStatusRule &ARule)
{
  StatusRuleItem item;
  item.id = FRuleId++;
  item.enabled = true;
  item.rule = ARule;
  FRules.insert(item.id,item);
  emit ruleInserted(item.id);
  return item.id;
}

QList<int> AutoStatus::rules() const
{
  return FRules.keys();
}

IAutoStatusRule AutoStatus::ruleValue(int ARuleId) const
{
  return FRules.value(ARuleId).rule;
}

bool AutoStatus::isRuleEnabled(int ARuleId) const
{
  return FRules.value(ARuleId).enabled;
}

void AutoStatus::setRuleEnabled(int ARuleId, bool AEnabled)
{
  if (isRuleEnabled(ARuleId) != AEnabled)
  {
    FRules[ARuleId].enabled = AEnabled;
    emit ruleChanged(ARuleId);
  }
}

void AutoStatus::updateRule(int ARuleId, const IAutoStatusRule &ARule)
{
  if (FRules.contains(ARuleId))
  {
    FRules[ARuleId].rule = ARule;
    emit ruleChanged(ARuleId);
  }
}

void AutoStatus::removeRule(int ARuleId)
{
  if (FRules.contains(ARuleId))
  {
    emit ruleRemoved(ARuleId);
    FRules.remove(ARuleId);
  }
}

void AutoStatus::setActiveRule(int ARuleId)
{
  if (FStatusChanger && ARuleId!=FActiveRule)
  {
    if (ARuleId>0 && FRules.contains(ARuleId))
    {
      IAutoStatusRule rule = FRules.value(ARuleId).rule;
      if (FAutoStatusId == STATUS_NULL_ID)
      {
        FLastStatusId = FStatusChanger->mainStatus();
        FAutoStatusId = FStatusChanger->addStatusItem(tr("Auto status"),rule.show,rule.text,FStatusChanger->statusItemPriority(FLastStatusId));
      }
      else
        FStatusChanger->updateStatusItem(FAutoStatusId,tr("Auto status"),rule.show,rule.text,FStatusChanger->statusItemPriority(FLastStatusId));
      
      if (FAutoStatusId!=FStatusChanger->mainStatus())
        FStatusChanger->setMainStatus(FAutoStatusId);
    }
    else
    {
      FStatusChanger->setMainStatus(FLastStatusId);
      FStatusChanger->removeStatusItem(FAutoStatusId);
      FAutoStatusId = STATUS_NULL_ID;
    }
    FActiveRule = ARuleId;
    emit ruleActivated(ARuleId);
  }
}

void AutoStatus::updateActiveRule()
{
  int ruleId = 0;
  int ruleTime = 0;
  int idleSecs = idleSeconds();
  
  foreach(StatusRuleItem item,FRules)
  {
    if (item.enabled && item.rule.time<idleSecs && item.rule.time>ruleTime)
    {
      ruleId = item.id;
      ruleTime = item.rule.time;
    }
  }
  setActiveRule(ruleId);
}

void AutoStatus::onIdleTimerTimeout()
{
  if (FLastCursorPos != QCursor::pos())
  {
    FLastCursorPos = QCursor::pos();
    FLastCursorTime = QDateTime::currentDateTime();
  }
  
  if (FStatusChanger)
  {
    int show = FStatusChanger->statusItemShow(FStatusChanger->mainStatus());
    if (FActiveRule>0 || show==IPresence::Online || show==IPresence::Chat)
      updateActiveRule();
  }
}

void AutoStatus::onSettingsOpened()
{
  FRules.clear();
  ISettings *settings = FSettingsPlugin->settingsForPlugin(AUTOSTATUS_UUID);
  QList<QString> nsList = settings->values(SVN_RULE).keys();
  if (nsList.isEmpty())
  {
    IAutoStatusRule rule;
    rule.time = 15*60;
    rule.show = IPresence::Away;
    rule.text = tr("Status changed automatically to 'away'");
    insertRule(rule);
  }
  else foreach (QString ns, nsList)
  {
    IAutoStatusRule rule;
    rule.time = settings->valueNS(SVN_RULE_TIME,ns).toInt();
    rule.show = settings->valueNS(SVN_RULE_SHOW,ns).toInt();
    rule.text = settings->valueNS(SVN_RULE_TEXT,ns).toString();
    setRuleEnabled(insertRule(rule),settings->valueNS(SVN_RULE_ENABLED,ns,true).toBool());
  }
}

void AutoStatus::onProfileClosed(const QString &/*AProfileName*/)
{
  setActiveRule(0);
  FLastCursorTime = QDateTime::currentDateTime();
}

void AutoStatus::onSettingsClosed()
{
  ISettings *settings = FSettingsPlugin->settingsForPlugin(AUTOSTATUS_UUID);
  
  foreach(QString ns, settings->values(SVN_RULE).keys())
    settings->deleteValueNS(SVN_RULE,ns);

  int index = 0;
  foreach(StatusRuleItem item, FRules)
  {
    if (item.rule.time>0 && !item.rule.text.isEmpty())
    {
      QString ns = QString::number(index++);
      settings->setValueNS(SVN_RULE_ENABLED,ns,item.enabled);
      settings->setValueNS(SVN_RULE_TIME,ns,item.rule.time);
      settings->setValueNS(SVN_RULE_SHOW,ns,item.rule.show);
      settings->setValueNS(SVN_RULE_TEXT,ns,item.rule.text);
    }
  }
}

Q_EXPORT_PLUGIN2(plg_autostatus, AutoStatus)
