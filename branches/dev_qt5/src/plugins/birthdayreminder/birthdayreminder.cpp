#include "birthdayreminder.h"

#include <definitions/notificationtypes.h>
#include <definitions/notificationdataroles.h>
#include <definitions/notificationtypeorders.h>
#include <definitions/soundfiles.h>
#include <definitions/menuicons.h>
#include <definitions/resources.h>
#include <definitions/optionvalues.h>
#include <definitions/vcardvaluenames.h>
#include <definitions/rosterlabels.h>
#include <definitions/rostertooltiporders.h>
#include <definitions/rosterindexkinds.h>
#include <definitions/rosterindexroles.h>
#include <utils/iconstorage.h>
#include <utils/datetime.h>
#include <utils/options.h>
#include <utils/logger.h>

#define NOTIFY_WITHIN_DAYS   4
#define NOTIFY_TIMEOUT       90000

static const QList<int> BirthdayRosterKinds = QList<int>() << RIK_CONTACT;

BirthdayReminder::BirthdayReminder()
{
	FAvatars = NULL;
	FVCardPlugin = NULL;
	FRosterPlugin = NULL;
	FPresencePlugin = NULL;
	FRostersModel = NULL;
	FNotifications = NULL;
	FRostersViewPlugin = NULL;
	FMessageProcessor = NULL;

	FBirthdayLabelId = 0;

	FNotifyTimer.setSingleShot(false);
	FNotifyTimer.setInterval(NOTIFY_TIMEOUT);
	connect(&FNotifyTimer,SIGNAL(timeout()),SLOT(onShowNotificationTimer()));
}

BirthdayReminder::~BirthdayReminder()
{

}

void BirthdayReminder::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Birthday Reminder");
	APluginInfo->description = tr("Reminds about birthdays of your friends");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(VCARD_UUID);
}

bool BirthdayReminder::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);

	IPlugin *plugin = APluginManager->pluginInterface("IVCardPlugin").value(0,NULL);
	if (plugin)
	{
		FVCardPlugin = qobject_cast<IVCardPlugin *>(plugin->instance());
		if (FVCardPlugin)
			connect(FVCardPlugin->instance(),SIGNAL(vcardReceived(const Jid &)),SLOT(onVCardReceived(const Jid &)));
	}

	plugin = APluginManager->pluginInterface("IAvatars").value(0,NULL);
	if (plugin)
	{
		FAvatars = qobject_cast<IAvatars *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
	if (plugin)
	{
		FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
		if (FRostersViewPlugin)
		{
			connect(FRostersViewPlugin->rostersView()->instance(),SIGNAL(indexToolTips(IRosterIndex *, quint32, QMap<int,QString> &)),
				SLOT(onRosterIndexToolTips(IRosterIndex *, quint32, QMap<int,QString> &)));
		}
	}

	plugin = APluginManager->pluginInterface("IRosterPlugin").value(0,NULL);
	if (plugin)
	{
		FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
		if (FRosterPlugin)
		{
			connect(FRosterPlugin->instance(),SIGNAL(rosterItemReceived(IRoster *, const IRosterItem &, const IRosterItem &)),
				SLOT(onRosterItemReceived(IRoster *, const IRosterItem &, const IRosterItem &)));
		}
	}

	plugin = APluginManager->pluginInterface("IPresencePlugin").value(0,NULL);
	if (plugin)
	{
		FPresencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IRostersModel").value(0,NULL);
	if (plugin)
	{
		FRostersModel = qobject_cast<IRostersModel *>(plugin->instance());
		if (FRostersModel)
		{
			connect(FRostersModel->instance(),SIGNAL(indexInserted(IRosterIndex *)),SLOT(onRosterIndexInserted(IRosterIndex *)));
		}
	}

	plugin = APluginManager->pluginInterface("IMessageProcessor").value(0,NULL);
	if (plugin)
	{
		FMessageProcessor = qobject_cast<IMessageProcessor *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("INotifications").value(0,NULL);
	if (plugin)
	{
		FNotifications = qobject_cast<INotifications *>(plugin->instance());
		if (FNotifications)
		{
			connect(FNotifications->instance(),SIGNAL(notificationActivated(int)), SLOT(onNotificationActivated(int)));
			connect(FNotifications->instance(),SIGNAL(notificationRemoved(int)), SLOT(onNotificationRemoved(int)));
		}
	}

	connect(Options::instance(),SIGNAL(optionsOpened()),SLOT(onOptionsOpened()));
	connect(Options::instance(),SIGNAL(optionsClosed()),SLOT(onOptionsClosed()));

	return FVCardPlugin!=NULL;
}

bool BirthdayReminder::initObjects()
{
	if (FNotifications)
	{
		INotificationType notifyType;
		notifyType.order = NTO_BIRTHDAY_NOTIFY;
		notifyType.icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_BIRTHDAY_NOTIFY);
		notifyType.title = tr("When reminding of upcoming birthdays");
		notifyType.kindMask = INotification::PopupWindow|INotification::SoundPlay;
		notifyType.kindDefs = notifyType.kindMask;
		FNotifications->registerNotificationType(NNT_BIRTHDAY,notifyType);
	}
	if (FRostersViewPlugin)
	{
		AdvancedDelegateItem label(RLID_BIRTHDAY_NOTIFY);
		label.d->kind = AdvancedDelegateItem::CustomData;
		label.d->data = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_BIRTHDAY_NOTIFY);
		FBirthdayLabelId = FRostersViewPlugin->rostersView()->registerLabel(label);
	}
	return true;
}

