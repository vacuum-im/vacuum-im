#include "rostersviewplugin.h"

#include <QTimer>
#include <QScrollBar>

RostersViewPlugin::RostersViewPlugin()
{
	FRostersModel = NULL;
	FMainWindowPlugin = NULL;
	FOptionsManager = NULL;
	FRosterPlugin = NULL;
	FAccountManager = NULL;

	FSortFilterProxyModel = NULL;
	FLastModel = NULL;
	FShowOfflineAction = NULL;
	FShowResource = true;
	FStartRestoreExpandState = false;

	FViewSavedState.sliderPos = 0;
	FViewSavedState.currentIndex = NULL;

	FRostersView = new RostersView;
	connect(FRostersView,SIGNAL(viewModelAboutToBeChanged(QAbstractItemModel *)),SLOT(onViewModelAboutToBeChanged(QAbstractItemModel *)));
	connect(FRostersView,SIGNAL(viewModelChanged(QAbstractItemModel *)),SLOT(onViewModelChanged(QAbstractItemModel *)));
	connect(FRostersView,SIGNAL(collapsed(const QModelIndex &)),SLOT(onViewIndexCollapsed(const QModelIndex &)));
	connect(FRostersView,SIGNAL(expanded(const QModelIndex &)),SLOT(onViewIndexExpanded(const QModelIndex &)));
	connect(FRostersView,SIGNAL(destroyed(QObject *)), SLOT(onRostersViewDestroyed(QObject *)));
}

RostersViewPlugin::~RostersViewPlugin()
{
	delete FRostersView;
}

void RostersViewPlugin::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Roster View");
	APluginInfo->description = tr("Displays a hierarchical roster's model");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(ROSTERSMODEL_UUID);
}

bool RostersViewPlugin::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
	IPlugin *plugin = APluginManager->pluginInterface("IRostersModel").value(0,NULL);
	if (plugin)
	{
		FRostersModel = qobject_cast<IRostersModel *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IMainWindowPlugin").value(0,NULL);
	if (plugin)
	{
		FMainWindowPlugin = qobject_cast<IMainWindowPlugin *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IRosterPlugin").value(0,NULL);
	if (plugin)
	{
		FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
		if (FRosterPlugin)
		{
			connect(FRosterPlugin->instance(),SIGNAL(rosterStreamJidAboutToBeChanged(IRoster *, const Jid &)),
			        SLOT(onRosterStreamJidAboutToBeChanged(IRoster *, const Jid &)));
		}
	}

	plugin = APluginManager->pluginInterface("IAccountManager").value(0,NULL);
	if (plugin)
	{
		FAccountManager = qobject_cast<IAccountManager *>(plugin->instance());
		if (FAccountManager)
		{
			connect(FAccountManager->instance(),SIGNAL(shown(IAccount *)),SLOT(onAccountShown(IAccount *)));
			connect(FAccountManager->instance(),SIGNAL(hidden(IAccount *)),SLOT(onAccountHidden(IAccount *)));
			connect(FAccountManager->instance(),SIGNAL(destroyed(const QUuid &)),SLOT(onAccountDestroyed(const QUuid &)));
		}
	}

	plugin = APluginManager->pluginInterface("IOptionsManager").value(0,NULL);
	if (plugin)
	{
		FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());
	}

	connect(Options::instance(),SIGNAL(optionsOpened()),SLOT(onOptionsOpened()));
	connect(Options::instance(),SIGNAL(optionsChanged(const OptionsNode &)),SLOT(onOptionsChanged(const OptionsNode &)));

	return FRostersModel!=NULL;
}

