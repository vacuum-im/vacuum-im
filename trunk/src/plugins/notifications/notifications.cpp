#include "notifications.h"

#include <QProcess>
#include <QVBoxLayout>

#define ADR_NOTIFYID       Action::DR_Parametr1

Notifications::Notifications()
{
	FAvatars = NULL;
	FRosterPlugin = NULL;
	FStatusIcons = NULL;
	FStatusChanger = NULL;
	FTrayManager = NULL;
	FRostersModel = NULL;
	FRostersViewPlugin = NULL;
	FOptionsManager = NULL;
	FMainWindowPlugin = NULL;

	FActivateAll = NULL;
	FRemoveAll = NULL;
	FNotifyMenu = NULL;

	FNotifyId = 0;
	FSound = NULL;
}

Notifications::~Notifications()
{
	delete FActivateAll;
	delete FRemoveAll;
	delete FNotifyMenu;
	delete FSound;
}

void Notifications::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Notifications Manager");
	APluginInfo->description = tr("Allows other modules to notify the user of the events");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
}

bool Notifications::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
	IPlugin *plugin = APluginManager->pluginInterface("ITrayManager").value(0,NULL);
	if (plugin)
	{
		FTrayManager = qobject_cast<ITrayManager *>(plugin->instance());
		if (FTrayManager)
		{
			connect(FTrayManager->instance(),SIGNAL(notifyActivated(int, QSystemTrayIcon::ActivationReason)),
				SLOT(onTrayNotifyActivated(int, QSystemTrayIcon::ActivationReason)));
			connect(FTrayManager->instance(),SIGNAL(notifyRemoved(int)),SLOT(onTrayNotifyRemoved(int)));
		}
	}

	plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
	if (plugin)
	{
		FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
		if (FRostersViewPlugin)
		{
			connect(FRostersViewPlugin->rostersView()->instance(),SIGNAL(notifyActivated(IRosterIndex *, int)),
				SLOT(onRosterNotifyActivated(IRosterIndex *, int)));
			connect(FRostersViewPlugin->rostersView()->instance(),SIGNAL(notifyRemovedByIndex(IRosterIndex *, int)),
				SLOT(onRosterNotifyRemoved(IRosterIndex *, int)));
		}
	}

	plugin = APluginManager->pluginInterface("IRostersModel").value(0,NULL);
	if (plugin)
		FRostersModel = qobject_cast<IRostersModel *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IAvatars").value(0,NULL);
	if (plugin)
		FAvatars = qobject_cast<IAvatars *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IRosterPlugin").value(0,NULL);
	if (plugin)
		FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IStatusIcons").value(0,NULL);
	if (plugin)
		FStatusIcons = qobject_cast<IStatusIcons *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IStatusChanger").value(0,NULL);
	if (plugin)
		FStatusChanger = qobject_cast<IStatusChanger *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IMainWindowPlugin").value(0,NULL);
	if (plugin)
		FMainWindowPlugin = qobject_cast<IMainWindowPlugin *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IOptionsManager").value(0,NULL);
	if (plugin)
		FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());

	connect(Options::instance(),SIGNAL(optionsOpened()),SLOT(onOptionsOpened()));
	connect(Options::instance(),SIGNAL(optionsChanged(const OptionsNode &)),SLOT(onOptionsChanged(const OptionsNode &)));

	return true;
}

