#include "statuschanger.h"

#include <QTimer>
#include <QToolButton>
#include <definitions/actiongroups.h>
#include <definitions/optionvalues.h>
#include <definitions/optionnodes.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/rosterlabels.h>
#include <definitions/rosterindexkinds.h>
#include <definitions/rosterindexroles.h>
#include <definitions/notificationtypes.h>
#include <definitions/notificationdataroles.h>
#include <definitions/notificationtypeorders.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/soundfiles.h>
#include <utils/options.h>
#include <utils/logger.h>

#define MAX_TEMP_STATUS_ID                  -10

#define ADR_STREAMJID                       Action::DR_StreamJid
#define ADR_STATUS_CODE                     Action::DR_Parametr1

StatusChanger::StatusChanger()
{
	FPluginManager = NULL;
	FPresencePlugin = NULL;
	FRosterPlugin = NULL;
	FMainWindowPlugin = NULL;
	FRostersView = NULL;
	FRostersViewPlugin = NULL;
	FRostersModel = NULL;
	FTrayManager = NULL;
	FOptionsManager = NULL;
	FAccountManager = NULL;
	FNotifications = NULL;

	FMainMenu = NULL;
	FModifyStatus = NULL;
	FStatusIcons = NULL;
	FChangingPresence = NULL;

	FConnectingLabelId = 0;
}

StatusChanger::~StatusChanger()
{
	if (!FEditStatusDialog.isNull())
		FEditStatusDialog->reject();
	if (!FModifyStatusDialog.isNull())
		FModifyStatusDialog->reject();
	delete FMainMenu;
}

void StatusChanger::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Status Manager");
	APluginInfo->description = tr("Allows to change the status in Jabber network");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(PRESENCE_UUID);
}

bool StatusChanger::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);

	FPluginManager = APluginManager;
	connect(APluginManager->instance(),SIGNAL(shutdownStarted()),SLOT(onApplicationShutdownStarted()));

	IPlugin *plugin = APluginManager->pluginInterface("IPresencePlugin").value(0,NULL);
	if (plugin)
	{
		FPresencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance());
		if (FPresencePlugin)
		{
			connect(FPresencePlugin->instance(),SIGNAL(presenceAdded(IPresence *)),SLOT(onPresenceAdded(IPresence *)));
			connect(FPresencePlugin->instance(),SIGNAL(presenceChanged(IPresence *, int, const QString &, int)),SLOT(onPresenceChanged(IPresence *, int, const QString &, int)));
			connect(FPresencePlugin->instance(),SIGNAL(presenceRemoved(IPresence *)),SLOT(onPresenceRemoved(IPresence *)));
		}
	}

	plugin = APluginManager->pluginInterface("IRosterPlugin").value(0,NULL);
	if (plugin)
	{
		FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
		if (FRosterPlugin)
		{
			connect(FRosterPlugin->instance(),SIGNAL(rosterOpened(IRoster *)),SLOT(onRosterOpened(IRoster *)));
			connect(FRosterPlugin->instance(),SIGNAL(rosterClosed(IRoster *)),SLOT(onRosterClosed(IRoster *)));
		}
	}

	plugin = APluginManager->pluginInterface("IMainWindowPlugin").value(0,NULL);
	if (plugin)
	{
		FMainWindowPlugin = qobject_cast<IMainWindowPlugin *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
	if (plugin)
	{
		FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
		if (FRostersViewPlugin)
		{
			FRostersView = FRostersViewPlugin->rostersView();
			connect(FRostersView->instance(),SIGNAL(indexContextMenu(const QList<IRosterIndex *> &, quint32, Menu *)),SLOT(onRostersViewIndexContextMenu(const QList<IRosterIndex *> &, quint32, Menu *)));
		}
	}

	plugin = APluginManager->pluginInterface("IRostersModel").value(0,NULL);
	if (plugin)
	{
		FRostersModel = qobject_cast<IRostersModel *>(plugin->instance());
		if (FRostersModel)
			connect(FRostersModel->instance(),SIGNAL(streamJidChanged(const Jid &, const Jid &)),SLOT(onStreamJidChanged(const Jid &, const Jid &)));
	}

	plugin = APluginManager->pluginInterface("IAccountManager").value(0,NULL);
	if (plugin)
	{
		FAccountManager = qobject_cast<IAccountManager *>(plugin->instance());
		if (FAccountManager)
			connect(FAccountManager->instance(),SIGNAL(changed(IAccount *, const OptionsNode &)),SLOT(onAccountOptionsChanged(IAccount *, const OptionsNode &)));
	}

	plugin = APluginManager->pluginInterface("ITrayManager").value(0,NULL);
	if (plugin)
	{
		FTrayManager = qobject_cast<ITrayManager *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IOptionsManager").value(0,NULL);
	if (plugin)
	{
		FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());
		if (FOptionsManager)
			connect(FOptionsManager->instance(),SIGNAL(profileOpened(const QString &)),SLOT(onProfileOpened(const QString &)),Qt::QueuedConnection);
	}

	plugin = APluginManager->pluginInterface("IStatusIcons").value(0,NULL);
	if (plugin)
	{
		FStatusIcons = qobject_cast<IStatusIcons *>(plugin->instance());
		if (FStatusIcons)
			connect(FStatusIcons->instance(),SIGNAL(defaultIconsChanged()),SLOT(onDefaultStatusIconsChanged()));
	}

	plugin = APluginManager->pluginInterface("INotifications").value(0,NULL);
	if (plugin)
	{
		FNotifications = qobject_cast<INotifications *>(plugin->instance());
		if (FNotifications)
			connect(FNotifications->instance(),SIGNAL(notificationActivated(int)), SLOT(onNotificationActivated(int)));
	}

	connect(Options::instance(),SIGNAL(optionsOpened()),SLOT(onOptionsOpened()));
	connect(Options::instance(),SIGNAL(optionsClosed()),SLOT(onOptionsClosed()));
	connect(Options::instance(),SIGNAL(optionsChanged(const OptionsNode &)),SLOT(onOptionsChanged(const OptionsNode &)));

	return FPresencePlugin!=NULL;
}