bool RostersViewPlugin::initObjects()
{
	Shortcuts::declareShortcut(SCT_MAINWINDOW_TOGGLEOFFLINE, tr("Show/Hide offline contacts"),QKeySequence::UnknownKey);
	
	Shortcuts::declareGroup(SCTG_ROSTERVIEW,tr("Roster"));
	Shortcuts::declareShortcut(SCT_ROSTERVIEW_COPYJID,tr("Copy contact JID to clipboard"),QKeySequence::UnknownKey,Shortcuts::WidgetShortcut);
	Shortcuts::declareShortcut(SCT_ROSTERVIEW_COPYNAME,tr("Copy contact name to clipboard"),QKeySequence::UnknownKey,Shortcuts::WidgetShortcut);
	Shortcuts::declareShortcut(SCT_ROSTERVIEW_COPYSTATUS,tr("Copy contact status to clipboard"),QKeySequence::UnknownKey,Shortcuts::WidgetShortcut);

	FSortFilterProxyModel = new SortFilterProxyModel(this, this);
	FSortFilterProxyModel->setSortLocaleAware(true);
	FSortFilterProxyModel->setDynamicSortFilter(true);
	FSortFilterProxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
	FSortFilterProxyModel->sort(0,Qt::AscendingOrder);
	FRostersView->insertProxyModel(FSortFilterProxyModel,RPO_ROSTERSVIEW_SORTFILTER);

	if (FMainWindowPlugin)
	{
		FShowOfflineAction = new Action(this);
		FShowOfflineAction->setIcon(RSR_STORAGE_MENUICONS, MNI_ROSTERVIEW_HIDE_OFFLINE);
		FShowOfflineAction->setToolTip(tr("Show/Hide offline contacts"));
		FShowOfflineAction->setShortcutId(SCT_MAINWINDOW_TOGGLEOFFLINE);
		connect(FShowOfflineAction,SIGNAL(triggered(bool)),SLOT(onShowOfflineContactsAction(bool)));
		FMainWindowPlugin->mainWindow()->topToolBarChanger()->insertAction(FShowOfflineAction,TBG_MWTTB_ROSTERSVIEW);
		FMainWindowPlugin->mainWindow()->rostersWidget()->insertWidget(0,FRostersView);
	}

	if (FRostersModel)
	{
		FRostersModel->insertDefaultDataHolder(this);
		FRostersView->setRostersModel(FRostersModel);
	}

	Shortcuts::insertWidgetShortcut(SCT_ROSTERVIEW_COPYJID,FRostersView);
	Shortcuts::insertWidgetShortcut(SCT_ROSTERVIEW_COPYNAME,FRostersView);
	Shortcuts::insertWidgetShortcut(SCT_ROSTERVIEW_COPYSTATUS,FRostersView);

	return true;
}

bool RostersViewPlugin::initSettings()
{
	Options::setDefaultValue(OPV_ROSTER_SHOWOFFLINE,true);
	Options::setDefaultValue(OPV_ROSTER_SHOWRESOURCE,true);
	Options::setDefaultValue(OPV_ROSTER_SHOWSTATUSTEXT,true);
	Options::setDefaultValue(OPV_ROSTER_SORTBYSTATUS,false);

	if (FOptionsManager)
	{
		IOptionsDialogNode dnode = { ONO_ROSTER, OPN_ROSTER, tr("Roster"), MNI_ROSTERVIEW_OPTIONS };
		FOptionsManager->insertOptionsDialogNode(dnode);
		FOptionsManager->insertOptionsHolder(this);
	}
	return true;
}

QMultiMap<int, IOptionsWidget *> RostersViewPlugin::optionsWidgets(const QString &ANodeId, QWidget *AParent)
{
	QMultiMap<int, IOptionsWidget *> widgets;
	if (FOptionsManager && ANodeId == OPN_ROSTER)
	{
		widgets.insertMulti(OWO_ROSTER,FOptionsManager->optionsNodeWidget(Options::node(OPV_ROSTER_SHOWOFFLINE),tr("Show offline contact"),AParent));
		widgets.insertMulti(OWO_ROSTER,FOptionsManager->optionsNodeWidget(Options::node(OPV_ROSTER_SHOWRESOURCE),tr("Show contact resource in roster"),AParent));
		widgets.insertMulti(OWO_ROSTER,FOptionsManager->optionsNodeWidget(Options::node(OPV_ROSTER_SHOWSTATUSTEXT),tr("Show status message in roster"),AParent));
		widgets.insertMulti(OWO_ROSTER,FOptionsManager->optionsNodeWidget(Options::node(OPV_ROSTER_SORTBYSTATUS),tr("Sort contacts by status"),AParent));
	}
	return widgets;
}

int RostersViewPlugin::rosterDataOrder() const
{
	return RDHO_DEFAULT;
}

