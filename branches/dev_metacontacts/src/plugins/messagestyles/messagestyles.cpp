#include "messagestyles.h"

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

MessageStyles::MessageStyles()
{
	FAvatars = NULL;
	FStatusIcons = NULL;
	FVCardPlugin = NULL;
	FRosterPlugin = NULL;
	FOptionsManager = NULL;
}

MessageStyles::~MessageStyles()
{

}

void MessageStyles::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Message Styles Manager");
	APluginInfo->description = tr("Allows to use different styles to display messages");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
}

bool MessageStyles::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);

	QList<IPlugin *> plugins = APluginManager->pluginInterface("IMessageStylePlugin");
	foreach (IPlugin *plugin, plugins)
	{
		IMessageStylePlugin *stylePlugin = qobject_cast<IMessageStylePlugin *>(plugin->instance());
		if (stylePlugin)
			FStylePlugins.insert(stylePlugin->pluginId(),stylePlugin);
	}

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

	plugin = APluginManager->pluginInterface("IRosterPlugin").value(0,NULL);
	if (plugin)
	{
		FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IVCardPlugin").value(0,NULL);
	if (plugin)
	{
		FVCardPlugin = qobject_cast<IVCardPlugin *>(plugin->instance());
		if (FVCardPlugin)
		{
			connect(FVCardPlugin->instance(),SIGNAL(vcardReceived(const Jid &)),SLOT(onVCardChanged(const Jid &)));
			connect(FVCardPlugin->instance(),SIGNAL(vcardPublished(const Jid &)),SLOT(onVCardChanged(const Jid &)));
		}
	}

	connect(Options::instance(),SIGNAL(optionsChanged(const OptionsNode &)),SLOT(onOptionsChanged(const OptionsNode &)));

	return !FStylePlugins.isEmpty();
}

bool MessageStyles::initSettings()
{
	Options::setDefaultValue(OPV_MESSAGES_SHOWDATESEPARATORS,true);
	Options::setDefaultValue(OPV_MESSAGES_MAXMESSAGESINWINDOW,500);

	if (FOptionsManager)
	{
		IOptionsDialogNode dnode = { ONO_MESSAGE_STYLES, OPN_MESSAGE_STYLES, tr("Message Styles"), MNI_MESSAGE_STYLES };
		FOptionsManager->insertOptionsDialogNode(dnode);
		FOptionsManager->insertOptionsHolder(this);
	}
	return true;
}

QMultiMap<int, IOptionsWidget *> MessageStyles::optionsWidgets(const QString &ANodeId, QWidget *AParent)
{
	QMultiMap<int, IOptionsWidget *> widgets;
	if (ANodeId == OPN_MESSAGE_STYLES && !FStylePlugins.isEmpty())
	{
		widgets.insertMulti(OWO_MESSAGE_STYLES, new StyleOptionsWidget(this,AParent));
	}
	else if (ANodeId == OPN_MESSAGES)
	{
		widgets.insertMulti(OWO_MESSAGES_SHOWDATESEPARATORS,FOptionsManager->optionsNodeWidget(Options::node(OPV_MESSAGES_SHOWDATESEPARATORS),tr("Show date separators in chat window"),AParent));
	}
	return widgets;
}

QList<QString> MessageStyles::pluginList() const
{
	return FStylePlugins.keys();
}

IMessageStylePlugin *MessageStyles::pluginById(const QString &APluginId) const
{
	return FStylePlugins.value(APluginId,NULL);
}

IMessageStyle *MessageStyles::styleForOptions(const IMessageStyleOptions &AOptions) const
{
	IMessageStylePlugin *stylePlugin = pluginById(AOptions.pluginId);
	return stylePlugin!=NULL ? stylePlugin->styleForOptions(AOptions) : NULL;
}

IMessageStyleOptions MessageStyles::styleOptions(const OptionsNode &ANode, int AMessageType) const
{
	QString pluginId = ANode.value("style-type").toString();

	if (!FStylePlugins.contains(pluginId))
	{
		switch (AMessageType)
		{
		case Message::GroupChat:
		case Message::Chat:
			pluginId = "AdiumMessageStyle";
			break;
		default:
			pluginId = "SimpleMessageStyle";
		}
		if (!FStylePlugins.contains(pluginId))
			pluginId = FStylePlugins.keys().value(0);
	}

	IMessageStylePlugin *stylePlugin = pluginById(pluginId);
	return stylePlugin!=NULL ? stylePlugin->styleOptions(ANode.node("style",pluginId),AMessageType) : IMessageStyleOptions();
}