bool BirthdayReminder::initSettings()
{
	Options::setDefaultValue(OPV_BIRTHDAYREMINDER_STARTTIME,QTime(9,0));
	Options::setDefaultValue(OPV_BIRTHDAYREMINDER_STOPTIME,QTime(23,59,59));
	return true;
}

bool BirthdayReminder::startPlugin()
{
	FNotifyTimer.start();
	return true;
}

QDate BirthdayReminder::contactBithday(const Jid &AContactJid) const
{
	return FBirthdays.value(AContactJid.bare());
}

int BirthdayReminder::contactBithdayDaysLeft(const Jid &AContactJid) const
{
	QDate birthday = contactBithday(AContactJid);
	if (birthday.isValid())
	{
		QDate curDate = QDate::currentDate();
		if (curDate.month()<birthday.month() || (curDate.month()==birthday.month() && curDate.day()<=birthday.day()))
			birthday.setDate(curDate.year(),birthday.month(),birthday.day());
		else
			birthday.setDate(curDate.year()+1,birthday.month(),birthday.day());
		return curDate.daysTo(birthday);
	}
	return -1;
}

Jid BirthdayReminder::findContactStream(const Jid &AContactJid) const
{
	if (FRostersModel && FRosterPlugin)
	{
		foreach(const Jid &streamJid, FRostersModel->streams())
		{
			IRoster *roster = FRosterPlugin->findRoster(streamJid);
			if (roster && roster->rosterItem(AContactJid).isValid)
				return streamJid;
		}
	}
	return Jid::null;
}

void BirthdayReminder::updateBirthdaysStates()
{
	if (FNotifyDate != QDate::currentDate())
	{
		FNotifiedContacts.clear();
		FNotifyDate = QDate::currentDate();

		foreach(const Jid &contactJid, FBirthdays.keys())
			updateBirthdayState(contactJid);
	}
}