bool StatusChanger::initObjects()
{
	FMainMenu = new Menu;

	FModifyStatus = new Action(FMainMenu);
	FModifyStatus->setCheckable(true);
	FModifyStatus->setText(tr("Modify Status"));
	FModifyStatus->setIcon(RSR_STORAGE_MENUICONS, MNI_SCHANGER_MODIFY_STATUS);
	FMainMenu->addAction(FModifyStatus,AG_SCSM_STATUSCHANGER_ACTIONS,false);
	connect(FModifyStatus,SIGNAL(triggered(bool)),SLOT(onModifyStatusAction(bool)));

	Action *editStatus = new Action(FMainMenu);
	editStatus->setText(tr("Edit Statuses"));
	editStatus->setIcon(RSR_STORAGE_MENUICONS,MNI_SCHANGER_EDIT_STATUSES);
	connect(editStatus,SIGNAL(triggered(bool)), SLOT(onEditStatusAction(bool)));
	FMainMenu->addAction(editStatus,AG_SCSM_STATUSCHANGER_ACTIONS,false);

	createDefaultStatus();
	setMainStatusId(STATUS_OFFLINE);
	updateMainMenu();
	updateTrayToolTip();

	if (FOptionsManager)
		FOptionsManager->insertOptionsHolder(this);

	if (FMainWindowPlugin)
	{
		ToolBarChanger *changer = FMainWindowPlugin->mainWindow()->bottomToolBarChanger();
		QToolButton *button = changer->insertAction(FMainMenu->menuAction());
		button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
		button->setPopupMode(QToolButton::InstantPopup);
		button->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
	}

	if (FRostersViewPlugin)
	{
		AdvancedDelegateItem connectingLabel(RLID_SCHANGER_CONNECTING);
		connectingLabel.d->kind = AdvancedDelegateItem::CustomData;
		connectingLabel.d->flags = AdvancedDelegateItem::Blink;
		connectingLabel.d->data = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_SCHANGER_CONNECTING);
		FConnectingLabelId = FRostersViewPlugin->rostersView()->registerLabel(connectingLabel);
	}

	if (FTrayManager)
	{
		FTrayManager->contextMenu()->addAction(FMainMenu->menuAction(),AG_TMTM_STATUSCHANGER,true);
	}

	if (FNotifications)
	{
		INotificationType notifyType;
		notifyType.order = NTO_CONNECTION_ERROR;
		notifyType.icon = FStatusIcons!=NULL ? FStatusIcons->iconByStatus(IPresence::Error,QString::null,false) : QIcon();
		notifyType.title = tr("On loss of connection to the server");
		notifyType.kindMask = INotification::PopupWindow|INotification::SoundPlay;
		notifyType.kindDefs = notifyType.kindMask;
		FNotifications->registerNotificationType(NNT_CONNECTION_ERROR,notifyType);
	}

	return true;
}

bool StatusChanger::initSettings()
{
	Options::setDefaultValue(OPV_STATUS_SHOW,IPresence::Online);
	Options::setDefaultValue(OPV_STATUS_TEXT,nameByShow(IPresence::Online));
	Options::setDefaultValue(OPV_STATUS_PRIORITY,0);
	Options::setDefaultValue(OPV_STATUSES_MAINSTATUS,STATUS_ONLINE);
	Options::setDefaultValue(OPV_STATUSES_MODIFY,false);
	Options::setDefaultValue(OPV_ACCOUNT_AUTOCONNECT,false);
	Options::setDefaultValue(OPV_ACCOUNT_AUTORECONNECT,true);
	Options::setDefaultValue(OPV_ACCOUNT_STATUS_ISMAIN,true);
	Options::setDefaultValue(OPV_ACCOUNT_STATUS_LASTONLINE,STATUS_MAIN_ID);

	if (FOptionsManager)
	{
		FOptionsManager->insertOptionsHolder(this);
	}

	return true;
}

bool StatusChanger::startPlugin()
{
	updateMainMenu();
	return true;
}

QMultiMap<int, IOptionsWidget *> StatusChanger::optionsWidgets(const QString &ANodeId, QWidget *AParent)
{
	QMultiMap<int, IOptionsWidget *> widgets;
	QStringList nodeTree = ANodeId.split(".",QString::SkipEmptyParts);
	if (FOptionsManager && nodeTree.count()==2 && nodeTree.at(0)==OPN_ACCOUNTS)
	{
		OptionsNode aoptions = Options::node(OPV_ACCOUNT_ITEM,nodeTree.at(1));
		widgets.insertMulti(OWO_ACCOUNT_STATUS,FOptionsManager->optionsNodeWidget(aoptions.node("auto-connect"),tr("Auto connect on startup"),AParent));
		widgets.insertMulti(OWO_ACCOUNT_STATUS,FOptionsManager->optionsNodeWidget(aoptions.node("auto-reconnect"),tr("Auto reconnect if disconnected"),AParent));
	}
	return widgets;
}

Menu *StatusChanger::statusMenu() const
{
	return FMainMenu;
}

Menu *StatusChanger::streamMenu(const Jid &AStreamJid) const
{
	QMap<IPresence *, Menu *>::const_iterator it = FStreamMenu.constBegin();
	while (it!=FStreamMenu.constEnd())
	{
		if (it.key()->streamJid() == AStreamJid)
			return it.value();
		++it;
	}
	return NULL;
}

int StatusChanger::mainStatus() const
{
	return FStatusItems.value(STATUS_MAIN_ID).code;
}

void StatusChanger::setMainStatus(int AStatusId)
{
	setStreamStatus(Jid::null, AStatusId);
}

QList<Jid> StatusChanger::statusStreams(int AStatusId) const
{
	QList<Jid> streams;
	QMap<IPresence *, int>::const_iterator it = FCurrentStatus.constBegin();
	while (it!=FCurrentStatus.constEnd())
	{
		if (it.value() == AStatusId)
			streams.append(it.key()->streamJid());
		++it;
	}
	return streams;
}

int StatusChanger::streamStatus(const Jid &AStreamJid) const
{
	QMap<IPresence *, int>::const_iterator it = FCurrentStatus.constBegin();
	while (it!=FCurrentStatus.constEnd())
	{
		if (it.key()->streamJid() == AStreamJid)
			return it.value();
		++it;
	}
	return !AStreamJid.isValid() ? mainStatus() : STATUS_NULL_ID;
}