IMessageStyleOptions MessageStyles::styleOptions(int AMessageType, const QString &AContext) const
{
	OptionsNode node = Options::node(OPV_MESSAGESTYLE_MTYPE_ITEM,QString::number(AMessageType)).node("context",AContext);
	return styleOptions(node,AMessageType);
}

QString MessageStyles::contactAvatar(const Jid &AContactJid) const
{
	return FAvatars!=NULL ? FAvatars->avatarFileName(FAvatars->avatarHash(AContactJid)) : QString::null;
}

QString MessageStyles::contactName(const Jid &AStreamJid, const Jid &AContactJid) const
{
	QString name;
	if (!AContactJid.isValid())
	{
		if (!FStreamNames.contains(AStreamJid.bare()))
		{
			IVCard *vcard = FVCardPlugin!=NULL ? FVCardPlugin->getVCard(AStreamJid.bare()) : NULL;
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
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AStreamJid) : NULL;
		name = roster!=NULL ? roster->rosterItem(AContactJid).name : QString::null;
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

QString MessageStyles::contactIcon(const Jid &AStreamJid, const Jid &AContactJid) const
{
	if (FStatusIcons)
	{
		QString iconKey;
		if (AContactJid.isValid())
			iconKey = FStatusIcons->iconKeyByJid(AStreamJid,AContactJid);
		else
			iconKey = FStatusIcons->iconKeyByStatus(IPresence::Online,SUBSCRIPTION_BOTH,false);
		QString substorage = FStatusIcons->iconsetByJid(AContactJid.isValid() ? AContactJid : AStreamJid);
		return FStatusIcons->iconFileName(substorage,iconKey);
	}
	return QString::null;
}

QString MessageStyles::contactIcon(const Jid &AContactJid, int AShow, const QString &ASubscription, bool AAsk) const
{
	if (FStatusIcons)
	{
		QString iconKey = FStatusIcons->iconKeyByStatus(AShow,ASubscription,AAsk);
		QString substorage = FStatusIcons->iconsetByJid(AContactJid);
		return FStatusIcons->iconFileName(substorage,iconKey);
	}
	return QString::null;
}

QString MessageStyles::dateSeparator(const QDate &ADate, const QDate &ACurDate) const
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

QString MessageStyles::timeFormat(const QDateTime &ATime, const QDateTime &ACurTime) const
{
	int daysDelta = ATime.daysTo(ACurTime);
	if (daysDelta > 365)
		return tr("d MMM yyyy hh:mm");
	else if (daysDelta > 0)
		return tr("d MMM hh:mm");
	return tr("hh:mm:ss");
}

void MessageStyles::appendPendingChanges(int AMessageType, const QString &AContext)
{
	if (FPendingChages.isEmpty())
		QTimer::singleShot(0,this,SLOT(onApplyPendingChanges()));

	QPair<int,QString> item = qMakePair<int,QString>(AMessageType,AContext);
	if (!FPendingChages.contains(item))
		FPendingChages.append(item);
}

void MessageStyles::onVCardChanged(const Jid &AContactJid)
{
	if (FStreamNames.contains(AContactJid.bare()))
	{
		IVCard *vcard = FVCardPlugin!=NULL ? FVCardPlugin->getVCard(AContactJid) : NULL;
		if (vcard)
		{
			FStreamNames.insert(AContactJid.bare(),vcard->value(VVN_NICKNAME));
			vcard->unlock();
		}
	}
}

void MessageStyles::onOptionsChanged(const OptionsNode &ANode)
{
	QString cleanPath = Options::cleanNSpaces(ANode.path());
	if (cleanPath.startsWith(OPV_MESSAGESTYLE_STYLE_ITEM"."))
	{
		QList<QString> nspaces = ANode.parentNSpaces();
		QString type = nspaces.value(1);
		QString context = nspaces.value(2);
		QString plugin = nspaces.value(3);
		if (!plugin.isEmpty() && Options::node(OPV_MESSAGESTYLE_MTYPE_ITEM,type).node("context",context).value("style-type").toString() == plugin)
			appendPendingChanges(type.toInt(),context);
	}
	else if (cleanPath == OPV_MESSAGESTYLE_STYLE_TYPE)
	{
		QList<QString> nspaces = ANode.parentNSpaces();
		appendPendingChanges(nspaces.value(1).toInt(),nspaces.value(2));
	}
}

void MessageStyles::onApplyPendingChanges()
{
	for (int i=0; i<FPendingChages.count(); i++)
	{
		const QPair<int,QString> &item = FPendingChages.at(i);
		emit styleOptionsChanged(styleOptions(item.first,item.second),item.first,item.second);
	}
	FPendingChages.clear();
}

Q_EXPORT_PLUGIN2(plg_messagestyles, MessageStyles)
