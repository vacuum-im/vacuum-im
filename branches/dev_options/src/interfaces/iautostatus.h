#ifndef IAUTOSTATUS_H
#define IAUTOSTATUS_H

#define AUTOSTATUS_UUID   "{89687A92-B483-4d7a-B2CF-267A05D6CC5D}"

struct IAutoStatusRule {
  int time;
  int show;
  QString text;
};

class IAutoStatus {
public:
  virtual QObject *instance() =0;
  virtual int idleSeconds() const =0;
  virtual int activeRule() const =0;
  virtual int insertRule(const IAutoStatusRule &ARule) =0;
  virtual QList<int> rules() const =0;
  virtual IAutoStatusRule ruleValue(int ARuleId) const =0;
  virtual bool isRuleEnabled(int ARuleId) const =0;
  virtual void setRuleEnabled(int ARuleId, bool AEnabled) =0;
  virtual void updateRule(int ARuleId, const IAutoStatusRule &ARule) =0;
  virtual void removeRule(int ARuleId) =0;
protected:
  virtual void ruleInserted(int ARuleId) =0;
  virtual void ruleChanged(int ARuleId) =0;
  virtual void ruleRemoved(int ARuleId) =0;
  virtual void ruleActivated(int ARuleId) =0;
};

Q_DECLARE_INTERFACE(IAutoStatus,"Vacuum.Plugin.IAutoStatus/1.0")

#endif