QList<int> RostersViewPlugin::rosterDataRoles() const
{
	static QList<int> dataRoles  = QList<int>()
	                               << Qt::DisplayRole
	                               << Qt::BackgroundColorRole
	                               << Qt::ForegroundRole
	                               << RDR_FONT_WEIGHT
	                               << RDR_STATES_FORCE_ON;
	return dataRoles;
}

QList<int> RostersViewPlugin::rosterDataTypes() const
{
	static QList<int> indexTypes  = QList<int>()
	                                << RIT_STREAM_ROOT
	                                << RIT_GROUP
	                                << RIT_GROUP_BLANK
	                                << RIT_GROUP_AGENTS
	                                << RIT_GROUP_MY_RESOURCES
	                                << RIT_GROUP_NOT_IN_ROSTER
	                                << RIT_CONTACT
	                                << RIT_AGENT
	                                << RIT_MY_RESOURCE;
	return indexTypes;
}

QVariant RostersViewPlugin::rosterData(const IRosterIndex *AIndex, int ARole) const
{
	switch (AIndex->type())
	{
	case RIT_STREAM_ROOT:
		switch (ARole)
		{
		case Qt::DisplayRole:
			{
				QString display = AIndex->data(RDR_NAME).toString();
				if (display.isEmpty())
					display = AIndex->data(RDR_JID).toString();
				return display;
			}
		case Qt::ForegroundRole:
			return FRostersView->palette().color(QPalette::Active, QPalette::BrightText);
		case Qt::BackgroundColorRole:
			return FRostersView->palette().color(QPalette::Active, QPalette::Dark);
		case RDR_FONT_WEIGHT:
			return QFont::Bold;
		case RDR_STATES_FORCE_ON:
			return QStyle::State_Children;
		}
		break;
	case RIT_GROUP:
	case RIT_GROUP_BLANK:
	case RIT_GROUP_AGENTS:
	case RIT_GROUP_MY_RESOURCES:
	case RIT_GROUP_NOT_IN_ROSTER:
		switch (ARole)
		{
		case Qt::DisplayRole:
			return AIndex->data(RDR_NAME);
		case Qt::ForegroundRole:
			return FRostersView->palette().color(QPalette::Active, QPalette::Highlight);
		case RDR_FONT_WEIGHT:
			return QFont::DemiBold;
		case RDR_STATES_FORCE_ON:
			return QStyle::State_Children;
		}
		break;
	case RIT_CONTACT:
		switch (ARole)
		{
		case Qt::DisplayRole:
			{
				Jid indexJid = AIndex->data(RDR_JID).toString();
				QString display = AIndex->data(RDR_NAME).toString();
				if (display.isEmpty())
					display = indexJid.bare();
				if (FShowResource && !indexJid.resource().isEmpty())
					display += "/" + indexJid.resource();
				return display;
			}
		}
		break;
	case RIT_AGENT:
		switch (ARole)
		{
		case Qt::DisplayRole:
			{
				QString display = AIndex->data(RDR_NAME).toString();
				if (display.isEmpty())
				{
					Jid indexJid = AIndex->data(RDR_JID).toString();
					display = indexJid.bare();
				}
				return display;
			}
		}
		break;
	case RIT_MY_RESOURCE:
		switch (ARole)
		{
		case Qt::DisplayRole:
			{
				Jid indexJid = AIndex->data(RDR_JID).toString();
				return indexJid.resource();
			}
		}
		break;
	}
	return QVariant();
}

bool RostersViewPlugin::setRosterData(IRosterIndex *AIndex, int ARole, const QVariant &AValue)
{
	Q_UNUSED(AIndex);
	Q_UNUSED(ARole);
	Q_UNUSED(AValue);
	return false;
}

IRostersView *RostersViewPlugin::rostersView()
{
	return FRostersView;
}

void RostersViewPlugin::startRestoreExpandState()
{
	if (!FStartRestoreExpandState)
	{
		FStartRestoreExpandState = true;
		QTimer::singleShot(0,this,SLOT(onRestoreExpandState()));
	}
}

void RostersViewPlugin::restoreExpandState(const QModelIndex &AParent)
{
	QAbstractItemModel *curModel = FRostersView->model();
	int rows = curModel!=NULL ? curModel->rowCount(AParent) : -1;
	if (rows > 0)
	{
		if (AParent.isValid())
			loadExpandState(AParent);
		for (int row = 0; row<rows; row++)
			restoreExpandState(curModel->index(row,0,AParent));
	}
}

