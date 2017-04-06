#ifndef BIRTHDAYREMINDER_H
#define BIRTHDAYREMINDER_H

#include <QTimer>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ibirthdayreminder.h>
#include <interfaces/ivcardmanager.h>
#include <interfaces/irostermanager.h>
#include <interfaces/ipresencemanager.h>
#include <interfaces/iavatars.h>
#include <interfaces/inotifications.h>
#include <interfaces/irostersmodel.h>
#include <interfaces/irostersview.h>
#include <interfaces/imessageprocessor.h>

class BirthdayReminder : 
	public QObject,
	public IPlugin,
	public IBirthdayReminder
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IBirthdayReminder);
	Q_PLUGIN_METADATA(IID "org.vacuum-im.plugins.BirthdayReminder");
public:
	BirthdayReminder();
	~BirthdayReminder();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return BIRTHDAYREMINDER_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
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
	void onRosterIndexToolTips(IRosterIndex *AIndex, quint32 ALabelId, QMap<int, QString> &AToolTips);
	void onVCardReceived(const Jid &AContactJid);
	void onRosterItemReceived(IRoster *ARoster, const IRosterItem &AItem, const IRosterItem &ABefore);
	void onOptionsOpened();
	void onOptionsClosed();
private:
	IAvatars *FAvatars;
	IVCardManager *FVCardManager;
	IRosterManager *FRosterManager;
	IPresenceManager *FPresenceManager;
	IRostersModel *FRostersModel;
	INotifications *FNotifications;
	IRostersViewPlugin *FRostersViewPlugin;
	IMessageProcessor *FMessageProcessor;
private:
	quint32 FBirthdayLabelId;
	QDate FNotifyDate;
	QTimer FNotifyTimer;
	QMap<int, Jid> FNotifies;
	QList<Jid> FNotifiedContacts;
private:
	QMap<Jid, QDate> FBirthdays;
	QMap<Jid, int> FUpcomingBirthdays;
};

#endif // BIRTHDAYREMINDER_H