bool Notifications::initObjects()
{
	Shortcuts::declareShortcut(SCT_APP_TOGGLESOUND, tr("Enable/Disable notifications sound"), QKeySequence::UnknownKey, Shortcuts::ApplicationShortcut);
	Shortcuts::declareShortcut(SCT_APP_ACTIVATENOTIFICATIONS, tr("Activate all notifications"), QKeySequence::UnknownKey, Shortcuts::ApplicationShortcut);
	Shortcuts::declareShortcut(SCT_APP_REMOVENOTIFICATIONS, tr("Remove all notifications"), QKeySequence::UnknownKey, Shortcuts::ApplicationShortcut);

	FSoundOnOff = new Action(this);
	FSoundOnOff->setToolTip(tr("Enable/Disable notifications sound"));
	FSoundOnOff->setIcon(RSR_STORAGE_MENUICONS, MNI_NOTIFICATIONS_SOUND_ON);
	FSoundOnOff->setShortcutId(SCT_APP_TOGGLESOUND);
	connect(FSoundOnOff,SIGNAL(triggered(bool)),SLOT(onSoundOnOffActionTriggered(bool)));

	FActivateAll = new Action(this);
	FActivateAll->setVisible(false);
	FActivateAll->setText(tr("Activate All Notifications"));
	FActivateAll->setIcon(RSR_STORAGE_MENUICONS,MNI_NOTIFICATIONS_ACTIVATE_ALL);
	FActivateAll->setShortcutId(SCT_APP_ACTIVATENOTIFICATIONS);
	connect(FActivateAll,SIGNAL(triggered(bool)),SLOT(onTrayActionTriggered(bool)));

	FRemoveAll = new Action(this);
	FRemoveAll->setVisible(false);
	FRemoveAll->setText(tr("Remove All Notifications"));
	FRemoveAll->setIcon(RSR_STORAGE_MENUICONS,MNI_NOTIFICATIONS_REMOVE_ALL);
	FRemoveAll->setShortcutId(SCT_APP_REMOVENOTIFICATIONS);
	connect(FRemoveAll,SIGNAL(triggered(bool)),SLOT(onTrayActionTriggered(bool)));

	FNotifyMenu = new Menu;
	FNotifyMenu->setTitle(tr("Pending Notifications"));
	FNotifyMenu->setIcon(RSR_STORAGE_MENUICONS,MNI_NOTIFICATIONS);
	FNotifyMenu->menuAction()->setVisible(false);

	if (FTrayManager)
	{
		FTrayManager->contextMenu()->addAction(FActivateAll,AG_TMTM_NOTIFICATIONS,false);
		FTrayManager->contextMenu()->addAction(FRemoveAll,AG_TMTM_NOTIFICATIONS,false);
		FTrayManager->contextMenu()->addAction(FNotifyMenu->menuAction(),AG_TMTM_NOTIFICATIONS,false);
	}

	if (FMainWindowPlugin)
	{
		FMainWindowPlugin->mainWindow()->topToolBarChanger()->insertAction(FSoundOnOff,TBG_MWTTB_NOTIFICATIONS_SOUND);
	}

	return true;
}

bool Notifications::initSettings()
{
	Options::setDefaultValue(OPV_NOTIFICATIONS_ENABLESOUND,true);
	Options::setDefaultValue(OPV_NOTIFICATIONS_ROSTERNOTIFY,true);
	Options::setDefaultValue(OPV_NOTIFICATIONS_POPUPWINDOW,true);
	Options::setDefaultValue(OPV_NOTIFICATIONS_TRAYNOTIFY,true);
	Options::setDefaultValue(OPV_NOTIFICATIONS_TRAYACTION,true);
	Options::setDefaultValue(OPV_NOTIFICATIONS_ALERTWIDGET,true);
	Options::setDefaultValue(OPV_NOTIFICATIONS_TABPAGENOTIFY,true);
	Options::setDefaultValue(OPV_NOTIFICATIONS_AUTOACTIVATE,true);
	Options::setDefaultValue(OPV_NOTIFICATIONS_ENABLEALERTS,true);
	Options::setDefaultValue(OPV_NOTIFICATIONS_EXPANDGROUP,true);
	Options::setDefaultValue(OPV_NOTIFICATIONS_NOSOUNDIFDND,false);
	Options::setDefaultValue(OPV_NOTIFICATIONS_POPUPTIMEOUT,8);
	Options::setDefaultValue(OPV_NOTIFICATIONS_TABPAGE_SHOWMINIMIZED,true);
	Options::setDefaultValue(OPV_NOTIFICATIONS_SOUNDCOMMAND,QString("aplay"));

	if (FOptionsManager)
	{
		IOptionsDialogNode dnode = { ONO_NOTIFICATIONS, OPN_NOTIFICATIONS, tr("Notifications"), MNI_NOTIFICATIONS };
		FOptionsManager->insertOptionsDialogNode(dnode);
		FOptionsManager->insertOptionsHolder(this);
	}
	return true;
}