void StatusChanger::setStreamStatus(const Jid &AStreamJid, int AStatusId)
{
	if (FStatusItems.contains(AStatusId))
	{
		bool isSwitchOffline = false;
		bool isSwitchOnline = false;
		bool isChangeMainStatus = !AStreamJid.isValid() && AStatusId!=STATUS_MAIN_ID;

		StatusItem newStatus = FStatusItems.value(AStatusId);
		IPresence *mainPresence = visibleMainStatusPresence();
		StatusItem oldMainStatus = FStatusItems.value(FCurrentStatus.value(mainPresence,STATUS_OFFLINE));

		if (isChangeMainStatus)
		{
			if (newStatus.show==IPresence::Offline || newStatus.show==IPresence::Error)
				isSwitchOffline = true;
			else if (oldMainStatus.show==IPresence::Offline || oldMainStatus.show==IPresence::Error)
				isSwitchOnline = true;
			setMainStatusId(AStatusId);
		}

		for (QMap<IPresence *, int>::const_iterator it = FCurrentStatus.constBegin(); it!=FCurrentStatus.constEnd(); ++it)
		{
			IPresence *presence = it.key();
			int newStreamStatusId = AStatusId;
			StatusItem newStreamStatus = newStatus;

			bool acceptStatus = presence->streamJid()==AStreamJid;
			acceptStatus |= isChangeMainStatus && FMainStatusStreams.contains(presence);

			if (!acceptStatus && isSwitchOnline && !presence->xmppStream()->isConnected())
			{
				newStreamStatusId = FLastOnlineStatus.value(presence,STATUS_MAIN_ID);
				newStreamStatusId = FStatusItems.contains(newStreamStatusId) ? newStreamStatusId : STATUS_MAIN_ID;
				newStreamStatus = FStatusItems.value(newStreamStatusId);
				acceptStatus = true;
			}
			else if (isChangeMainStatus && presence==mainPresence)
			{
				acceptStatus = true;
			}
			else if (isSwitchOffline)
			{
				acceptStatus = true;
			}

			if (acceptStatus)
			{
				LOG_STRM_INFO(presence->streamJid(),QString("Changing stream status to=%1, show=%2, name=%3").arg(newStreamStatusId).arg(newStreamStatus.show).arg(newStreamStatus.name));

				if (newStreamStatusId == STATUS_MAIN_ID)
					FMainStatusStreams += presence;
				else if (presence->streamJid() == AStreamJid)
					FMainStatusStreams -= presence;

				FChangingPresence = presence;
				if (presence->setPresence(newStreamStatus.show, newStreamStatus.text, newStreamStatus.priority))
				{
					FChangingPresence = NULL;

					int statusId = FMainStatusStreams.contains(presence) ? STATUS_MAIN_ID : newStreamStatus.code;
					if (newStreamStatus.show!=IPresence::Offline && newStreamStatus.show!=IPresence::Error)
					{
						FLastOnlineStatus.insert(presence, statusId);
					}
					else if (presence->xmppStream()->isConnected())
					{
						LOG_STRM_INFO(presence->streamJid(),"Close XMPP stream due to offline status");
						presence->xmppStream()->close();
					}

					setStreamStatusId(presence, statusId);
					updateStreamMenu(presence);
				}
				else
				{
					FChangingPresence = NULL;
					if (newStreamStatus.show!=IPresence::Offline && !presence->xmppStream()->isConnected())
					{
						LOG_STRM_INFO(presence->streamJid(),"Open XMPP stream due to online status");
						if (presence->xmppStream()->open())
						{
							setStreamStatusId(presence, STATUS_CONNECTING_ID);

							int statusId = FMainStatusStreams.contains(presence) ? STATUS_MAIN_ID : newStreamStatus.code;
							FLastOnlineStatus.insert(presence, statusId);

							insertConnectingLabel(presence);
							FConnectStatus.insert(presence, statusId);
						}
					}
					if (!FConnectStatus.contains(presence) && isChangeMainStatus && it.value()==STATUS_MAIN_ID)
					{
						setStreamStatusId(presence,oldMainStatus.code);
					}
				}
			}
		}
		updateMainMenu();
	}
}

QString StatusChanger::statusItemName(int AStatusId) const
{
	if (FStatusItems.contains(AStatusId))
		return FStatusItems.value(AStatusId).name;
	return QString::null;
}

int StatusChanger::statusItemShow(int AStatusId) const
{
	if (FStatusItems.contains(AStatusId))
		return FStatusItems.value(AStatusId).show;
	return -1;
}

QString StatusChanger::statusItemText(int AStatusId) const
{
	if (FStatusItems.contains(AStatusId))
		return FStatusItems.value(AStatusId).text;
	return QString::null;
}

int StatusChanger::statusItemPriority(int AStatusId) const
{
	if (FStatusItems.contains(AStatusId))
		return FStatusItems.value(AStatusId).priority;
	return 0;
}

QList<int> StatusChanger::statusItems() const
{
	return FStatusItems.keys();
}

QList<int> StatusChanger::activeStatusItems() const
{
	QList<int> active;
	foreach (int statusId, FCurrentStatus)
		active.append(statusId > STATUS_NULL_ID ? statusId : FStatusItems.value(statusId).code);
	return active;
}

QList<int> StatusChanger::statusByShow(int AShow) const
{
	QList<int> statuses;
	for (QMap<int, StatusItem>::const_iterator it = FStatusItems.constBegin(); it!=FStatusItems.constEnd(); ++it)
	{
		if (it.key()>STATUS_NULL_ID && it->show==AShow)
			statuses.append(it->code);
	}
	return statuses;
}

int StatusChanger::statusByName(const QString &AName) const
{
	for (QMap<int, StatusItem>::const_iterator it = FStatusItems.constBegin(); it!=FStatusItems.constEnd(); ++it)
	{
		if (it->name.toLower() == AName.toLower())
			return it->code;
	}
	return STATUS_NULL_ID;
}

