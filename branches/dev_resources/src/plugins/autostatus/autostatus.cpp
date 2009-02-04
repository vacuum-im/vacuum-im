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
  FOptionsWidget = NULL;
  
  FRuleId = 1;
  FActiveRule = 0;
  FAutoStatusId = NULL_STATUS_ID;
  FLastStatusId = STATUS_ONLINE;
  FLastCursorPos = QCursor::pos();
  FLastCursorTime = QDateTime::currentDateTime();

  FIdleTimer.setSingleShot(false);
  FIdleTimer.start(IDLE_TIMER_TIMEOUT);
  connect(&FIdleTimer,SIGNAL(timeout()),SLOT(onIdleTimerTimeout()));
}

AutoStatus::~AutoStatus()
{

}

void AutoStatus::pluginInfo(IPluginInfo *APluginInfo)
{
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->description = tr("Changing status depending on user activity");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = tr("Auto status"); 
  APluginInfo->uid = AUTOSTATUS_UUID;
  APluginInfo->version = "0.1";
  APluginInfo->dependences.append(STATUSCHANGER_UUID);
}

bool AutoStatus::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *plugin = APluginManager->getPlugins("IStatusChanger").value(0,NULL);
  if (plugin)
  {
    FStatusChanger = qobject_cast<IStatusChanger *>(plugin->instance());
  }

  plugin = APluginManager->getPlugins("ISettingsPlugin").value(0,NULL);
  if (plugin)
  {
    FSettingsPlugin = qobject_cast<ISettingsPlugin *>(plugin->instance());
    if (FSettingsPlugin)
    {
      connect(FSettingsPlugin->instance(),SIGNAL(settingsOpened()),SLOT(onSettingsOpened()));
      connect(FSettingsPlugin->instance(),SIGNAL(settingsClosed()),SLOT(onSettingsClosed()));
      connect(FSettingsPlugin->instance(),SIGNAL(optionsDialogAccepted()),SLOT(onOptionsDialogAccepted()));
      connect(FSettingsPlugin->instance(),SIGNAL(optionsDialogRejected()),SLOT(onOptionsDialogRejected()));
      connect(FSettingsPlugin->instance(),SIGNAL(optionsDialogClosed()),SLOT(onOptionsDialogClosed()));
    }
  }

  return FStatusChanger;
}

bool AutoStatus::initObjects()
{
  if (FSettingsPlugin)
  {
    FSettingsPlugin->openOptionsNode(ON_AUTO_STATUS,tr("Auto status"),tr("Edit auto status rules"),MNI_AUTOSTATUS);
    FSettingsPlugin->insertOptionsHolder(this);
  }
  return true;
}

QWidget *AutoStatus::optionsWidget(const QString &ANode, int &/*AOrder*/)
{
  if (ANode == ON_AUTO_STATUS)
  {
    FOptionsWidget = new StatusOptionsWidget(this,FStatusChanger,NULL);
    return FOptionsWidget;
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
  if (ARuleId!=FActiveRule)
  {
    if (ARuleId>0 && FRules.contains(ARuleId))
    {
      IAutoStatusRule rule = FRules.value(ARuleId).rule;
      if (FAutoStatusId == NULL_STATUS_ID)
      {
        FLastStatusId = FStatusChanger->mainStatus();
        FAutoStatusId = FStatusChanger->addStatusItem(tr("Auto status"),rule.show,rule.text,FStatusChanger->statusItemPriority(FLastStatusId));
      }
      else
        FStatusChanger->updateStatusItem(FAutoStatusId,tr("Auto status"),rule.show,rule.text,FStatusChanger->statusItemPriority(FLastStatusId));
      
      if (FAutoStatusId!=FStatusChanger->mainStatus())
        FStatusChanger->setStatus(FAutoStatusId);
    }
    else
    {
      FStatusChanger->setStatus(FLastStatusId);
      FStatusChanger->removeStatusItem(FAutoStatusId);
      FAutoStatusId = NULL_STATUS_ID;
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
  
  int show = FStatusChanger->statusItemShow(FStatusChanger->mainStatus());
  if (FActiveRule>0 || show==IPresence::Online || show==IPresence::Chat)
    updateActiveRule();
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

void AutoStatus::onOptionsDialogAccepted()
{
  FOptionsWidget->applyOptions();
  emit optionsAccepted();
}

void AutoStatus::onOptionsDialogRejected()
{
  emit optionsRejected();
}

void AutoStatus::onOptionsDialogClosed()
{
  FOptionsWidget = NULL;
}

Q_EXPORT_PLUGIN2(AutoStatusPlugin, AutoStatus)