QMultiMap<int, IOptionsWidget *> Notifications::optionsWidgets(const QString &ANodeId, QWidget *AParent)
{
	QMultiMap<int, IOptionsWidget *> widgets;
	if (FOptionsManager && ANodeId == OPN_NOTIFICATIONS)
	{
		widgets.insertMulti(OWO_NOTIFICATIONS_EXTENDED,FOptionsManager->optionsNodeWidget(Options::node(OPV_NOTIFICATIONS_ENABLEALERTS),tr("Enable alerts in task bar"),AParent));
		widgets.insertMulti(OWO_NOTIFICATIONS_EXTENDED,FOptionsManager->optionsNodeWidget(Options::node(OPV_NOTIFICATIONS_EXPANDGROUP),tr("Expand contact groups in roster"),AParent));
		widgets.insertMulti(OWO_NOTIFICATIONS_EXTENDED,FOptionsManager->optionsNodeWidget(Options::node(OPV_NOTIFICATIONS_NOSOUNDIFDND),tr("Disable sounds when status is 'Do not disturb'"),AParent));
		widgets.insertMulti(OWO_NOTIFICATIONS_EXTENDED,FOptionsManager->optionsNodeWidget(Options::node(OPV_NOTIFICATIONS_TABPAGE_SHOWMINIMIZED),tr("Show minimized window on notification"),AParent));
		widgets.insertMulti(OWO_NOTIFICATIONS_COMMON, new OptionsWidget(this, AParent));
		foreach(QString id, FTypeRecords.keys())
		{
			TypeRecord typeRecord = FTypeRecords.value(id);
			if (!typeRecord.title.isEmpty())
				widgets.insertMulti(typeRecord.optionsOrder, new NotifyKindsWidget(this,id,typeRecord.title,typeRecord.kindMask,AParent));
		}
	}
	return widgets;
}

QList<int> Notifications::notifications() const
{
	return FNotifyRecords.keys();
}

INotification Notifications::notificationById(int ANotifyId) const
{
	return FNotifyRecords.value(ANotifyId).notification;
}

