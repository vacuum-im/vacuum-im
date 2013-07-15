#ifndef AUTOSTATUS_H
#define AUTOSTATUS_H

#include <QTimer>
#include <QDateTime>
#include <definitions/optionvalues.h>
#include <definitions/optionnodes.h>
#include <definitions/optionnodeorders.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/menuicons.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/iautostatus.h>
#include <interfaces/istatuschanger.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/iaccountmanager.h>
#include <interfaces/ipresence.h>
#include <utils/options.h>
#include <utils/systemmanager.h>
#include "statusoptionswidget.h"

class AutoStatus :
	public QObject,
	public IPlugin,
	public IAutoStatus,
	public IOptionsHolder
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IAutoStatus IOptionsHolder);
#ifdef HAVE_QT5
	Q_PLUGIN_METADATA(IID "org.jrudevels.vacuum.IAutoStatus");
#endif
public:
	AutoStatus();
	~AutoStatus();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return AUTOSTATUS_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin();
	//IOptionsHolder
	virtual QMultiMap<int, IOptionsWidget *> optionsWidgets(const QString &ANodeId, QWidget *AParent);
	//IAutoStatus
	virtual QUuid activeRule() const;
	virtual QList<QUuid> rules() const;
	virtual IAutoStatusRule ruleValue(const QUuid &ARuleId) const;
	virtual bool isRuleEnabled(const QUuid &ARuleId) const;
	virtual void setRuleEnabled(const QUuid &ARuleId, bool AEnabled);
	virtual QUuid insertRule(const IAutoStatusRule &ARule);
	virtual void updateRule(const QUuid &ARuleId, const IAutoStatusRule &ARule);
	virtual void removeRule(const QUuid &ARuleId);
signals:
	void ruleInserted(const QUuid &ARuleId);
	void ruleChanged(const QUuid &ARuleId);
	void ruleRemoved(const QUuid &ARuleId);
	void ruleActivated(const QUuid &ARuleId);
protected:
	void replaceDateTime(QString &AText, const QString &APattern, const QDateTime &ADateTime);
	void prepareRule(IAutoStatusRule &ARule);
	void setActiveRule(const QUuid &ARuleId);
	void updateActiveRule();
protected slots:
	void onSystemIdleChanged(int ASeconds);
	void onOptionsOpened();
	void onProfileClosed(const QString &AName);
private:
	IStatusChanger *FStatusChanger;
	IAccountManager *FAccountManager;
	IOptionsManager *FOptionsManager;
private:
	int FAutoStatusId;
	QUuid FActiveRule;
	QMap<Jid, int> FStreamStatus;
};

#endif // AUTOSTATUS_H
