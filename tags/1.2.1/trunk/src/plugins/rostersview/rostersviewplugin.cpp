#include "rostersviewplugin.h"

#include <QTimer>
#include <QScrollBar>

RostersViewPlugin::RostersViewPlugin()
{
	FRostersModel = NULL;
	FMainWindowPlugin = NULL;
	FOptionsManager = NULL;

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

bool RostersViewPlugin::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
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
	
	Shortcuts::declareGroup(SCTG_ROSTERVIEW,tr("Roster"),SGO_ROSTERVIEW);
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

		FMainWindowPlugin->mainWindow()->mainTabWidget()->insertTabPage(MWTP_ROSTERSVIEW,FRostersView);
	}

	if (FRostersModel)
	{
		FRostersModel->insertDefaultDataHolder(this);
		FRostersView->setRostersModel(FRostersModel);
	}

	FRostersView->insertLabelHolder(RLHO_ROSTERSVIEW_NOTIFY,FRostersView);
	FRostersView->insertLabelHolder(RLHO_ROSTERSVIEW_DISPLAY,FRostersView);

	Shortcuts::insertWidgetShortcut(SCT_ROSTERVIEW_COPYJID,FRostersView);
	Shortcuts::insertWidgetShortcut(SCT_ROSTERVIEW_COPYNAME,FRostersView);
	Shortcuts::insertWidgetShortcut(SCT_ROSTERVIEW_COPYSTATUS,FRostersView);

	registerExpandableRosterIndexType(RIT_STREAM_ROOT,RDR_PREP_BARE_JID);
	registerExpandableRosterIndexType(RIT_GROUP,RDR_GROUP);
	registerExpandableRosterIndexType(RIT_GROUP_BLANK,RDR_NAME);
	registerExpandableRosterIndexType(RIT_GROUP_NOT_IN_ROSTER,RDR_NAME);
	registerExpandableRosterIndexType(RIT_GROUP_MY_RESOURCES,RDR_NAME);
	registerExpandableRosterIndexType(RIT_GROUP_AGENTS,RDR_NAME);

	return true;
}

bool RostersViewPlugin::initSettings()
{
	Options::setDefaultValue(OPV_ROSTER_SHOWOFFLINE,true);
	Options::setDefaultValue(OPV_ROSTER_SHOWRESOURCE,true);
	Options::setDefaultValue(OPV_ROSTER_SORTBYSTATUS,false);
	Options::setDefaultValue(OPV_ROSTER_HIDE_SCROLLBAR,false);

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
		widgets.insertMulti(OWO_ROSTER,FOptionsManager->optionsNodeWidget(Options::node(OPV_ROSTER_SORTBYSTATUS),tr("Sort contacts by status"),AParent));
		widgets.insertMulti(OWO_ROSTER,FOptionsManager->optionsNodeWidget(Options::node(OPV_ROSTER_HIDE_SCROLLBAR),tr("Do not show the scroll bars"),AParent));
	}
	return widgets;
}

int RostersViewPlugin::rosterDataOrder() const
{
	return RDHO_DEFAULT;
}

QList<int> RostersViewPlugin::rosterDataRoles() const
{
	static const QList<int> dataRoles = QList<int>() 
		<< Qt::DisplayRole << Qt::ForegroundRole << Qt::BackgroundColorRole << RDR_STATES_FORCE_ON << RDR_ALLWAYS_VISIBLE;
	return dataRoles;
}

QList<int> RostersViewPlugin::rosterDataTypes() const
{
	static const QList<int> indexTypes = QList<int>() << RIT_ANY_TYPE;
	return indexTypes;
}