int Notifications::appendNotification(const INotification &ANotification)
{
	NotifyRecord record;
	int notifyId = ++FNotifyId;
	record.notification = ANotification;
	emit notificationAppend(notifyId, record.notification);

	bool isDND = FStatusChanger!=NULL ? FStatusChanger->statusItemShow(STATUS_MAIN_ID)==IPresence::DoNotDisturb : false;

	QIcon icon = qvariant_cast<QIcon>(record.notification.data.value(NDR_ICON));
	QString toolTip = record.notification.data.value(NDR_TOOLTIP).toString();

	if (FRostersModel && FRostersViewPlugin && Options::node(OPV_NOTIFICATIONS_ROSTERNOTIFY).value().toBool() &&
		(record.notification.kinds & INotification::RosterNotify)>0)
	{
		if (!showNotifyByHandler(INotification::RosterNotify,notifyId,record.notification))
		{
			Jid streamJid = record.notification.data.value(NDR_STREAM_JID).toString();
			Jid contactJid = record.notification.data.value(NDR_CONTACT_JID).toString();
			int order = record.notification.data.value(NDR_ROSTER_ORDER).toInt();
			int flags = IRostersView::LabelBlink|IRostersView::LabelVisible;
			flags = flags | (Options::node(OPV_NOTIFICATIONS_EXPANDGROUP).value().toBool() ? IRostersView::LabelExpandParents : 0);
			QList<IRosterIndex *> indexes = FRostersModel->getContactIndexList(streamJid,contactJid,true);
			record.rosterId = FRostersViewPlugin->rostersView()->appendNotify(indexes,order,icon,toolTip,flags);
		}
	}

	if (Options::node(OPV_NOTIFICATIONS_POPUPWINDOW).value().toBool() && (record.notification.kinds & INotification::PopupWindow)>0)
	{
		if (!showNotifyByHandler(INotification::PopupWindow,notifyId,record.notification))
		{
			record.popupWidget = new NotifyWidget(record.notification);
			connect(record.popupWidget,SIGNAL(notifyActivated()),SLOT(onWindowNotifyActivated()));
			connect(record.popupWidget,SIGNAL(notifyRemoved()),SLOT(onWindowNotifyRemoved()));
			connect(record.popupWidget,SIGNAL(windowDestroyed()),SLOT(onWindowNotifyDestroyed()));
			record.popupWidget->appear();
		}
	}

	if (FTrayManager)
	{
		if (Options::node(OPV_NOTIFICATIONS_TRAYNOTIFY).value().toBool() && (record.notification.kinds & INotification::TrayNotify)>0)
		{
			if (!showNotifyByHandler(INotification::TrayNotify,notifyId,record.notification))
			{
				ITrayNotify notify;
				notify.blink = true;
				notify.icon = icon;
				notify.toolTip = toolTip;
				record.trayId = FTrayManager->appendNotify(notify);
			}
		}

		if (!toolTip.isEmpty() && Options::node(OPV_NOTIFICATIONS_TRAYACTION).value().toBool() &&
		    (record.notification.kinds & INotification::TrayAction)>0)
		{
			if (!showNotifyByHandler(INotification::TrayAction,notifyId,record.notification))
			{
				record.trayAction = new Action(FNotifyMenu);
				record.trayAction->setIcon(icon);
				record.trayAction->setText(toolTip);
				record.trayAction->setData(ADR_NOTIFYID,notifyId);
				connect(record.trayAction,SIGNAL(triggered(bool)),SLOT(onActionNotifyActivated(bool)));
				FNotifyMenu->addAction(record.trayAction);
			}
		}
	}

	if (!(isDND && Options::node(OPV_NOTIFICATIONS_NOSOUNDIFDND).value().toBool()) &&
		Options::node(OPV_NOTIFICATIONS_ENABLESOUND).value().toBool() && (record.notification.kinds & INotification::SoundPlay)>0)
	{
		if (!showNotifyByHandler(INotification::SoundPlay,notifyId,record.notification))
		{
			QString soundName = record.notification.data.value(NDR_SOUND_FILE).toString();
			QString soundFile = FileStorage::staticStorage(RSR_STORAGE_SOUNDS)->fileFullName(soundName);
			if (!soundFile.isEmpty())
			{
				if (QSound::isAvailable())
				{
					delete FSound;
					FSound = new QSound(soundFile);
					FSound->play();
				}
#ifdef Q_WS_X11
				else
				{
					QProcess::startDetached(Options::node(OPV_NOTIFICATIONS_SOUNDCOMMAND).value().toString(),QStringList()<<soundFile);
				}
#endif
			}
		}
	}

	if (Options::node(OPV_NOTIFICATIONS_ALERTWIDGET).value().toBool() && (record.notification.kinds & INotification::AlertWidget)>0)
	{
		if (!showNotifyByHandler(INotification::AlertWidget,notifyId,record.notification))
		{
			QWidget *widget = qobject_cast<QWidget *>((QWidget *)record.notification.data.value(NDR_ALERT_WIDGET).toInt());
			if (widget)
				WidgetManager::alertWidget(widget);
		}
	}

	if (Options::node(OPV_NOTIFICATIONS_TABPAGENOTIFY).value().toBool() && (record.notification.kinds & INotification::TabPageNotify)>0)
	{
		if (!showNotifyByHandler(INotification::TabPageNotify,notifyId,record.notification))
		{
			ITabPage *page = qobject_cast<ITabPage *>((QWidget *)record.notification.data.value(NDR_TABPAGE_OBJECT).toInt());
			if (page && page->tabPageNotifier())
			{
				ITabPageNotify notify;
				notify.icon = icon;
				notify.toolTip = toolTip;
				notify.priority = record.notification.data.value(NDR_TABPAGE_PRIORITY).toInt();
				notify.blink = record.notification.data.value(NDR_TABPAGE_ICONBLINK).toBool();
				record.tabPageId = page->tabPageNotifier()->insertNotify(notify);
				record.tabPageNotifier = page->tabPageNotifier()->instance();
			}
		}
	}
		
	if (Options::node(OPV_NOTIFICATIONS_AUTOACTIVATE).value().toBool() && (record.notification.kinds & INotification::AutoActivate)>0)
	{
		FDelayedActivations.append(notifyId);
		QTimer::singleShot(0,this,SLOT(onActivateDelayedActivations()));
	}

	if (FNotifyRecords.isEmpty())
	{
		FActivateAll->setVisible(true);
		FRemoveAll->setVisible(true);
	}
	FNotifyMenu->menuAction()->setVisible(!FNotifyMenu->isEmpty());

	FNotifyRecords.insert(notifyId,record);
	emit notificationAppended(notifyId, record.notification);

	return notifyId;
}

void Notifications::activateNotification(int ANotifyId)
{
	if (FNotifyRecords.contains(ANotifyId))
	{
		emit notificationActivated(ANotifyId);
	}
}

