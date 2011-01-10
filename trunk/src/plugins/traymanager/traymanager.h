#ifndef TRAYMANAGER_H
#define TRAYMANAGER_H

#include <QMap>
#include <QTimer>
#include <definitions/actiongroups.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/itraymanager.h>

struct NotifyItem {
	QIcon icon;
	QString toolTip;
	bool blink;
};

class TrayManager :
			public QObject,
			public IPlugin,
			public ITrayManager
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin ITrayManager);
public:
	TrayManager();
	~TrayManager();
	virtual QObject *instance() { return this; }
	//IPlugin
	virtual QUuid pluginUuid() const { return TRAYMANAGER_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects() { return true; }
	virtual bool initSettings() { return true; }
	virtual bool startPlugin();
	//ITrayManager
	virtual QIcon currentIcon() const;
	virtual QString currentToolTip() const;
	virtual int currentNotify() const;
	virtual QIcon mainIcon() const;
	virtual void setMainIcon(const QIcon &AIcon);
	virtual QString mainToolTip() const;
	virtual void setMainToolTip(const QString &AToolTip);
	virtual bool isTrayIconVisible() const;
	virtual void setTrayIconVisible(bool AVisible);
	virtual void showMessage(const QString &ATitle, const QString &AMessage, QSystemTrayIcon::MessageIcon AIcon = QSystemTrayIcon::Information, int ATimeout = 10000);
	virtual void addAction(Action *AAction, int AGroup = AG_DEFAULT, bool ASort = false);
	virtual void removeAction(Action *AAction);
	virtual int appendNotify(const QIcon &AIcon, const QString &AToolTip, bool ABlink);
	virtual void updateNotify(int ANotifyId, const QIcon &AIcon, const QString &AToolTip, bool ABlink);
	virtual void removeNotify(int ANotifyId);
signals:
	void messageClicked();
	void messageShown(const QString &ATitle, const QString &AMessage,QSystemTrayIcon::MessageIcon AIcon, int ATimeout);
	void notifyAdded(int ANotifyId);
	void notifyActivated(int ANotifyId, QSystemTrayIcon::ActivationReason AReason);
	void notifyRemoved(int ANotifyId);
protected:
	void setTrayIcon(const QIcon &AIcon, const QString &AToolTip, bool ABlink);
protected slots:
	void onActivated(QSystemTrayIcon::ActivationReason AReason);
	void onBlinkTimer();
private:
	Menu *FContextMenu;
	Action *FQuitAction;
private:
	QTimer FBlinkTimer;
	QSystemTrayIcon FTrayIcon;
private:
	bool FBlinkShow;
	int FCurNotifyId;
	int FNextNotifyId;
	QIcon FCurIcon;
	QIcon FMainIcon;
	QString FMainToolTip;
	QMap<int,NotifyItem *> FNotifyItems;
};

#endif // TRAYMANAGER_H
