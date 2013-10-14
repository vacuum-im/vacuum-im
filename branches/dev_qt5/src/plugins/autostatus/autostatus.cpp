#include "autostatus.h"

#include <QCursor>

#define IDLE_TIMER_TIMEOUT  1000

AutoStatus::AutoStatus()
{
	FStatusChanger = NULL;
	FAccountManager = NULL;
	FOptionsManager = NULL;

	FAutoStatusId = STATUS_NULL_ID;
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
	APluginInfo->dependences.append(ACCOUNTMANAGER_UUID);
}

bool AutoStatus::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);

	IPlugin *plugin = APluginManager->pluginInterface("IStatusChanger").value(0,NULL);
	if (plugin)
	{
		FStatusChanger = qobject_cast<IStatusChanger *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IAccountManager").value(0,NULL);
	if (plugin)
	{
		FAccountManager = qobject_cast<IAccountManager *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IOptionsManager").value(0,NULL);
	if (plugin)
	{
		FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());
		if (FOptionsManager)
		{
			connect(FOptionsManager->instance(),SIGNAL(profileClosed(const QString &)),SLOT(onProfileClosed(const QString &)));
		}
	}

	connect(Options::instance(),SIGNAL(optionsOpened()),SLOT(onOptionsOpened()));
	return FStatusChanger!=NULL && FAccountManager!=NULL;
}

bool AutoStatus::initObjects()
{
	return true;
}

bool AutoStatus::initSettings()
{
	Options::setDefaultValue(OPV_AUTOSTARTUS_RULE_ENABLED,false);
	Options::setDefaultValue(OPV_AUTOSTARTUS_RULE_TIME,10*60);
	Options::setDefaultValue(OPV_AUTOSTARTUS_RULE_SHOW,IPresence::Away);
	Options::setDefaultValue(OPV_AUTOSTARTUS_RULE_PRIORITY,20);
	Options::setDefaultValue(OPV_AUTOSTARTUS_RULE_TEXT,tr("Auto status 'Away' due to inactivity for more than #(m) minutes"));

	if (FOptionsManager)
	{
		IOptionsDialogNode dnode = { ONO_AUTO_STATUS, OPN_AUTO_STATUS, tr("Auto status"), MNI_AUTOSTATUS };
		FOptionsManager->insertOptionsDialogNode(dnode);
		FOptionsManager->insertOptionsHolder(this);
	}
	return true;
}

bool AutoStatus::startPlugin()
{
	SystemManager::startSystemIdle();
	connect(SystemManager::instance(),SIGNAL(systemIdleChanged(int)),SLOT(onSystemIdleChanged(int)));
	return true;
}

QMultiMap<int, IOptionsWidget *> AutoStatus::optionsWidgets(const QString &ANodeId, QWidget *AParent)
{
	QMultiMap<int, IOptionsWidget *> widgets;
	if (ANodeId == OPN_AUTO_STATUS)
	{
		widgets.insertMulti(OWO_AUTOSTATUS, new StatusOptionsWidget(this,FStatusChanger,AParent));
	}
	return widgets;
}

QUuid AutoStatus::activeRule() const
{
	return FActiveRule;
}

QList<QUuid> AutoStatus::rules() const
{
	QList<QUuid> rulesIdList;
	foreach(const QString &ruleId, Options::node(OPV_AUTOSTARTUS_ROOT).childNSpaces("rule"))
		rulesIdList.append(ruleId);
	return rulesIdList;
}

IAutoStatusRule AutoStatus::ruleValue(const QUuid &ARuleId) const
{
	IAutoStatusRule rule;
	if (rules().contains(ARuleId))
	{
		OptionsNode ruleNode = Options::node(OPV_AUTOSTARTUS_RULE_ITEM,ARuleId.toString());
		rule.time = ruleNode.value("time").toInt();
		rule.show = ruleNode.value("show").toInt();
		rule.text = ruleNode.value("text").toString();
		rule.priority = ruleNode.value("priority").toInt();
	}
	return rule;
}

bool AutoStatus::isRuleEnabled(const QUuid &ARuleId) const
{
	if (rules().contains(ARuleId))
		return Options::node(OPV_AUTOSTARTUS_RULE_ITEM,ARuleId.toString()).value("enabled").toBool();
	return false;
}

void AutoStatus::setRuleEnabled(const QUuid &ARuleId, bool AEnabled)
{
	if (rules().contains(ARuleId))
	{
		Options::node(OPV_AUTOSTARTUS_RULE_ITEM,ARuleId.toString()).setValue(AEnabled,"enabled");
		emit ruleChanged(ARuleId);
	}
}

QUuid AutoStatus::insertRule(const IAutoStatusRule &ARule)
{
	QUuid ruleId = QUuid::createUuid();
	OptionsNode ruleNode = Options::node(OPV_AUTOSTARTUS_RULE_ITEM,ruleId.toString());
	ruleNode.setValue(true,"enabled");
	ruleNode.setValue(ARule.time,"time");
	ruleNode.setValue(ARule.show,"show");
	ruleNode.setValue(ARule.text,"text");
	ruleNode.setValue(ARule.priority,"priority");
	emit ruleInserted(ruleId);
	return ruleId;
}