void Notifications::removeNotification(int ANotifyId)
{
	if (FNotifyRecords.contains(ANotifyId))
	{
		NotifyRecord record = FNotifyRecords.take(ANotifyId);
		if (FRostersViewPlugin && record.rosterId!=0)
		{
			FRostersViewPlugin->rostersView()->removeNotify(record.rosterId);
		}
		if (!record.popupWidget.isNull())
		{
			record.popupWidget->deleteLater();
		}
		if (FTrayManager && record.trayId!=0)
		{
			FTrayManager->removeNotify(record.trayId);
		}
		if (!record.trayAction.isNull())
		{
			FNotifyMenu->removeAction(record.trayAction);
			delete record.trayAction;
		}
		if (!record.tabPageNotifier.isNull())
		{
			ITabPageNotifier *notifier =  qobject_cast<ITabPageNotifier *>(record.tabPageNotifier);
			if (notifier)
				notifier->removeNotify(record.tabPageId);
		}
		if (FNotifyRecords.isEmpty())
		{
			FActivateAll->setVisible(false);
			FRemoveAll->setVisible(false);
		}
		FNotifyMenu->menuAction()->setVisible(!FNotifyMenu->isEmpty());
		emit notificationRemoved(ANotifyId);
	}
}

void Notifications::registerNotificationType(const QString &AType, int AOptionsOrder, const QString &ATitle, ushort AKindMask, ushort ADefault)
{
	if (!FTypeRecords.contains(AType))
	{
		TypeRecord typeRecord;
		typeRecord.optionsOrder = AOptionsOrder;
		typeRecord.title = ATitle;
		typeRecord.kinds = 0xFFFF;
		typeRecord.defaults = ADefault;
		typeRecord.kindMask = AKindMask;
		FTypeRecords.insert(AType,typeRecord);
	}
}

ushort Notifications::notificationKinds(const QString &AType) const
{
	if (FTypeRecords.contains(AType))
	{
		TypeRecord &typeRecord = FTypeRecords[AType];
		if (typeRecord.kinds == 0xFFFF)
			typeRecord.kinds = Options::node(OPV_NOTIFICATIONS_TYPEKINDS_ITEM,AType).value().toInt() ^ typeRecord.defaults;
		return typeRecord.kinds;
	}
	return 0xFFFF;
}

void Notifications::setNotificationKinds(const QString &AType, ushort AKinds)
{
	if (FTypeRecords.contains(AType))
	{
		TypeRecord &typeRecord = FTypeRecords[AType];
		typeRecord.kinds = AKinds & typeRecord.kindMask;
		Options::node(OPV_NOTIFICATIONS_TYPEKINDS_ITEM,AType).setValue(typeRecord.kinds ^ typeRecord.defaults);
	}
}

void Notifications::removeNotificationType(const QString &AType)
{
	FTypeRecords.remove(AType);
	Options::node(OPV_NOTIFICATIONS_ROOT).removeChilds("type-kinds.type",AType);
}

void Notifications::insertNotificationHandler(int AOrder, INotificationHandler *AHandler)
{
	if (AHandler != NULL)
	{
		FHandlers.insertMulti(AOrder, AHandler);
		emit notificationHandlerInserted(AOrder,AHandler);
	}
}

void Notifications::removeNotificationHandler(int AOrder, INotificationHandler *AHandler)
{
	if (FHandlers.contains(AOrder,AHandler))
	{
		FHandlers.remove(AOrder,AHandler);
		emit notificationHandlerRemoved(AOrder,AHandler);
	}
}

QImage Notifications::contactAvatar(const Jid &AContactJid) const
{
	return FAvatars!=NULL ? FAvatars->avatarImage(AContactJid) : QImage();
}

QIcon Notifications::contactIcon(const Jid &AStreamJid, const Jid &AContactJid) const
{
	return FStatusIcons!=NULL ? FStatusIcons->iconByJid(AStreamJid,AContactJid) : QIcon();
}

QString Notifications::contactName(const Jid &AStreamJId, const Jid &AContactJid) const
{
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AStreamJId) : NULL;
	QString name = roster!=NULL ? roster->rosterItem(AContactJid).name : AContactJid.node();
	if (name.isEmpty())
		name = AContactJid.bare();
	return name;
}

int Notifications::notifyIdByRosterId(int ARosterId) const
{
	QMap<int,NotifyRecord>::const_iterator it = FNotifyRecords.constBegin();
	for (; it!=FNotifyRecords.constEnd(); it++)
		if (it.value().rosterId == ARosterId)
			return it.key();
	return -1;
}

int Notifications::notifyIdByTrayId(int ATrayId) const
{
	QMap<int,NotifyRecord>::const_iterator it = FNotifyRecords.constBegin();
	for (; it!=FNotifyRecords.constEnd(); it++)
		if (it.value().trayId == ATrayId)
			return it.key();
	return -1;
}

int Notifications::notifyIdByWidget(NotifyWidget *AWidget) const
{
	QMap<int,NotifyRecord>::const_iterator it = FNotifyRecords.constBegin();
	for (; it!=FNotifyRecords.constEnd(); it++)
		if (it.value().popupWidget == AWidget)
			return it.key();
	return -1;
}

