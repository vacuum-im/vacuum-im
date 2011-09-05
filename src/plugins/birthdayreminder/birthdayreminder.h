#ifndef BIRTHDAYREMINDER_H
#define BIRTHDAYREMINDER_H

#include <QTimer>
#include <definitions/notificationtypes.h>
#include <definitions/notificationdataroles.h>
#include <definitions/notificationtypeorders.h>
#include <definitions/soundfiles.h>
#include <definitions/menuicons.h>
#include <definitions/resources.h>
#include <definitions/vcardvaluenames.h>
#include <definitions/rosterlabelorders.h>
#include <definitions/rostertooltiporders.h>
#include <definitions/rosterindextyperole.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ibirthdayreminder.h>
#include <interfaces/ivcard.h>
#include <interfaces/iroster.h>
#include <interfaces/ipresence.h>
#include <interfaces/iavatars.h>
#include <interfaces/inotifications.h>
#include <interfaces/irostersmodel.h>
#include <interfaces/irostersview.h>
#include <interfaces/imessageprocessor.h>
#include <utils/options.h>
#include <utils/datetime.h>
#include <utils/iconstorage.h>

class BirthdayReminder : 
	public QObject,
	public IPlugin,
	public IBirthdayReminder
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IBirthdayReminder);
public:
	BirthdayReminder();
	~BirthdayReminder();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return BIRTHDAYREMINDER_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings() { return true; }
	virtual bool startPlugin();
	//IBirthdayReminder
	virtual QDate contactBithday(const Jid &AContactJid) const;
	virtual int contactBithdayDaysLeft(const Jid &AContactJid) const;
protected:
	Jid findContactStream(const Jid &AContactJid) const;
	void updateBirthdaysStates();
	bool updateBirthdayState(const Jid &AContactJid);
	void setContactBithday(const Jid &AContactJid, const QDate &ABirthday);
protected slots:
	void onShowNotificationTimer();
	void onNotificationActivated(int ANotifyId);
	void onNotificationRemoved(int ANotifyId);
	void onRosterIndexInserted(IRosterIndex *AIndex);
	void onRosterIndexToolTips(IRosterIndex *AIndex, int ALabelId, QMultiMap<int,QString> &AToolTips);
	void onVCardReceived(const Jid &AContactJid);
	void onRosterItemReceived(IRoster *ARoster, const IRosterItem &AItem);
	void onOptionsOpened();
	void onOptionsClosed();
private:
	IAvatars *FAvatars;
	IVCardPlugin *FVCardPlugin;
	IRosterPlugin *FRosterPlugin;
	IPresencePlugin *FPresencePlugin;
	IRostersModel *FRostersModel;
	INotifications *FNotifications;
	IRostersViewPlugin *FRostersViewPlugin;
	IMessageProcessor *FMessageProcessor;
private:
	int FBirthdayLabelId;
	QDate FNotifyDate;
	QTimer FNotifyTimer;
	QMap<int, Jid> FNotifies;
	QList<Jid> FNotifiedContacts;
private:
	QMap<Jid, QDate> FBirthdays;
	QMap<Jid, int> FUpcomingBirthdays;
};

#endif // BIRTHDAYREMINDER_H