QVariant RostersViewPlugin::rosterData(const IRosterIndex *AIndex, int ARole) const
{
	if (FRostersModel)
	{
		if (AIndex->parentIndex() == FRostersModel->rootIndex())
		{
			switch (ARole)
			{
			case Qt::DisplayRole:
				return AIndex->data(RDR_NAME);
			case Qt::ForegroundRole:
				return FRostersView->palette().color(QPalette::Active, QPalette::BrightText);
			case Qt::BackgroundColorRole:
				return FRostersView->palette().color(QPalette::Active, QPalette::Dark);
			case RDR_STATES_FORCE_ON:
				return QStyle::State_Children;
			case RDR_ALLWAYS_VISIBLE:
				return 1;
			}
		}
		else if (FRostersModel->isGroupType(AIndex->type()))
		{
			switch (ARole)
			{
			case Qt::DisplayRole:
				return AIndex->data(RDR_NAME);
			case Qt::ForegroundRole:
				return FRostersView->palette().color(QPalette::Active, QPalette::Highlight);
			case RDR_STATES_FORCE_ON:
				return QStyle::State_Children;
			}
		}
		else if (ARole == Qt::DisplayRole)
		{
			Jid streamJid = AIndex->data(RDR_STREAM_JID).toString();
			Jid indexJid = AIndex->data(RDR_FULL_JID).toString();
			QString name = AIndex->data(RDR_NAME).toString();
			if (streamJid.pBare() != indexJid.pBare())
			{
				if (name.isEmpty())
					name = indexJid.uBare();
				if (FShowResource && !indexJid.node().isEmpty() && !indexJid.resource().isEmpty())
					name += "/" + indexJid.resource();
			}
			else if (name.isEmpty())
			{
				name = indexJid.resource();
			}
			return name;
		}
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
	if (rows >= 0)
	{
		if (AParent.isValid())
			loadExpandState(AParent);
		for (int row = 0; row<rows; row++)
			restoreExpandState(curModel->index(row,0,AParent));
	}
}

void RostersViewPlugin::registerExpandableRosterIndexType(int AType, int AUniqueRole)
{
	if (!FExpandableTypes.contains(AType))
		FExpandableTypes.insert(AType,AUniqueRole);
}

QString RostersViewPlugin::rootExpandId(const QModelIndex &AIndex) const
{
	QModelIndex index = AIndex;
	while (index.parent().isValid())
		index=index.parent();
	return indexExpandId(index);
}

QString RostersViewPlugin::indexExpandId(const QModelIndex &AIndex) const
{
	int role = FExpandableTypes.value(AIndex.data(RDR_TYPE).toInt());
	if (role > 0)
		return AIndex.data(role).toString();
	return QString::null;
}

void RostersViewPlugin::loadExpandState(const QModelIndex &AIndex)
{
	QString indexId = indexExpandId(AIndex);
	if (!indexId.isEmpty())
	{
		QString rootId = rootExpandId(AIndex);
		bool isExpanded = FExpandStates.value(rootId).value(indexId,true);
		if (isExpanded && !FRostersView->isExpanded(AIndex))
			FRostersView->expand(AIndex);
		else if (!isExpanded && FRostersView->isExpanded(AIndex))
			FRostersView->collapse(AIndex);
	}
}

void RostersViewPlugin::saveExpandState(const QModelIndex &AIndex)
{
	QString indexId = indexExpandId(AIndex);
	if (!indexId.isEmpty())
	{
		QString rootId = rootExpandId(AIndex);
		if (!rootId.isEmpty())
		{
			if (!FRostersView->isExpanded(AIndex))
				FExpandStates[rootId][indexId] = false;
			else
				FExpandStates[rootId].remove(indexId);
		}
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
	{
		FViewSavedState.currentIndex = NULL;
	}
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
		disconnect(FRostersView->model(),SIGNAL(rowsAboutToBeRemoved(const QModelIndex &, int , int )),this,SLOT(onViewRowsAboutToBeRemoved(const QModelIndex &, int , int )));
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
		connect(FRostersView->model(),SIGNAL(rowsAboutToBeRemoved(const QModelIndex &, int , int )),SLOT(onViewRowsAboutToBeRemoved(const QModelIndex &, int , int )));
		startRestoreExpandState();
	}
}

void RostersViewPlugin::onViewRowsInserted(const QModelIndex &AParent, int AStart, int AEnd)
{
	for (int row=AStart; row<=AEnd; row++)
	{
		QModelIndex index = FRostersView->model()->index(row,0,AParent);
		if (!AParent.isValid())
		{
			QString rootId = rootExpandId(index);
			if (!rootId.isEmpty() && !FExpandStates.contains(rootId))
			{
				QByteArray data = Options::fileValue("rosterview.expand-state",rootId).toByteArray();
				QDataStream stream(data);
				stream >> FExpandStates[rootId];
			}
		}
		restoreExpandState(index);
	}
}

void RostersViewPlugin::onViewRowsAboutToBeRemoved(const QModelIndex &AParent, int AStart, int AEnd)
{
	for (int row=AStart; !AParent.isValid() && row<=AEnd; row++)
	{
		QString rootId = rootExpandId(FRostersView->model()->index(row,0,AParent));
		if (FExpandStates.contains(rootId))
		{
			QByteArray data;
			QDataStream stream(&data, QIODevice::WriteOnly);
			stream << FExpandStates.take(rootId);
			Options::setFileValue(data,"rosterview.expand-state",rootId);
		}
	}
}

void RostersViewPlugin::onViewIndexCollapsed(const QModelIndex &AIndex)
{
	saveExpandState(AIndex);
}

void RostersViewPlugin::onViewIndexExpanded(const QModelIndex &AIndex)
{
	saveExpandState(AIndex);
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
	onOptionsChanged(Options::node(OPV_ROSTER_SORTBYSTATUS));
	onOptionsChanged(Options::node(OPV_ROSTER_HIDE_SCROLLBAR));
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
	else if (ANode.path() == OPV_ROSTER_SORTBYSTATUS)
	{
		FSortFilterProxyModel->invalidate();
	}
	else if (ANode.path() == OPV_ROSTER_HIDE_SCROLLBAR)
	{
		FRostersView->setVerticalScrollBarPolicy(ANode.value().toBool() ? Qt::ScrollBarAlwaysOff : Qt::ScrollBarAsNeeded);
		FRostersView->setHorizontalScrollBarPolicy(ANode.value().toBool() ? Qt::ScrollBarAlwaysOff : Qt::ScrollBarAsNeeded);
	}
}

void RostersViewPlugin::onShowOfflineContactsAction(bool)
{
	OptionsNode node = Options::node(OPV_ROSTER_SHOWOFFLINE);
	node.setValue(!node.value().toBool());
}

Q_EXPORT_PLUGIN2(plg_rostersview, RostersViewPlugin)