int StatusChanger::addStatusItem(const QString &AName, int AShow, const QString &AText, int APriority)
{
	int statusId = statusByName(AName);
	if (statusId==STATUS_NULL_ID && !AName.isEmpty())
	{
		statusId = qrand();
		while(statusId<=STATUS_MAX_STANDART_ID || FStatusItems.contains(statusId))
			statusId = (statusId > STATUS_MAX_STANDART_ID) ? statusId+1 : STATUS_MAX_STANDART_ID+1;

		StatusItem status;
		status.code = statusId;
		status.name = AName;
		status.show = AShow;
		status.text = AText;
		status.priority = APriority;
		FStatusItems.insert(statusId,status);
		createStatusActions(statusId);

		LOG_DEBUG(QString("Status item created, id=%1, show=%2, name=%3").arg(status.code).arg(status.show).arg(status.name));
		emit statusItemAdded(statusId);
	}
	else if (statusId > 0)
	{
		updateStatusItem(statusId,AName,AShow,AText,APriority);
	}
	return statusId;
}

void StatusChanger::updateStatusItem(int AStatusId, const QString &AName, int AShow, const QString &AText, int APriority)
{
	if (FStatusItems.contains(AStatusId) && !AName.isEmpty())
	{
		StatusItem &status = FStatusItems[AStatusId];
		if (status.name==AName || statusByName(AName)==STATUS_NULL_ID)
		{
			status.name = AName;
			status.show = AShow;
			status.text = AText;
			status.priority = APriority;
			updateStatusActions(AStatusId);

			LOG_DEBUG(QString("Status item updated, id=%1, show=%2, name=%3").arg(status.code).arg(status.show).arg(status.name));
			emit statusItemChanged(AStatusId);

			resendUpdatedStatus(AStatusId);
		}
	}
}

void StatusChanger::removeStatusItem(int AStatusId)
{
	if (AStatusId>STATUS_MAX_STANDART_ID && FStatusItems.contains(AStatusId) && !activeStatusItems().contains(AStatusId))
	{
		LOG_DEBUG(QString("Status item removed, id=%1").arg(AStatusId));
		emit statusItemRemoved(AStatusId);

		removeStatusActions(AStatusId);
		FStatusItems.remove(AStatusId);
	}
	else if (activeStatusItems().contains(AStatusId))
	{
		LOG_WARNING(QString("Failed to remove status item, id=%1: Item is active").arg(AStatusId));
	}
	else if (FStatusItems.contains(AStatusId))
	{
		REPORT_ERROR("Failed to remove status item: Item is not user defined");
	}
}

QIcon StatusChanger::iconByShow(int AShow) const
{
	return FStatusIcons != NULL ? FStatusIcons->iconByStatus(AShow,QString::null,false) : QIcon();
}

QString StatusChanger::nameByShow(int AShow) const
{
	switch (AShow)
	{
	case IPresence::Offline:
		return tr("Offline");
	case IPresence::Online:
		return tr("Online");
	case IPresence::Chat:
		return tr("Chat");
	case IPresence::Away:
		return tr("Away");
	case IPresence::ExtendedAway:
		return tr("Extended Away");
	case IPresence::DoNotDisturb:
		return tr("Do not disturb");
	case IPresence::Invisible:
		return tr("Invisible");
	case IPresence::Error:
		return tr("Error");
	default:
		return tr("Unknown Status");
	}
}

void StatusChanger::createDefaultStatus()
{
	StatusItem status;
	status.code = STATUS_ONLINE;
	status.name = nameByShow(IPresence::Online);
	status.show = IPresence::Online;
	status.text = tr("Online");
	status.priority = 30;
	FStatusItems.insert(status.code,status);
	createStatusActions(status.code);

	status.code = STATUS_CHAT;
	status.name = nameByShow(IPresence::Chat);
	status.show = IPresence::Chat;
	status.text = tr("Free for chat");
	status.priority = 25;
	FStatusItems.insert(status.code,status);
	createStatusActions(status.code);

	status.code = STATUS_AWAY;
	status.name = nameByShow(IPresence::Away);
	status.show = IPresence::Away;
	status.text = tr("I'm away from my desk");
	status.priority = 20;
	FStatusItems.insert(status.code,status);
	createStatusActions(status.code);

	status.code = STATUS_DND;
	status.name = nameByShow(IPresence::DoNotDisturb);
	status.show = IPresence::DoNotDisturb;
	status.text = tr("Do not disturb");
	status.priority = 15;
	FStatusItems.insert(status.code,status);
	createStatusActions(status.code);

	status.code = STATUS_EXAWAY;
	status.name = nameByShow(IPresence::ExtendedAway);
	status.show = IPresence::ExtendedAway;
	status.text = tr("Not available");
	status.priority = 10;
	FStatusItems.insert(status.code,status);
	createStatusActions(status.code);

	status.code = STATUS_INVISIBLE;
	status.name = nameByShow(IPresence::Invisible);
	status.show = IPresence::Invisible;
	status.text = tr("Disconnected");
	status.priority = 5;
	FStatusItems.insert(status.code,status);
	createStatusActions(status.code);

	status.code = STATUS_OFFLINE;
	status.name = nameByShow(IPresence::Offline);
	status.show = IPresence::Offline;
	status.text = tr("Disconnected");
	status.priority = 0;
	FStatusItems.insert(status.code,status);
	createStatusActions(status.code);

	status.code = STATUS_ERROR_ID;
	status.name = nameByShow(IPresence::Error);
	status.show = IPresence::Error;
	status.text.clear();
	status.priority = 0;
	FStatusItems.insert(status.code,status);

	status.code = STATUS_CONNECTING_ID;
	status.name = tr("Connecting...");
	status.show = IPresence::Offline;
	status.text.clear();
	status.priority = 0;
	FStatusItems.insert(status.code,status);
}

void StatusChanger::setMainStatusId(int AStatusId)
{
	if (FStatusItems.contains(AStatusId))
	{
		LOG_DEBUG(QString("Main status changed to=%1").arg(AStatusId));
		FStatusItems[STATUS_MAIN_ID] = FStatusItems.value(AStatusId);
		updateMainStatusActions();
	}
}

void StatusChanger::setStreamStatusId(IPresence *APresence, int AStatusId)
{
	if (FStatusItems.contains(AStatusId))
	{
		LOG_STRM_DEBUG(APresence->streamJid(),QString("Stream status changed to=%1").arg(AStatusId));

		FCurrentStatus[APresence] = AStatusId;
		if (AStatusId > MAX_TEMP_STATUS_ID)
			removeTempStatus(APresence);
		updateTrayToolTip();

		if (APresence->show() == IPresence::Error)
		{
			if (!FNotifyId.contains(APresence))
				insertStatusNotification(APresence);
			FFastReconnect -= APresence;
		}
		else
		{
			removeStatusNotification(APresence);
		}

		emit statusChanged(APresence->streamJid(), AStatusId);
	}
}