bool Notifications::showNotifyByHandler(ushort AKind, int ANotifyId, const INotification &ANotification) const
{
	QMultiMap<int, INotificationHandler *>::const_iterator it = FHandlers.constBegin();
	while (it != FHandlers.constEnd())
	{
		if (it.value()->showNotification(it.key(),AKind,ANotifyId,ANotification))
			return true;
		it++;
	}
	return false;
}

void Notifications::removeInvisibleNotification(int ANotifyId)
{
	NotifyRecord record = FNotifyRecords.value(ANotifyId);
	if (record.notification.flags & INotification::RemoveInvisible)
	{
		bool invisible = true;
		if (record.trayId != 0)
			invisible = false;
		if (record.rosterId != 0)
			invisible = false;
		if (record.tabPageId != 0)
			invisible = false;
		if (!record.popupWidget.isNull())
			invisible = false;
		if (invisible)
			removeNotification(ANotifyId);
	}
}

void Notifications::onActivateDelayedActivations()
{
	foreach(int notifyId, FDelayedActivations)
		activateNotification(notifyId);
	FDelayedActivations.clear();
}

void Notifications::onSoundOnOffActionTriggered(bool)
{
	OptionsNode node = Options::node(OPV_NOTIFICATIONS_ENABLESOUND);
	node.setValue(!node.value().toBool());
}

void Notifications::onTrayActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		foreach(int notifyId, FNotifyRecords.keys())
		{
			if (action == FActivateAll)
				activateNotification(notifyId);
			else if (action == FRemoveAll)
				removeNotification(notifyId);
		}
	}
}

void Notifications::onRosterNotifyActivated(IRosterIndex *AIndex, int ANotifyId)
{
	Q_UNUSED(AIndex);
	activateNotification(notifyIdByRosterId(ANotifyId));
}

void Notifications::onRosterNotifyRemoved(IRosterIndex *AIndex, int ANotifyId)
{
	Q_UNUSED(AIndex);
	int notifyId = notifyIdByRosterId(ANotifyId);
	if (FNotifyRecords.contains(notifyId))
		FNotifyRecords[notifyId].rosterId = 0;
}

void Notifications::onTrayNotifyActivated(int ANotifyId, QSystemTrayIcon::ActivationReason AReason)
{
	if (AReason == QSystemTrayIcon::Trigger)
		activateNotification(notifyIdByTrayId(ANotifyId));
}

void Notifications::onTrayNotifyRemoved(int ANotifyId)
{
	int notifyId = notifyIdByTrayId(ANotifyId);
	if (FNotifyRecords.contains(notifyId))
		FNotifyRecords[notifyId].trayId = 0;
}

void Notifications::onWindowNotifyActivated()
{
	activateNotification(notifyIdByWidget(qobject_cast<NotifyWidget*>(sender())));
}

void Notifications::onWindowNotifyRemoved()
{
	removeNotification(notifyIdByWidget(qobject_cast<NotifyWidget*>(sender())));
}

void Notifications::onWindowNotifyDestroyed()
{
	int notifyId = notifyIdByWidget(qobject_cast<NotifyWidget*>(sender()));
	if (FNotifyRecords.contains(notifyId))
	{
		FNotifyRecords[notifyId].popupWidget = NULL;
		removeInvisibleNotification(notifyId);
	}
}

void Notifications::onActionNotifyActivated(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		int notifyId = action->data(ADR_NOTIFYID).toInt();
		activateNotification(notifyId);
	}
}

void Notifications::onOptionsOpened()
{
	onOptionsChanged(Options::node(OPV_NOTIFICATIONS_ENABLESOUND));
	onOptionsChanged(Options::node(OPV_NOTIFICATIONS_ENABLEALERTS));
}

void Notifications::onOptionsChanged(const OptionsNode &ANode)
{
	if (ANode.path() == OPV_NOTIFICATIONS_ENABLESOUND)
	{
		FSoundOnOff->setIcon(RSR_STORAGE_MENUICONS, ANode.value().toBool() ? MNI_NOTIFICATIONS_SOUND_ON : MNI_NOTIFICATIONS_SOUND_OFF);
	}
	else if (ANode.path() == OPV_NOTIFICATIONS_ENABLEALERTS)
	{
		WidgetManager::setWidgetAlertEnabled(ANode.value().toBool());
	}
}

Q_EXPORT_PLUGIN2(plg_notifications, Notifications)
