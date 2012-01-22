#ifndef NOTIFICATIONS_H
#define NOTIFICATIONS_H

#include <QTimer>
#include <QSound>
#include <QPointer>
#include <definitions/notificationdataroles.h>
#include <definitions/actiongroups.h>
#include <definitions/toolbargroups.h>
#include <definitions/optionvalues.h>
#include <definitions/optionnodes.h>
#include <definitions/optionnodeorders.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/shortcuts.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/inotifications.h>
#include <interfaces/irostersview.h>
#include <interfaces/itraymanager.h>
#include <interfaces/iroster.h>
#include <interfaces/iavatars.h>
#include <interfaces/istatusicons.h>
#include <interfaces/ipresence.h>
#include <interfaces/istatuschanger.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/imainwindow.h>
#include <interfaces/imessagewidgets.h>
#include <utils/action.h>
#include <utils/options.h>
#include <utils/shortcuts.h>
#include <utils/widgetmanager.h>
#include "notifywidget.h"
#include "notifyoptionswidget.h"

struct NotifyRecord 
{
	NotifyRecord() {
		trayId=0;
		rosterId=0;
		tabPageId=0;
	}
	int trayId;
	int rosterId;
	int tabPageId;
	INotification notification;
	QPointer<Action> trayAction;
	QPointer<QObject> tabPageNotifier;
	QPointer<NotifyWidget> popupWidget;
};

struct TypeRecord 
{
	ushort kinds;
	INotificationType type;
};

class Notifications :
	public QObject,
	public IPlugin,
	public INotifications,
	public IOptionsHolder
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin INotifications IOptionsHolder);
public:
	Notifications();
	~Notifications();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return NOTIFICATIONS_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin();
	//IOptionsHolder
	virtual QMultiMap<int, IOptionsWidget *> optionsWidgets(const QString &ANodeId, QWidget *AParent);
	//INotifications
	virtual QList<int> notifications() const;
	virtual INotification notificationById(int ANotifyId) const;
	virtual int appendNotification(const INotification &ANotification);
	virtual void activateNotification(int ANotifyId);
	virtual void removeNotification(int ANotifyId);
	//Kind options for notification types
	virtual void registerNotificationType(const QString &ATypeId, const INotificationType &AType);
	virtual QList<QString> notificationTypes() const;
	virtual INotificationType notificationType(const QString &ATypeId) const;
	virtual void removeNotificationType(const QString &ATypeId);
	virtual ushort enabledNotificationKinds() const;
	virtual void setEnabledNotificationKinds(ushort AKinds);
	virtual ushort typeNotificationKinds(const QString &ATypeId) const;
	virtual ushort enabledTypeNotificationKinds(const QString &ATypeId) const;
	virtual void setTypeNotificationKinds(const QString &ATypeId, ushort AKinds);
	//Notification Handlers
	virtual void insertNotificationHandler(int AOrder, INotificationHandler *AHandler);
	virtual void removeNotificationHandler(int AOrder, INotificationHandler *AHandler);
	//Notification Utilities
	virtual QImage contactAvatar(const Jid &AContactJid) const;
	virtual QIcon contactIcon(const Jid &AStreamJid, const Jid &AContactJid) const;
	virtual QString contactName(const Jid &AStreamJId, const Jid &AContactJid) const;
signals:
	void notificationActivated(int ANotifyId);
	void notificationRemoved(int ANotifyId);
	void notificationAppend(int ANotifyId, INotification &ANotification);
	void notificationAppended(int ANotifyId, const INotification &ANotification);
	void notificationHandlerInserted(int AOrder, INotificationHandler *AHandler);
	void notificationHandlerRemoved(int AOrder, INotificationHandler *AHandler);
protected:
	int notifyIdByRosterId(int ARosterId) const;
	int notifyIdByTrayId(int ATrayId) const;
	int notifyIdByWidget(NotifyWidget *AWidget) const;
	bool showNotifyByHandler(ushort AKind, int ANotifyId, const INotification &ANotification) const;
	void removeInvisibleNotification(int ANotifyId);
protected slots:
	void onActivateDelayedActivations();
	void onSoundOnOffActionTriggered(bool);
	void onTrayActionTriggered(bool);
	void onRosterNotifyActivated(int ANotifyId);
	void onRosterNotifyRemoved(int ANotifyId);
	void onTrayNotifyActivated(int ANotifyId, QSystemTrayIcon::ActivationReason AReason);
	void onTrayNotifyRemoved(int ANotifyId);
	void onWindowNotifyActivated();
	void onWindowNotifyRemoved();
	void onWindowNotifyDestroyed();
	void onActionNotifyActivated(bool);
	void onOptionsOpened();
	void onOptionsChanged(const OptionsNode &ANode);
	void onShortcutActivated(const QString &AId, QWidget *AWidget);
private:
	IAvatars *FAvatars;
	IRosterPlugin *FRosterPlugin;
	IStatusIcons *FStatusIcons;
	IStatusChanger *FStatusChanger;
	ITrayManager *FTrayManager;
	IRostersModel *FRostersModel;
	IRostersViewPlugin *FRostersViewPlugin;
	IOptionsManager *FOptionsManager;
	IMainWindowPlugin *FMainWindowPlugin;
private:
	Menu *FNotifyMenu;
	Action *FSoundOnOff;
	Action *FRemoveAll;
	Action *FActivateLast;
	QList<int> FTrayNotifies;
private:
	int FNotifyId;
	QSound *FSound;
	QList<int> FDelayedActivations;
	QMap<int, NotifyRecord> FNotifyRecords;
	mutable QMap<QString, TypeRecord> FTypeRecords;
	QMultiMap<int, INotificationHandler *> FHandlers;
};

#endif // NOTIFICATIONS_H