Action *StatusChanger::createStatusAction(int AStatusId, const Jid &AStreamJid, QObject *AParent) const
{
	Action *action = new Action(AParent);
	if (AStreamJid.isValid())
		action->setData(ADR_STREAMJID,AStreamJid.full());
	action->setData(ADR_STATUS_CODE,AStatusId);
	connect(action,SIGNAL(triggered(bool)),SLOT(onSetStatusByAction(bool)));
	updateStatusAction(AStatusId,action);
	return action;
}

void StatusChanger::updateStatusAction(int AStatusId, Action *AAction) const
{
	StatusItem status = FStatusItems.value(AStatusId);
	AAction->setText(status.name);
	AAction->setIcon(iconByShow(status.show));

	int sortShow = status.show != IPresence::Offline ? status.show : 100;
	AAction->setData(Action::DR_SortString,QString("%1-%2").arg(sortShow,5,10,QChar('0')).arg(status.name));
}

void StatusChanger::createStatusActions(int AStatusId)
{
	int group = AStatusId > STATUS_MAX_STANDART_ID ? AG_SCSM_STATUSCHANGER_CUSTOM_STATUS : AG_SCSM_STATUSCHANGER_DEFAULT_STATUS;

	FMainMenu->addAction(createStatusAction(AStatusId,Jid::null,FMainMenu),group,true);
	for (QMap<IPresence *, Menu *>::const_iterator it = FStreamMenu.constBegin(); it!=FStreamMenu.constEnd(); ++it)
		it.value()->addAction(createStatusAction(AStatusId,it.key()->streamJid(),it.value()),group,true);
}

void StatusChanger::updateStatusActions(int AStatusId)
{
	QMultiHash<int, QVariant> data;
	data.insert(ADR_STATUS_CODE,AStatusId);
	QList<Action *> actionList = FMainMenu->findActions(data,true);
	foreach (Action *action, actionList) {
		updateStatusAction(AStatusId,action); }
}

void StatusChanger::removeStatusActions(int AStatusId)
{
	QMultiHash<int, QVariant> data;
	data.insert(ADR_STATUS_CODE,AStatusId);
	qDeleteAll(FMainMenu->findActions(data,true));
}

void StatusChanger::createStreamMenu(IPresence *APresence)
{
	if (!FStreamMenu.contains(APresence))
	{
		Jid streamJid = APresence->streamJid();
		IAccount *account = FAccountManager!=NULL ? FAccountManager->accountByStream(streamJid) : NULL;

		Menu *sMenu = new Menu(FMainMenu);
		if (account)
			sMenu->setTitle(account->name());
		else
			sMenu->setTitle(APresence->streamJid().uFull());
		FStreamMenu.insert(APresence,sMenu);

		QMap<int, StatusItem>::const_iterator it = FStatusItems.constBegin();
		while (it != FStatusItems.constEnd())
		{
			if (it.key() > STATUS_MAX_STANDART_ID)
				sMenu->addAction(createStatusAction(it.key(),streamJid,sMenu),AG_SCSM_STATUSCHANGER_CUSTOM_STATUS,true);
			else if (it.key() > STATUS_NULL_ID)
				sMenu->addAction(createStatusAction(it.key(),streamJid,sMenu),AG_SCSM_STATUSCHANGER_DEFAULT_STATUS,true);
			++it;
		}

		Action *action = createStatusAction(STATUS_MAIN_ID, streamJid, sMenu);
		action->setData(ADR_STATUS_CODE, STATUS_MAIN_ID);
		sMenu->addAction(action,AG_SCSM_STATUSCHANGER_STREAMS,true);
		FMainStatusActions.insert(APresence,action);

		FMainMenu->addAction(sMenu->menuAction(),AG_SCSM_STATUSCHANGER_STREAMS,true);
	}
}

void StatusChanger::updateStreamMenu(IPresence *APresence)
{
	int statusId = FCurrentStatus.value(APresence,STATUS_MAIN_ID);

	Menu *sMenu = FStreamMenu.value(APresence);
	if (sMenu)
		sMenu->setIcon(iconByShow(statusItemShow(statusId)));

	Action *mAction = FMainStatusActions.value(APresence);
	if (mAction)
		mAction->setVisible(FCurrentStatus.value(APresence) != STATUS_MAIN_ID);
}

IPresence *StatusChanger::visibleMainStatusPresence() const
{
	IPresence *presence = NULL;
	int statusId = STATUS_OFFLINE;

	bool isOnline = false;
	QMap<IPresence *, int>::const_iterator it = FCurrentStatus.constBegin();
	while (it!=FCurrentStatus.constEnd() && (!isOnline || statusId!=STATUS_MAIN_ID))
	{
		if (it.key()->xmppStream()->isOpen())
		{
			isOnline = true;
			presence = it.key();
			statusId = it.value();
		}
		else if (!isOnline && it.value()==STATUS_CONNECTING_ID)
		{
			isOnline = true;
			presence = it.key();
			statusId = STATUS_CONNECTING_ID;
		}
		else if (!isOnline && statusId!=STATUS_MAIN_ID)
		{
			presence = it.key();
			statusId = it.value();
		}
		++it;
	}

	return presence;
}

void StatusChanger::updateMainMenu()
{
	int statusId = FCurrentStatus.value(visibleMainStatusPresence(),STATUS_OFFLINE);

	if (statusId == STATUS_CONNECTING_ID)
		FMainMenu->setIcon(RSR_STORAGE_MENUICONS, MNI_SCHANGER_CONNECTING);
	else
		FMainMenu->setIcon(iconByShow(statusItemShow(statusId)));
	FMainMenu->setTitle(statusItemName(statusId));
	FMainMenu->menuAction()->setEnabled(!FCurrentStatus.isEmpty());

	if (FTrayManager)
	{
		//if (statusId == STATUS_CONNECTING_ID)
		//	FTrayManager->setIcon(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_SCHANGER_CONNECTING));
		//else
			FTrayManager->setIcon(iconByShow(statusItemShow(statusId)));
	}

	if (FRostersModel)
	{
		IRosterIndex *croot = FRostersModel->contactsRoot();
		croot->setData(statusItemShow(statusId),RDR_SHOW);
		croot->setData(statusItemText(statusId),RDR_STATUS);
		croot->setData(statusItemPriority(statusId),RDR_PRIORITY);
	}
}