QString RostersViewPlugin::indexGroupName(const QModelIndex &AIndex) const
{
	int indexType = AIndex.data(RDR_TYPE).toInt();
	switch (indexType)
	{
	case RIT_GROUP:
		return AIndex.data(RDR_GROUP).toString();
	case RIT_GROUP_BLANK:
		return FRostersModel!=NULL ? FRostersModel->blankGroupName() : QString::null;
	case RIT_GROUP_NOT_IN_ROSTER:
		return FRostersModel!=NULL ? FRostersModel->notInRosterGroupName() : QString::null;
	case RIT_GROUP_MY_RESOURCES:
		return FRostersModel!=NULL ? FRostersModel->myResourcesGroupName() : QString::null;
	case RIT_GROUP_AGENTS:
		return FRostersModel!=NULL ? FRostersModel->agentsGroupName() : QString::null;
	default:
		return QString::null;
	}
	return QString::null;
}

void RostersViewPlugin::loadExpandState(const QModelIndex &AIndex)
{
	QString groupName = indexGroupName(AIndex);
	if (!groupName.isEmpty() || AIndex.data(RDR_TYPE).toInt()==RIT_STREAM_ROOT)
	{
		Jid streamJid = AIndex.data(RDR_STREAM_JID).toString();
		bool isExpanded = FExpandState.value(streamJid).value(groupName,true);
		if (isExpanded && !FRostersView->isExpanded(AIndex))
			FRostersView->expand(AIndex);
		else if (!isExpanded && FRostersView->isExpanded(AIndex))
			FRostersView->collapse(AIndex);
	}
}

void RostersViewPlugin::saveExpandState(const QModelIndex &AIndex)
{
	QString groupName = indexGroupName(AIndex);
	if (!groupName.isEmpty() || AIndex.data(RDR_TYPE).toInt()==RIT_STREAM_ROOT)
	{
		Jid streamJid = AIndex.data(RDR_STREAM_JID).toString();
		if (!FRostersView->isExpanded(AIndex))
			FExpandState[streamJid][groupName] = false;
		else
			FExpandState[streamJid].remove(groupName);
	}
}

void RostersViewPlugin::onRostersViewDestroyed(QObject *AObject)
{
	Q_UNUSED(AObject);
	FRostersView = NULL;
}

void RostersViewPlugin::onViewModelAboutToBeReset()
{
	if (FRostersView->currentIndex().isValid())
	{
		FViewSavedState.currentIndex = FRostersView->rostersModel()->rosterIndexByModelIndex(FRostersView->mapToModel(FRostersView->currentIndex()));
		FViewSavedState.sliderPos = FRostersView->verticalScrollBar()->sliderPosition();
	}
	else
		FViewSavedState.currentIndex = NULL;
}

void RostersViewPlugin::onViewModelReset()
{
	restoreExpandState();
	if (FViewSavedState.currentIndex != NULL)
	{
		FRostersView->setCurrentIndex(FRostersView->mapFromModel(FRostersView->rostersModel()->modelIndexByRosterIndex(FViewSavedState.currentIndex)));
		FRostersView->verticalScrollBar()->setSliderPosition(FViewSavedState.sliderPos);
	}
}

void RostersViewPlugin::onViewModelAboutToBeChanged(QAbstractItemModel *AModel)
{
	Q_UNUSED(AModel);
	if (FRostersView->model())
	{
		disconnect(FRostersView->model(),SIGNAL(modelAboutToBeReset()),this,SLOT(onViewModelAboutToBeReset()));
		disconnect(FRostersView->model(),SIGNAL(modelReset()),this,SLOT(onViewModelReset()));
		disconnect(FRostersView->model(),SIGNAL(rowsInserted(const QModelIndex &, int , int )),this,SLOT(onViewRowsInserted(const QModelIndex &, int , int )));
	}
}