bool BirthdayReminder::updateBirthdayState(const Jid &AContactJid)
{
	bool notify = false;
	int daysLeft = contactBithdayDaysLeft(AContactJid);

	bool isStateChanged = false;
	if (daysLeft>=0 && daysLeft<=NOTIFY_WITHIN_DAYS)
	{
		notify = true;
		isStateChanged = !FUpcomingBirthdays.contains(AContactJid);
		FUpcomingBirthdays.insert(AContactJid,daysLeft);
	}
	else
	{
		isStateChanged = FUpcomingBirthdays.contains(AContactJid);
		FUpcomingBirthdays.remove(AContactJid);
	}

	if (isStateChanged && FRostersViewPlugin && FRostersModel)
	{
		QMultiMap<int, QVariant> findData;
		foreach(int kind, BirthdayRosterKinds)
			findData.insertMulti(RDR_KIND,kind);
		findData.insertMulti(RDR_PREP_BARE_JID,AContactJid.pBare());
		foreach (IRosterIndex *index, FRostersModel->rootIndex()->findChilds(findData,true))
			FRostersViewPlugin->rostersView()->insertLabel(FBirthdayLabelId,index);
	}

	return notify;
}

void BirthdayReminder::setContactBithday(const Jid &AContactJid, const QDate &ABirthday)
{
	Jid contactJid = AContactJid.bare();
	if (FBirthdays.value(contactJid) != ABirthday)
	{
		if (ABirthday.isValid())
			FBirthdays.insert(contactJid,ABirthday);
		else
			FBirthdays.remove(contactJid);
		updateBirthdayState(contactJid);
	}
}

void BirthdayReminder::onShowNotificationTimer()
{
	if (FNotifications && FNotifications->notifications().isEmpty())
	{
		QTime start = Options::node(OPV_BIRTHDAYREMINDER_STARTTIME).value().toTime();
		QTime stop = Options::node(OPV_BIRTHDAYREMINDER_STOPTIME).value().toTime();
		if (start<=QTime::currentTime() && QTime::currentTime()<=stop)
		{
			INotification notify;
			notify.kinds = FNotifications->enabledTypeNotificationKinds(NNT_BIRTHDAY);
			if ((notify.kinds & (INotification::PopupWindow|INotification::SoundPlay))>0)
			{
				updateBirthdaysStates();
				notify.typeId = NNT_BIRTHDAY;
				QSet<Jid> notifyList = FUpcomingBirthdays.keys().toSet() - FNotifiedContacts.toSet();
				foreach(const Jid &contactJid, notifyList)
				{
					Jid streamJid = findContactStream(contactJid);

					notify.data.insert(NDR_STREAM_JID,streamJid.full());
					notify.data.insert(NDR_CONTACT_JID,contactJid.full());
					notify.data.insert(NDR_ICON,IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_BIRTHDAY_NOTIFY));
					notify.data.insert(NDR_POPUP_CAPTION,tr("Birthday remind"));
					notify.data.insert(NDR_POPUP_TITLE,FNotifications->contactName(streamJid,contactJid));
					notify.data.insert(NDR_POPUP_IMAGE,FNotifications->contactAvatar(contactJid));

					QDate	birthday = contactBithday(contactJid);
					int daysLeft = FUpcomingBirthdays.value(contactJid);
					notify.data.insert(NDR_POPUP_TEXT,daysLeft>0 ? tr("Birthday in %n day(s),\n %1","",daysLeft).arg(birthday.toString(Qt::SystemLocaleLongDate)) : tr("Birthday today!"));

					if (daysLeft == 0)
						notify.data.insert(NDR_POPUP_TIMEOUT,0);
					else
						notify.data.remove(NDR_POPUP_TIMEOUT);

					notify.data.insert(NDR_SOUND_FILE,SDF_BIRTHDAY_NOTIFY);

					FNotifiedContacts.append(contactJid);
					FNotifies.insert(FNotifications->appendNotification(notify),contactJid);
				}
			}
		}
	}
}