void StatusChanger::updateTrayToolTip()
{
	if (FTrayManager)
	{
		QString trayToolTip;
		QMap<IPresence *, int>::const_iterator it = FCurrentStatus.constBegin();
		while (it != FCurrentStatus.constEnd())
		{
			IPresence *presence = it.key();
			IAccount *account = FAccountManager->accountByStream(presence->streamJid());
			if (!trayToolTip.isEmpty())
				trayToolTip+="\n";
			trayToolTip += tr("%1 - %2").arg(account->name()).arg(statusItemName(it.value()));
			++it;
		}
		FTrayManager->setToolTip(trayToolTip);
	}
}

void StatusChanger::updateMainStatusActions()
{
	QIcon icon = iconByShow(statusItemShow(STATUS_MAIN_ID));
	QString name = statusItemName(STATUS_MAIN_ID);
	foreach (Action *action, FMainStatusActions)
	{
		action->setIcon(icon);
		action->setText(name);
	}
}

void StatusChanger::insertConnectingLabel(IPresence *APresence)
{
	if (FRostersModel && FRostersView)
	{
		IRosterIndex *sindex = FRostersModel->streamIndex(APresence->xmppStream()->streamJid());
		if (sindex)
			FRostersView->insertLabel(FConnectingLabelId,sindex);

		IRosterIndex *croot = FRostersModel->contactsRoot();
		if (croot && FConnectStatus.isEmpty())
			FRostersView->insertLabel(FConnectingLabelId,croot);
	}
}

void StatusChanger::removeConnectingLabel(IPresence *APresence)
{
	if (FRostersModel && FRostersView)
	{
		IRosterIndex *sindex = FRostersModel->streamIndex(APresence->xmppStream()->streamJid());
		if (sindex)
			FRostersView->removeLabel(FConnectingLabelId,sindex);

		IRosterIndex *croot = FRostersModel->contactsRoot();
		if (croot && FConnectStatus.isEmpty())
			FRostersView->removeLabel(FConnectingLabelId,croot);
	}
}

void StatusChanger::autoReconnect(IPresence *APresence)
{
	IAccount *account = FAccountManager!=NULL ? FAccountManager->accountByStream(APresence->streamJid()) : NULL;
	if (account && account->optionsNode().value("auto-reconnect").toBool())
	{
		int statusId = FLastOnlineStatus.value(APresence, STATUS_MAIN_ID);
		int statusShow = statusItemShow(statusId);
		if (statusShow!=IPresence::Offline && statusShow!=IPresence::Error)
		{
			int reconSecs = FFastReconnect.contains(APresence) ? 1 : 30;
			FPendingReconnect.insert(APresence,QPair<QDateTime,int>(QDateTime::currentDateTime().addSecs(reconSecs),statusId));
			QTimer::singleShot(reconSecs*1000+100,this,SLOT(onReconnectTimer()));
		}
	}
}

int StatusChanger::createTempStatus(IPresence *APresence, int AShow, const QString &AText, int APriority)
{
	removeTempStatus(APresence);

	StatusItem status;
	status.name = nameByShow(AShow).append('*');
	status.show = AShow;
	status.text = AText;
	status.priority = APriority;
	status.code = MAX_TEMP_STATUS_ID;
	while (FStatusItems.contains(status.code))
		status.code--;
	FStatusItems.insert(status.code,status);
	FTempStatus.insert(APresence,status.code);
	return status.code;
}

void StatusChanger::removeTempStatus(IPresence *APresence)
{
	if (FTempStatus.contains(APresence))
		if (!activeStatusItems().contains(FTempStatus.value(APresence)))
			FStatusItems.remove(FTempStatus.take(APresence));
}

void StatusChanger::resendUpdatedStatus(int AStatusId)
{
	if (FStatusItems[STATUS_MAIN_ID].code == AStatusId)
		setMainStatus(AStatusId);

	for (QMap<IPresence *, int>::const_iterator it = FCurrentStatus.constBegin(); it != FCurrentStatus.constEnd(); ++it)
		if (it.value() == AStatusId)
			setStreamStatus(it.key()->streamJid(), AStatusId);
}

void StatusChanger::removeAllCustomStatuses()
{
	foreach (int statusId, FStatusItems.keys())
		if (statusId > STATUS_MAX_STANDART_ID)
			removeStatusItem(statusId);
}

void StatusChanger::insertStatusNotification(IPresence *APresence)
{
	removeStatusNotification(APresence);
	if (FNotifications)
	{
		INotification notify;
		notify.kinds = FNotifications->enabledTypeNotificationKinds(NNT_CONNECTION_ERROR);
		if (notify.kinds > 0)
		{
			notify.typeId = NNT_CONNECTION_ERROR;
			notify.data.insert(NDR_ICON,FStatusIcons!=NULL ? FStatusIcons->iconByStatus(IPresence::Error,QString::null,false) : QIcon());
			notify.data.insert(NDR_POPUP_CAPTION, tr("Connection error"));
			notify.data.insert(NDR_POPUP_TITLE,FAccountManager!=NULL ? FAccountManager->accountByStream(APresence->streamJid())->name() : APresence->streamJid().uFull());
			notify.data.insert(NDR_STREAM_JID,APresence->streamJid().full());
			notify.data.insert(NDR_CONTACT_JID,APresence->streamJid().full());
			notify.data.insert(NDR_POPUP_IMAGE, FNotifications->contactAvatar(APresence->streamJid()));
			notify.data.insert(NDR_POPUP_TEXT,APresence->status());
			notify.data.insert(NDR_SOUND_FILE,SDF_SCHANGER_CONNECTION_ERROR);
			FNotifyId.insert(APresence,FNotifications->appendNotification(notify));
		}
	}
}

void StatusChanger::removeStatusNotification(IPresence *APresence)
{
	if (FNotifications && FNotifyId.contains(APresence))
		FNotifications->removeNotification(FNotifyId.take(APresence));
}