void RostersViewPlugin::onViewModelChanged(QAbstractItemModel *AModel)
{
	Q_UNUSED(AModel);
	if (FRostersView->model())
	{
		connect(FRostersView->model(),SIGNAL(modelAboutToBeReset()),SLOT(onViewModelAboutToBeReset()));
		connect(FRostersView->model(),SIGNAL(modelReset()),SLOT(onViewModelReset()));
		connect(FRostersView->model(),SIGNAL(rowsInserted(const QModelIndex &, int , int )),SLOT(onViewRowsInserted(const QModelIndex &, int , int )));
		startRestoreExpandState();
	}
}

void RostersViewPlugin::onViewRowsInserted(const QModelIndex &AParent, int AStart, int AEnd)
{
	if (AStart == 0)
		loadExpandState(AParent);
	for (int row=AStart; row<=AEnd; row++)
		restoreExpandState(AParent.child(row,0));
}

void RostersViewPlugin::onViewIndexCollapsed(const QModelIndex &AIndex)
{
	saveExpandState(AIndex);
}

void RostersViewPlugin::onViewIndexExpanded(const QModelIndex &AIndex)
{
	saveExpandState(AIndex);
}

void RostersViewPlugin::onRosterStreamJidAboutToBeChanged(IRoster *ARoster, const Jid &AAfter)
{
	Jid before = ARoster->streamJid();
	if (FExpandState.contains(before))
	{
		QHash<QString, bool> state = FExpandState.take(before);
		if (before && AAfter)
			FExpandState.insert(AAfter,state);
	}
}

void RostersViewPlugin::onAccountShown(IAccount *AAccount)
{
	if (AAccount->isActive())
	{
		QByteArray data = Options::fileValue("rosterview.expand-state",AAccount->accountId().toString()).toByteArray();
		QDataStream stream(data);
		stream >> FExpandState[AAccount->xmppStream()->streamJid()];
	}
}

void RostersViewPlugin::onAccountHidden(IAccount *AAccount)
{
	if (AAccount->isActive())
	{
		QByteArray data;
		QDataStream stream(&data, QIODevice::WriteOnly);
		stream << FExpandState.take(AAccount->xmppStream()->streamJid());
		Options::setFileValue(data,"rosterview.expand-state",AAccount->accountId().toString());
	}
}

void RostersViewPlugin::onAccountDestroyed(const QUuid &AAccountId)
{
	Options::setFileValue(QVariant(),"rosterview.expand-state",AAccountId.toString());
}

void RostersViewPlugin::onRestoreExpandState()
{
	restoreExpandState();
	FStartRestoreExpandState = false;
}

void RostersViewPlugin::onOptionsOpened()
{
	onOptionsChanged(Options::node(OPV_ROSTER_SHOWOFFLINE));
	onOptionsChanged(Options::node(OPV_ROSTER_SHOWRESOURCE));
	onOptionsChanged(Options::node(OPV_ROSTER_SHOWSTATUSTEXT));
	onOptionsChanged(Options::node(OPV_ROSTER_SORTBYSTATUS));
}

void RostersViewPlugin::onOptionsChanged(const OptionsNode &ANode)
{
	if (ANode.path() == OPV_ROSTER_SHOWOFFLINE)
	{
		FShowOfflineAction->setIcon(RSR_STORAGE_MENUICONS, ANode.value().toBool() ? MNI_ROSTERVIEW_SHOW_OFFLINE : MNI_ROSTERVIEW_HIDE_OFFLINE);
		FSortFilterProxyModel->invalidate();
		if (ANode.value().toBool())
			restoreExpandState();
	}
	else if (ANode.path() == OPV_ROSTER_SHOWRESOURCE)
	{
		FShowResource = ANode.value().toBool();
		emit rosterDataChanged(NULL, Qt::DisplayRole);
	}
	else if (ANode.path() == OPV_ROSTER_SHOWSTATUSTEXT)
	{
		FRostersView->updateStatusText();
		emit rosterDataChanged(NULL, Qt::DisplayRole);
	}
	else if (ANode.path() == OPV_ROSTER_SORTBYSTATUS)
	{
		FSortFilterProxyModel->invalidate();
	}
}

void RostersViewPlugin::onShowOfflineContactsAction(bool)
{
	OptionsNode node = Options::node(OPV_ROSTER_SHOWOFFLINE);
	node.setValue(!node.value().toBool());
}

Q_EXPORT_PLUGIN2(plg_rostersview, RostersViewPlugin)
