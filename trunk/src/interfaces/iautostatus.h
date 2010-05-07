#ifndef IAUTOSTATUS_H
#define IAUTOSTATUS_H

#include <QUuid>

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
	virtual QUuid activeRule() const =0;
	virtual QList<QUuid> rules() const =0;
	virtual IAutoStatusRule ruleValue(const QUuid &ARuleId) const =0;
	virtual bool isRuleEnabled(const QUuid &ARuleId) const =0;
	virtual void setRuleEnabled(const QUuid &ARuleId, bool AEnabled) =0;
	virtual QUuid insertRule(const IAutoStatusRule &ARule) =0;
	virtual void updateRule(const QUuid &ARuleId, const IAutoStatusRule &ARule) =0;
	virtual void removeRule(const QUuid &ARuleId) =0;
protected:
	virtual void ruleInserted(const QUuid &ARuleId) =0;
	virtual void ruleChanged(const QUuid &ARuleId) =0;
	virtual void ruleRemoved(const QUuid &ARuleId) =0;
	virtual void ruleActivated(const QUuid &ARuleId) =0;
};

Q_DECLARE_INTERFACE(IAutoStatus,"Vacuum.Plugin.IAutoStatus/1.0")

#endif