void StatusChanger::onSetStatusByAction(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QString streamJid = action->data(ADR_STREAMJID).toString();
		int statusId = action->data(ADR_STATUS_CODE).toInt();
		if (Options::node(OPV_STATUSES_MODIFY).value().toBool())
		{
			delete FModifyStatusDialog;
			FModifyStatusDialog = new ModifyStatusDialog(this,statusId,streamJid,NULL);
			FModifyStatusDialog->show();
		}
		else
		{
			setStreamStatus(streamJid, statusId);
		}
	}
}

void StatusChanger::onPresenceAdded(IPresence *APresence)
{
	if (FStreamMenu.count() == 1)
		FStreamMenu.value(FStreamMenu.keys().first())->menuAction()->setVisible(true);

	createStreamMenu(APresence);
	setStreamStatusId(APresence,STATUS_OFFLINE);

	if (FStreamMenu.count() == 1)
		FStreamMenu.value(FStreamMenu.keys().first())->menuAction()->setVisible(false);

	IAccount *account = FAccountManager!=NULL ? FAccountManager->accountByStream(APresence->streamJid()) : NULL;
	if (account)
	{
		if (account->optionsNode().value("status.is-main").toBool())
			FMainStatusStreams += APresence;
		FLastOnlineStatus.insert(APresence, account->optionsNode().value("status.last-online").toInt());
	}

	updateStreamMenu(APresence);
	updateMainMenu();
}

void StatusChanger::onPresenceChanged(IPresence *APresence, int AShow, const QString &AText, int APriority)
{
	if (FCurrentStatus.contains(APresence))
	{
		if (AShow == IPresence::Error)
		{
			autoReconnect(APresence);
			setStreamStatusId(APresence, STATUS_ERROR_ID);
			updateStreamMenu(APresence);
			updateMainMenu();
		}
		else if (FChangingPresence != APresence)
		{
			StatusItem status = FStatusItems.value(FCurrentStatus.value(APresence));
			if (status.name.isEmpty() || status.show!=AShow || status.priority!=APriority || status.text!=AText)
			{
				setStreamStatusId(APresence, createTempStatus(APresence,AShow,AText,APriority));
				updateStreamMenu(APresence);
				updateMainMenu();
			}
		}
		if (FConnectStatus.contains(APresence))
		{
			FConnectStatus.remove(APresence);
			removeConnectingLabel(APresence);
		}

		if (AShow!= IPresence::Offline && AShow!=IPresence::Error)
			FFastReconnect += APresence;
		else
			FFastReconnect -= APresence;
	}
}

void StatusChanger::onPresenceRemoved(IPresence *APresence)
{
	IAccount *account = FAccountManager!=NULL ? FAccountManager->accountByStream(APresence->streamJid()) : NULL;
	if (account)
	{
		bool isMainStatus = FMainStatusStreams.contains(APresence);
		account->optionsNode().setValue(isMainStatus,"status.is-main");
		account->optionsNode().setValue(FLastOnlineStatus.value(APresence, STATUS_MAIN_ID),"status.last-online");
	}

	removeStatusNotification(APresence);
	removeTempStatus(APresence);

	FConnectStatus.remove(APresence);
	removeConnectingLabel(APresence);

	FFastReconnect -= APresence;
	FMainStatusStreams -= APresence;
	FMainStatusActions.remove(APresence);
	FCurrentStatus.remove(APresence);
	FLastOnlineStatus.remove(APresence);
	FPendingReconnect.remove(APresence);
	delete FStreamMenu.take(APresence);

	if (FStreamMenu.count() == 1)
		FStreamMenu.value(FStreamMenu.keys().first())->menuAction()->setVisible(false);

	updateMainMenu();
	updateTrayToolTip();
}

void StatusChanger::onRosterOpened(IRoster *ARoster)
{
	IPresence *presence = FPresencePlugin->findPresence(ARoster->streamJid());
	if (FConnectStatus.contains(presence))
		setStreamStatus(presence->streamJid(), FConnectStatus.value(presence));
	FPluginManager->delayShutdown();
}

void StatusChanger::onRosterClosed(IRoster *ARoster)
{
	IPresence *presence = FPresencePlugin->findPresence(ARoster->streamJid());
	if (FConnectStatus.contains(presence))
		setStreamStatus(presence->streamJid(), FConnectStatus.value(presence));
	FPluginManager->continueShutdown();
}

void StatusChanger::onStreamJidChanged(const Jid &ABefore, const Jid &AAfter)
{
	QMultiHash<int,QVariant> data;
	data.insert(ADR_STREAMJID,ABefore.full());
	QList<Action *> actionList = FMainMenu->findActions(data,true);
	foreach (Action *action, actionList)
		action->setData(ADR_STREAMJID,AAfter.full());
}

void StatusChanger::onRostersViewIndexContextMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu)
{
	if (ALabelId==AdvancedDelegateItem::DisplayId && AIndexes.count()==1)
	{
		IRosterIndex *index = AIndexes.first();
		if (index->kind() == RIK_STREAM_ROOT)
		{
			Menu *menu = streamMenu(index->data(RDR_STREAM_JID).toString());
			if (menu)
			{
				Action *action = new Action(AMenu);
				action->setMenu(menu);
				action->setText(tr("Status"));
				action->setIcon(menu->menuAction()->icon());
				AMenu->addAction(action,AG_RVCM_STATUSCHANGER,true);
			}
		}
		else if (index->kind() == RIK_CONTACTS_ROOT)
		{
			if (index->data(RDR_STREAMS).toStringList().count()>1)
			{
				Menu *menu = new Menu(AMenu);
				menu->setTitle(tr("Status"));
				menu->setIcon(FMainMenu->icon());
				foreach(Action *action, FMainMenu->groupActions(AG_SCSM_STATUSCHANGER_CUSTOM_STATUS))
					menu->addAction(action,AG_SCSM_STATUSCHANGER_CUSTOM_STATUS,true);
				foreach(Action *action, FMainMenu->groupActions(AG_SCSM_STATUSCHANGER_DEFAULT_STATUS))
					menu->addAction(action,AG_SCSM_STATUSCHANGER_DEFAULT_STATUS,true);
				AMenu->addAction(menu->menuAction(),AG_RVCM_STATUSCHANGER,true);
			}
		}
	}
}

