#ifndef INOTIFICATIONS_H
#define INOTIFICATIONS_H

#include <QMap>
#include <QIcon>
#include <QImage>
#include <utils/jid.h>

#define NOTIFICATIONS_UUID  "{59887A91-A483-4a7c-A2DE-227A01D6BC5E}"

struct INotification 
{
	enum NotifyKinds {
		RosterNotify          = 0x0001,
		PopupWindow           = 0x0002,
		TrayNotify            = 0x0004,
		TrayAction            = 0x0008,
		SoundPlay             = 0x0010,
		AlertWidget           = 0x0020,
		TabPageNotify         = 0x0040,
		ShowMinimized         = 0x0080,
		AutoActivate          = 0x8000
	};
	enum NotifyFlags {
		RemoveInvisible       = 0x0001
	};
	INotification() {
		kinds = 0;
		flags = RemoveInvisible;
	}
	QString typeId;
	ushort kinds;
	ushort flags;
	QMap<int, QVariant> data;
};

struct INotificationType
{
	INotificationType() {
		order = 0;
		kindMask = 0;
		kindDefs = 0;
	}
	int order;
	QString title;
	ushort kindMask;
	ushort kindDefs;
};

class INotificationHandler
{
public:
	virtual bool showNotification(int AOrder, ushort AKind, int ANotifyId, const INotification &ANotification) =0;
};

class INotifications 
{
public:
	virtual QObject *instance() =0;
	virtual QList<int> notifications() const =0;
	virtual INotification notificationById(int ANotifyId) const =0;
	virtual int appendNotification(const INotification &ANotification) =0;
	virtual void activateNotification(int ANotifyId) =0;
	virtual void removeNotification(int ANotifyId) =0;
	virtual void registerNotificationType(const QString &ATypeId, const INotificationType &AType) =0;
	virtual QList<QString> notificationTypes() const =0;
	virtual INotificationType notificationType(const QString &ATypeId) const =0;
	virtual ushort notificationKinds(const QString &ATypeId) const =0;
	virtual void setNotificationKinds(const QString &ATypeId, ushort AKinds) =0;
	virtual void removeNotificationType(const QString &ATypeId) =0;
	virtual void insertNotificationHandler(int AOrder, INotificationHandler *AHandler) =0;
	virtual void removeNotificationHandler(int AOrder, INotificationHandler *AHandler) =0;
	virtual QImage contactAvatar(const Jid &AContactJid) const =0;
	virtual QIcon contactIcon(const Jid &AStreamJid, const Jid &AContactJid) const =0;
	virtual QString contactName(const Jid &AStreamJId, const Jid &AContactJid) const =0;
protected:
	virtual void notificationActivated(int ANotifyId) =0;
	virtual void notificationRemoved(int ANotifyId) =0;
	virtual void notificationAppend(int ANotifyId, INotification &ANotification) =0;
	virtual void notificationAppended(int ANotifyId, const INotification &ANotification) =0;
	virtual void notificationHandlerInserted(int AOrder, INotificationHandler *AHandler) =0;
	virtual void notificationHandlerRemoved(int AOrder, INotificationHandler *AHandler) =0;
};

Q_DECLARE_INTERFACE(INotificationHandler,"Vacuum.Plugin.INotificationHandler/1.1")
Q_DECLARE_INTERFACE(INotifications,"Vacuum.Plugin.INotifications/1.2")

#endif //INOTIFICATIONS_H
