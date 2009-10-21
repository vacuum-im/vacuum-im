#ifndef AUTOSTATUS_H
#define AUTOSTATUS_H

#include <QTimer>
#include <QDateTime>
#include <definations/optionnodes.h>
#include <definations/optionnodeorders.h>
#include <definations/optionwidgetorders.h>
#include <definations/menuicons.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/iautostatus.h>
#include <interfaces/istatuschanger.h>
#include <interfaces/isettings.h>
#include "statusoptionswidget.h"

struct StatusRuleItem {
  int id;
  bool enabled;
  IAutoStatusRule rule;
};

class AutoStatus : 
  public QObject,
  public IPlugin,
  public IAutoStatus,
  public IOptionsHolder
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IAutoStatus IOptionsHolder);
public:
  AutoStatus();
  ~AutoStatus();
  //IPlugin
  virtual QObject *instance() { return this; }
  virtual QUuid pluginUuid() const { return AUTOSTATUS_UUID; }
  virtual void pluginInfo(IPluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects();
  virtual bool initSettings() { return true; }
  virtual bool startPlugin();
  //IOptionsHolder
  virtual QWidget *optionsWidget(const QString &ANode, int &AOrder);
  //IAutoStatus
  virtual int idleSeconds() const;
  virtual int activeRule() const;
  virtual int insertRule(const IAutoStatusRule &ARule);
  virtual QList<int> rules() const;
  virtual IAutoStatusRule ruleValue(int ARuleId) const;
  virtual bool isRuleEnabled(int ARuleId) const;
  virtual void setRuleEnabled(int ARuleId, bool AEnabled);
  virtual void updateRule(int ARuleId, const IAutoStatusRule &ARule);
  virtual void removeRule(int ARuleId);
signals:
  virtual void ruleInserted(int ARuleId);
  virtual void ruleChanged(int ARuleId);
  virtual void ruleRemoved(int ARuleId);
  virtual void ruleActivated(int ARuleId);
  virtual void optionsAccepted();
  virtual void optionsRejected();
protected:
  void setActiveRule(int ARuleId);
  void updateActiveRule();
protected slots:
  void onIdleTimerTimeout();
  void onSettingsOpened();
  void onProfileClosed(const QString &AProfileName);
  void onSettingsClosed();
private:
  IStatusChanger *FStatusChanger;
  ISettingsPlugin *FSettingsPlugin;
private:
  int FRuleId;
  int FActiveRule;
  int FLastStatusId;
  int FAutoStatusId;
  QTimer FIdleTimer;
  QPoint FLastCursorPos;
  QDateTime FLastCursorTime;
  QHash<int,StatusRuleItem> FRules;
};

#endif // AUTOSTATUS_H