void StatusChanger::onDefaultStatusIconsChanged()
{
	foreach (const StatusItem &status, FStatusItems)
		updateStatusActions(status.code);

	foreach (IPresence *presence, FStreamMenu.keys())
		updateStreamMenu(presence);

	updateMainStatusActions();
	updateMainMenu();
}

void StatusChanger::onOptionsOpened()
{
	removeAllCustomStatuses();

	foreach (const QString &ns, Options::node(OPV_STATUSES_ROOT).childNSpaces("status"))
	{
		int statusId = ns.toInt();
		OptionsNode soptions = Options::node(OPV_STATUS_ITEM, ns);
		if (statusId > STATUS_MAX_STANDART_ID)
		{
			QString statusName = soptions.value("name").toString();
			if (!statusName.isEmpty() && statusByName(statusName)==STATUS_NULL_ID)
			{
				StatusItem status;
				status.code = statusId;
				status.name = statusName;
				status.show = (IPresence::Show)soptions.value("show").toInt();
				status.text = soptions.value("text").toString();
				status.priority = soptions.value("priority").toInt();
				FStatusItems.insert(status.code,status);
				createStatusActions(status.code);
			}
		}
		else if (statusId > STATUS_NULL_ID && FStatusItems.contains(statusId))
		{
			StatusItem &status = FStatusItems[statusId];
			status.text = soptions.hasValue("text") ? soptions.value("text").toString() : status.text;
			status.priority = soptions.hasValue("priority") ? soptions.value("priority").toInt() : status.priority;
			updateStatusActions(statusId);
		}
	}

	FModifyStatus->setChecked(Options::node(OPV_STATUSES_MODIFY).value().toBool());
	setMainStatusId(Options::node(OPV_STATUSES_MAINSTATUS).value().toInt());
}

void StatusChanger::onOptionsClosed()
{
	delete FEditStatusDialog;
	delete FModifyStatusDialog;

	QList<QString> oldNS = Options::node(OPV_STATUSES_ROOT).childNSpaces("status");
	foreach (const StatusItem &status, FStatusItems)
	{
		if (status.code > STATUS_NULL_ID)
		{
			OptionsNode soptions = Options::node(OPV_STATUS_ITEM, QString::number(status.code));
			if (status.code > STATUS_MAX_STANDART_ID)
				soptions.setValue(status.show,"show");
			soptions.setValue(status.name,"name");
			soptions.setValue(status.text,"text");
			soptions.setValue(status.priority,"priority");
		}
		oldNS.removeAll(QString::number(status.code));
	}

	foreach(const QString &ns, oldNS)
		Options::node(OPV_STATUSES_ROOT).removeChilds("status",ns);

	Options::node(OPV_STATUSES_MAINSTATUS).setValue(FStatusItems.value(STATUS_MAIN_ID).code);

	setMainStatusId(STATUS_OFFLINE);
	removeAllCustomStatuses();
}

void StatusChanger::onOptionsChanged(const OptionsNode &ANode)
{
	if (ANode.path() == OPV_STATUSES_MODIFY)
	{
		FModifyStatus->setChecked(ANode.value().toBool());
	}
}

void StatusChanger::onProfileOpened(const QString &AProfile)
{
	Q_UNUSED(AProfile);
	foreach(IPresence *presence, FCurrentStatus.keys())
	{
		IAccount *account = FAccountManager!=NULL ? FAccountManager->accountByStream(presence->streamJid()) : NULL;
		if (account!=NULL && account->optionsNode().value("auto-connect").toBool())
		{
			int statusId = !FMainStatusStreams.contains(presence) ? FLastOnlineStatus.value(presence, STATUS_MAIN_ID) : STATUS_MAIN_ID;
			if (!FStatusItems.contains(statusId))
				statusId = STATUS_MAIN_ID;
			LOG_STRM_INFO(presence->streamJid(),"Automatically connecting stream on startup");
			setStreamStatus(presence->streamJid(),statusId);
		}
	}
}

void StatusChanger::onApplicationShutdownStarted()
{
	foreach(IPresence *presence, FCurrentStatus.keys())
	{
		if (presence->isOpen())
		{
			LOG_STRM_INFO(presence->streamJid(),QString("Closing XMPP stream due to application shutdown"));
			presence->setPresence(IPresence::Offline,tr("Left Vacuum-IM"),0);
			presence->xmppStream()->close();
		}
	}
}

void StatusChanger::onReconnectTimer()
{
	QMap<IPresence *,QPair<QDateTime,int> >::iterator it = FPendingReconnect.begin();
	while (it != FPendingReconnect.end())
	{
		if (it.value().first <= QDateTime::currentDateTime())
		{
			IPresence *presence = it.key();
			int statusId = FStatusItems.contains(it.value().second) ? it.value().second : STATUS_MAIN_ID;
			it = FPendingReconnect.erase(it);
			if (presence->show() == IPresence::Error)
			{
				LOG_STRM_INFO(presence->streamJid(),"Automatically reconnecting stream on error");
				setStreamStatus(presence->streamJid(),statusId);
			}
		}
		else
		{
			++it;
		}
	}
}

void StatusChanger::onEditStatusAction(bool)
{
	if (FEditStatusDialog.isNull())
	{
		FEditStatusDialog = new EditStatusDialog(this);
		FEditStatusDialog->show();
	}
	else
	{
		FEditStatusDialog->show();
	}
}

void StatusChanger::onModifyStatusAction(bool)
{
	Options::node(OPV_STATUSES_MODIFY).setValue(FModifyStatus->isChecked());
}

void StatusChanger::onAccountOptionsChanged(IAccount *AAccount, const OptionsNode &ANode)
{
	if (AAccount->optionsNode().childPath(ANode)=="name")
	{
		Menu *sMenu = streamMenu(AAccount->streamJid());
		if (sMenu)
			sMenu->setTitle(ANode.value().toString());
	}
}

void StatusChanger::onNotificationActivated(int ANotifyId)
{
	if (FNotifyId.values().contains(ANotifyId))
		FNotifications->removeNotification(ANotifyId);
}

Q_EXPORT_PLUGIN2(plg_statuschanger, StatusChanger)