void AutoStatus::updateRule(const QUuid &ARuleId, const IAutoStatusRule &ARule)
{
	if (rules().contains(ARuleId))
	{
		OptionsNode ruleNode = Options::node(OPV_AUTOSTARTUS_RULE_ITEM,ARuleId.toString());
		ruleNode.setValue(ARule.time,"time");
		ruleNode.setValue(ARule.show,"show");
		ruleNode.setValue(ARule.text,"text");
		ruleNode.setValue(ARule.priority,"priority");
		emit ruleChanged(ARuleId);
	}
}

void AutoStatus::removeRule(const QUuid &ARuleId)
{
	if (rules().contains(ARuleId))
	{
		Options::node(OPV_AUTOSTARTUS_ROOT).removeChilds("rule",ARuleId.toString());
		emit ruleRemoved(ARuleId);
	}
}

void AutoStatus::replaceDateTime(QString &AText, const QString &APattern, const QDateTime &ADateTime)
{
	int pos = 0;
	QRegExp regExp(APattern);
	regExp.setMinimal(true);
	while ((pos = regExp.indexIn(AText, pos)) != -1)
	{
		QString replText = !regExp.cap(1).isEmpty() ? ADateTime.toString(regExp.cap(1)) : ADateTime.toString();
		AText.replace(pos,regExp.matchedLength(),replText);
		pos += replText.size();
	}
}

void AutoStatus::prepareRule(IAutoStatusRule &ARule)
{
	replaceDateTime(ARule.text,"\\%\\((.*)\\)",QDateTime::currentDateTime());
	replaceDateTime(ARule.text,"\\$\\((.*)\\)",QDateTime::currentDateTime().addSecs(0-ARule.time));
	replaceDateTime(ARule.text,"\\#\\((.*)\\)",QDateTime(QDate::currentDate()).addSecs(ARule.time));
}

void AutoStatus::setActiveRule(const QUuid &ARuleId)
{
	if (FAccountManager && FStatusChanger && ARuleId!=FActiveRule)
	{
		if (!ARuleId.isNull() && rules().contains(ARuleId))
		{
			IAutoStatusRule rule = ruleValue(ARuleId);
			prepareRule(rule);
			if (FAutoStatusId == STATUS_NULL_ID)
			{
				FAutoStatusId = FStatusChanger->addStatusItem(tr("Auto status"),rule.show,rule.text,rule.priority);
				foreach(IAccount *account, FAccountManager->accounts())
				{
					if (account->isActive() && account->xmppStream()->isOpen())
					{
						Jid streamJid = account->xmppStream()->streamJid();
						int status = FStatusChanger->streamStatus(streamJid);
						int show = FStatusChanger->statusItemShow(status);
						if (show==IPresence::Online || show==IPresence::Chat)
						{
							FStreamStatus.insert(streamJid,status);
							FStatusChanger->setStreamStatus(streamJid, FAutoStatusId);
						}
					}
				}
			}
			else
			{
				FStatusChanger->updateStatusItem(FAutoStatusId,tr("Auto status"),rule.show,rule.text,rule.priority);
			}
		}
		else
		{
			foreach(const Jid &streamJid, FStreamStatus.keys())
				FStatusChanger->setStreamStatus(streamJid, FStreamStatus.take(streamJid));
			foreach(const Jid &streamJid, FStatusChanger->statusStreams(FAutoStatusId))
				FStatusChanger->setStreamStatus(streamJid,STATUS_MAIN_ID);
			FStatusChanger->removeStatusItem(FAutoStatusId);
			FAutoStatusId = STATUS_NULL_ID;
		}
		FActiveRule = ARuleId;
		emit ruleActivated(ARuleId);
	}
}

void AutoStatus::updateActiveRule()
{
	QUuid newRuleId;
	int ruleTime = 0;
	int idleSecs = SystemManager::systemIdle();

	foreach(const QUuid &ruleId, rules())
	{
		IAutoStatusRule rule = ruleValue(ruleId);
		if (isRuleEnabled(ruleId) && rule.time<idleSecs && rule.time>ruleTime)
		{
			newRuleId = ruleId;
			ruleTime = rule.time;
		}
	}
	setActiveRule(newRuleId);
}

void AutoStatus::onSystemIdleChanged(int ASeconds)
{
	Q_UNUSED(ASeconds);
	if (FStatusChanger)
	{
		int show = FStatusChanger->statusItemShow(FStatusChanger->mainStatus());
		if (!FActiveRule.isNull() || show==IPresence::Online || show==IPresence::Chat)
			updateActiveRule();
	}
}

void AutoStatus::onOptionsOpened()
{
	if (Options::node(OPV_AUTOSTARTUS_ROOT).childNSpaces("rule").isEmpty())
	{
		Options::node(OPV_AUTOSTARTUS_RULE_ITEM,QUuid::createUuid().toString()).setValue(true,"enabled");
	}
}

void AutoStatus::onProfileClosed(const QString &AName)
{
	Q_UNUSED(AName);
	setActiveRule(QUuid());
}
