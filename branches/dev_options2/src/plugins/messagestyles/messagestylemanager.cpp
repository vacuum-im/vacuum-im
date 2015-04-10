#include "messagestylemanager.h"

#include <QTimer>
#include <QCoreApplication>
#include <definitions/optionvalues.h>
#include <definitions/optionnodes.h>
#include <definitions/optionnodeorders.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/vcardvaluenames.h>
#include <definitions/menuicons.h>
#include <utils/message.h>
#include <utils/options.h>
#include <utils/logger.h>
#include "styleselectoptionswidget.h"

MessageStyleManager::MessageStyleManager()
{
	FAvatars = NULL;
	FStatusIcons = NULL;
	FVCardManager = NULL;
	FRosterManager = NULL;
	FOptionsManager = NULL;
}

MessageStyleManager::~MessageStyleManager()
{

}

void MessageStyleManager::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Message Styles Manager");
	APluginInfo->description = tr("Allows to use different styles to display messages");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
}

bool MessageStyleManager::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);

	IPlugin *plugin = APluginManager->pluginInterface("IOptionsManager").value(0,NULL);
	if (plugin)
	{
		FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IAvatars").value(0,NULL);
	if (plugin)
	{
		FAvatars = qobject_cast<IAvatars *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IStatusIcons").value(0,NULL);
	if (plugin)
	{
		FStatusIcons = qobject_cast<IStatusIcons *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IRosterManager").value(0,NULL);
	if (plugin)
	{
		FRosterManager = qobject_cast<IRosterManager *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IVCardManager").value(0,NULL);
	if (plugin)
	{
		FVCardManager = qobject_cast<IVCardManager *>(plugin->instance());
		if (FVCardManager)
		{
			connect(FVCardManager->instance(),SIGNAL(vcardReceived(const Jid &)),SLOT(onVCardChanged(const Jid &)));
			connect(FVCardManager->instance(),SIGNAL(vcardPublished(const Jid &)),SLOT(onVCardChanged(const Jid &)));
		}
	}

	connect(Options::instance(),SIGNAL(optionsChanged(const OptionsNode &)),SLOT(onOptionsChanged(const OptionsNode &)));

	return true;
}

bool MessageStyleManager::initSettings()
{
	Options::setDefaultValue(OPV_MESSAGES_SHOWDATESEPARATORS,true);
	Options::setDefaultValue(OPV_MESSAGES_MAXMESSAGESINWINDOW,500);

	if (FOptionsManager)
	{
		FOptionsManager->insertOptionsDialogHolder(this);
	}

	return true;
}

QMultiMap<int, IOptionsDialogWidget *> MessageStyleManager::optionsDialogWidgets(const QString &ANodeId, QWidget *AParent)
{
	QMultiMap<int, IOptionsDialogWidget *> widgets;
	if (ANodeId == OPN_APPEARANCE)
	{
		static const QList<int> messageTypes = QList<int>() << Message::Chat << Message::GroupChat << Message::Normal << Message::Headline << Message::Error;

		widgets.insertMulti(OHO_APPEARANCE_MESSAGESTYLE, FOptionsManager->newOptionsDialogHeader(tr("Messages styles"),AParent));

		int index = 0;
		foreach(int messageType, messageTypes)
		{
			widgets.insertMulti(OWO_APPEARANCE_MESSAGETYPESTYLE + index, new StyleSelectOptionsWidget(this,messageType,AParent));
			index++;
		}
	}
	else if (ANodeId == OPN_MESSAGES)
	{
		widgets.insertMulti(OWO_MESSAGES_SHOWDATESEPARATORS,FOptionsManager->newOptionsDialogWidget(Options::node(OPV_MESSAGES_SHOWDATESEPARATORS),tr("Show date separators in message window"),AParent));
	}
	return widgets;
}

QList<QString> MessageStyleManager::styleEngines() const
{
	return FStyleEngines.keys();
}

IMessageStyleEngine *MessageStyleManager::findStyleEngine(const QString &AEngineId) const
{
	return FStyleEngines.value(AEngineId);
}

void MessageStyleManager::registerStyleEngine(IMessageStyleEngine *AEngine)
{
	if (AEngine)
	{
		FStyleEngines.insert(AEngine->engineId(),AEngine);
		emit styleEngineRegistered(AEngine);
	}
}

IMessageStyle *MessageStyleManager::styleForOptions(const IMessageStyleOptions &AOptions) const
{
	IMessageStyleEngine *engine = findStyleEngine(AOptions.engineId);
	return engine!=NULL ? engine->styleForOptions(AOptions) : NULL;
}

IMessageStyleOptions MessageStyleManager::styleOptions(int AMessageType, const QString &AContext) const
{
	OptionsNode node = Options::node(OPV_MESSAGESTYLE_MTYPE_ITEM,QString::number(AMessageType)).node("context",AContext);
	QString engineId = node.value("engine-id").toString();

	// Select default style engine
	if (!FStyleEngines.contains(engineId))
	{
		switch (AMessageType)
		{
		case Message::Chat:
		case Message::GroupChat:
			engineId = "AdiumMessageStyle";
			break;
		default:
			engineId = "SimpleMessageStyle";
		}
	}

	// Select supported style engine
	if (!FStyleEngines.contains(engineId))
	{
		foreach(const QString &id, styleEngines())
		{
			IMessageStyleEngine *engine = findStyleEngine(id);
			if (engine!=NULL && engine->supportedMessageTypes().contains(AMessageType))
			{
				engineId = engine->engineId();
				break;
			}
		}
	}

	IMessageStyleEngine *engine = findStyleEngine(engineId);
	return engine!=NULL ? engine->styleOptions(node.node("engine",engineId)) : IMessageStyleOptions();
}

QString MessageStyleManager::contactAvatar(const Jid &AContactJid) const
{
	return FAvatars!=NULL ? FAvatars->avatarFileName(FAvatars->avatarHash(AContactJid)) : QString::null;
}

QString MessageStyleManager::contactName(const Jid &AStreamJid, const Jid &AContactJid) const
{
	QString name;
	if (!AContactJid.isValid())
	{
		if (!FStreamNames.contains(AStreamJid.bare()))
		{
			IVCard *vcard = FVCardManager!=NULL ? FVCardManager->getVCard(AStreamJid.bare()) : NULL;
			if (vcard)
			{
				name = vcard->value(VVN_NICKNAME);
				vcard->unlock();
			}
			FStreamNames.insert(AStreamJid.bare(),name);
		}
		else
		{
			name = FStreamNames.value(AStreamJid.bare());
		}
	}
	else if (AStreamJid.pBare() == AContactJid.pBare())
	{
		name = !AContactJid.resource().isEmpty() ? AContactJid.resource() : AContactJid.uNode();
	}
	else
	{
		IRoster *roster = FRosterManager!=NULL ? FRosterManager->findRoster(AStreamJid) : NULL;
		name = roster!=NULL ? roster->findItem(AContactJid).name : QString::null;
	}

	if (name.isEmpty())
	{
		if (AContactJid.isValid())
			name = !AContactJid.node().isEmpty() ? AContactJid.uNode() : AContactJid.domain();
		else
			name = !AStreamJid.node().isEmpty() ? AStreamJid.uNode() : AStreamJid.domain();
	}

	return name;
}

QString MessageStyleManager::contactIcon(const Jid &AStreamJid, const Jid &AContactJid) const
{
	if (FStatusIcons)
	{
		QString iconKey;
		if (AContactJid.isValid())
			iconKey = FStatusIcons->iconKeyByJid(AStreamJid,AContactJid);
		else
			iconKey = FStatusIcons->iconKeyByStatus(IPresence::Online,SUBSCRIPTION_BOTH,false);
		QString iconset = FStatusIcons->iconsetByJid(AContactJid.isValid() ? AContactJid : AStreamJid);
		return FStatusIcons->iconFileName(iconset,iconKey);
	}
	return QString::null;
}

QString MessageStyleManager::contactIcon(const Jid &AContactJid, int AShow, const QString &ASubscription, bool AAsk) const
{
	if (FStatusIcons)
	{
		QString iconset = FStatusIcons->iconsetByJid(AContactJid);
		QString iconKey = FStatusIcons->iconKeyByStatus(AShow,ASubscription,AAsk);
		return FStatusIcons->iconFileName(iconset,iconKey);
	}
	return QString::null;
}

QString MessageStyleManager::dateSeparator(const QDate &ADate, const QDate &ACurDate) const
{
	static const QList<QString> mnames = QList<QString>() << tr("January") << tr("February") <<  tr("March") <<  tr("April")
		<< tr("May") << tr("June") << tr("July") << tr("August") << tr("September") << tr("October") << tr("November") << tr("December");
	static const QList<QString> dnames = QList<QString>() << tr("Monday") << tr("Tuesday") <<  tr("Wednesday") <<  tr("Thursday")
		<< tr("Friday") << tr("Saturday") << tr("Sunday");

	QString text;
	if (ADate == ACurDate)
		text = ADate.toString(tr("%1, %2 dd")).arg(tr("Today")).arg(mnames.value(ADate.month()-1));
	else if (ADate.year() == ACurDate.year())
		text = ADate.toString(tr("%1, %2 dd")).arg(dnames.value(ADate.dayOfWeek()-1)).arg(mnames.value(ADate.month()-1));
	else
		text = ADate.toString(tr("%1, %2 dd, yyyy")).arg(dnames.value(ADate.dayOfWeek()-1)).arg(mnames.value(ADate.month()-1));

	return text;
}

QString MessageStyleManager::timeFormat(const QDateTime &ATime, const QDateTime &ACurTime) const
{
	int daysDelta = ATime.daysTo(ACurTime);
	if (daysDelta > 365)
		return tr("d MMM yyyy hh:mm");
	else if (daysDelta > 0)
		return tr("d MMM hh:mm");
	return tr("hh:mm:ss");
}

void MessageStyleManager::appendPendingChanges(int AMessageType, const QString &AContext)
{
	if (FPendingChages.isEmpty())
		QTimer::singleShot(0,this,SLOT(onApplyPendingChanges()));

	QPair<int,QString> item = qMakePair<int,QString>(AMessageType,AContext);
	if (!FPendingChages.contains(item))
		FPendingChages.append(item);
}

void MessageStyleManager::onVCardChanged(const Jid &AContactJid)
{
	if (FStreamNames.contains(AContactJid.bare()))
	{
		IVCard *vcard = FVCardManager!=NULL ? FVCardManager->getVCard(AContactJid) : NULL;
		if (vcard)
		{
			FStreamNames.insert(AContactJid.bare(),vcard->value(VVN_NICKNAME));
			vcard->unlock();
		}
	}
}

void MessageStyleManager::onOptionsChanged(const OptionsNode &ANode)
{
	QString cleanPath = Options::cleanNSpaces(ANode.path());
	if (cleanPath == OPV_MESSAGESTYLE_CONTEXT_ENGINEID)
	{
		QList<QString> nspaces = ANode.parentNSpaces();
		appendPendingChanges(nspaces.value(1).toInt(),nspaces.value(2));
	}
	else if (cleanPath == OPV_MESSAGESTYLE_ENGINE_STYLEID)
	{
		QList<QString> nspaces = ANode.parentNSpaces();
		appendPendingChanges(nspaces.value(1).toInt(),nspaces.value(2));
	}
	else if (cleanPath.startsWith(OPV_MESSAGESTYLE_STYLE_ITEM"."))
	{
		QList<QString> nspaces = ANode.parentNSpaces();
		QString mtype = nspaces.value(1);
		QString context = nspaces.value(2);
		QString engineId = nspaces.value(3);
		QString styleId = nspaces.value(4);

		if (!engineId.isEmpty() && !styleId.isEmpty())
		{
			OptionsNode node = Options::node(OPV_MESSAGESTYLE_MTYPE_ITEM,mtype).node("context",context);
			if (node.value("engine-id").toString() == engineId)
				if (node.node("engine",engineId).value("style-id").toString() == styleId)
					appendPendingChanges(mtype.toInt(),context);
		}
	}
}

void MessageStyleManager::onApplyPendingChanges()
{
	for (int i=0; i<FPendingChages.count(); i++)
	{
		const QPair<int,QString> &item = FPendingChages.at(i);
		emit styleOptionsChanged(styleOptions(item.first,item.second),item.first,item.second);
	}
	FPendingChages.clear();
}

Q_EXPORT_PLUGIN2(plg_messagestylemanager, MessageStyleManager)