void BirthdayReminder::onNotificationActivated(int ANotifyId)
{
	if (FNotifies.contains(ANotifyId))
	{
		if (FMessageProcessor)
		{
			Jid contactJid = FNotifies.value(ANotifyId);
			Jid streamJid = findContactStream(contactJid);
			IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->findPresence(streamJid) : NULL;
			QList<IPresenceItem> presences = presence!=NULL ? presence->findItems(contactJid) : QList<IPresenceItem>();
			FMessageProcessor->createMessageWindow(streamJid, !presences.isEmpty() ? presences.first().itemJid : contactJid, Message::Chat, IMessageHandler::SM_SHOW);
		}
		FNotifications->removeNotification(ANotifyId);
	}
}

void BirthdayReminder::onNotificationRemoved(int ANotifyId)
{
	if (FNotifies.contains(ANotifyId))
	{
		FNotifies.remove(ANotifyId);
	}
}

void BirthdayReminder::onRosterIndexInserted(IRosterIndex *AIndex)
{
	if (FRostersViewPlugin && BirthdayRosterKinds.contains(AIndex->kind()))
	{
		if (FUpcomingBirthdays.contains(AIndex->data(RDR_PREP_BARE_JID).toString()))
			FRostersViewPlugin->rostersView()->insertLabel(FBirthdayLabelId,AIndex);
	}
}

void BirthdayReminder::onRosterIndexToolTips(IRosterIndex *AIndex, quint32 ALabelId, QMap<int,QString> &AToolTips)
{
	if (ALabelId==AdvancedDelegateItem::DisplayId || ALabelId==FBirthdayLabelId)
	{
		Jid contactJid = AIndex->data(RDR_PREP_BARE_JID).toString();
		int daysLeft = contactBithdayDaysLeft(contactJid);
		if (daysLeft>=0 && daysLeft<=NOTIFY_WITHIN_DAYS)
		{
			if (ALabelId == FBirthdayLabelId)
			{
				QDate birthday = contactBithday(contactJid);
				QString tip = tr("%1 marks %n years","",QDate::currentDate().year() - birthday.year()).arg(QDate::currentDate().addDays(daysLeft).toString(Qt::DefaultLocaleLongDate));
				AToolTips.insert(RTTO_BIRTHDAY_NOTIFY,tip);
			}
			QString tip = daysLeft>0 ? tr("Birthday in %n day(s)!","",daysLeft) : tr("Birthday today!");
			AToolTips.insert(RTTO_BIRTHDAY_NOTIFY,tip);
		}
	}
}

void BirthdayReminder::onVCardReceived(const Jid &AContactJid)
{
	if (findContactStream(AContactJid).isValid())
	{
		IVCard *vcard = FVCardPlugin->getVCard(AContactJid);
		setContactBithday(AContactJid,DateTime(vcard->value(VVN_BIRTHDAY)).dateTime().date());
		vcard->unlock();
	}
}

void BirthdayReminder::onRosterItemReceived(IRoster *ARoster, const IRosterItem &AItem, const IRosterItem &ABefore)
{
	Q_UNUSED(ARoster);
	if (!ABefore.isValid && FVCardPlugin && FVCardPlugin->hasVCard(AItem.itemJid))
	{
		IVCard *vcard = FVCardPlugin->getVCard(AItem.itemJid);
		setContactBithday(AItem.itemJid,DateTime(vcard->value(VVN_BIRTHDAY)).dateTime().date());
		vcard->unlock();
	}
}

void BirthdayReminder::onOptionsOpened()
{
	FNotifyDate = Options::fileValue("birthdays.notify.date").toDate();
	QStringList notified = Options::fileValue("birthdays.notify.notified").toStringList();

	FNotifiedContacts.clear();
	foreach(const QString &contactJid, notified)
		FNotifiedContacts.append(contactJid);

	updateBirthdaysStates();
}

void BirthdayReminder::onOptionsClosed()
{
	QStringList notified;
	foreach (const Jid &contactJid, FNotifiedContacts)
		notified.append(contactJid.bare());

	Options::setFileValue(FNotifyDate,"birthdays.notify.date");
	Options::setFileValue(notified,"birthdays.notify.notified");
}
