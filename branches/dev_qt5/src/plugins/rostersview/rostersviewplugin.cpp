#include "rostersviewplugin.h"

#include <QTimer>
#include <QComboBox>
#include <QClipboard>
#include <QScrollBar>
#include <QApplication>
#include <QTextDocument>
#include <definitions/optionvalues.h>
#include <definitions/optionnodes.h>
#include <definitions/optionnodeorders.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/actiongroups.h>
#include <definitions/toolbargroups.h>
#include <definitions/mainwindowtabpages.h>
#include <definitions/rosterlabels.h>
#include <definitions/rosterindexkinds.h>
#include <definitions/rosterindexroles.h>
#include <definitions/rosterproxyorders.h>
#include <definitions/rostertooltiporders.h>
#include <definitions/rosterdataholderorders.h>
#include <definitions/rosterlabelholderorders.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/shortcuts.h>
#include <definitions/shortcutgrouporders.h>
#include <utils/textmanager.h>
#include <utils/shortcuts.h>
#include <utils/options.h>
#include <utils/action.h>
#include <utils/logger.h>

#define ADR_CLIPBOARD_DATA      Action::DR_Parametr1

RostersViewPlugin::RostersViewPlugin()
{
	FStatusIcons = NULL;
	FRostersModel = NULL;
	FPresenceManager = NULL;
	FOptionsManager = NULL;
	FAccountManager = NULL;
	FMainWindowPlugin = NULL;

	FSortFilterProxyModel = NULL;
	FLastModel = NULL;
	FShowOfflineAction = NULL;
	FShowStatus = true;
	FShowResource = true;
	FStartRestoreExpandState = false;

	FViewSavedState.sliderPos = 0;
	FViewSavedState.currentIndex = NULL;

	FRostersView = new RostersView;
	connect(FRostersView,SIGNAL(viewModelAboutToBeChanged(QAbstractItemModel *)),SLOT(onViewModelAboutToBeChanged(QAbstractItemModel *)));
	connect(FRostersView,SIGNAL(viewModelChanged(QAbstractItemModel *)),SLOT(onViewModelChanged(QAbstractItemModel *)));
	connect(FRostersView,SIGNAL(collapsed(const QModelIndex &)),SLOT(onViewIndexCollapsed(const QModelIndex &)));
	connect(FRostersView,SIGNAL(expanded(const QModelIndex &)),SLOT(onViewIndexExpanded(const QModelIndex &)));
	connect(FRostersView,SIGNAL(destroyed(QObject *)), SLOT(onViewDestroyed(QObject *)));

	connect(FRostersView,SIGNAL(indexClipboardMenu(const QList<IRosterIndex *> &, quint32, Menu *)),
		SLOT(onRostersViewClipboardMenu(const QList<IRosterIndex *> &, quint32, Menu *)));
	connect(FRostersView,SIGNAL(indexToolTips(IRosterIndex *, quint32, QMap<int,QString> &)),
		SLOT(onRostersViewIndexToolTips(IRosterIndex *, quint32, QMap<int,QString> &)));
	connect(FRostersView,SIGNAL(indexContextMenu(const QList<IRosterIndex *> &, quint32, Menu *)),
		SLOT(onRostersViewIndexContextMenu(const QList<IRosterIndex *> &, quint32, Menu *)));
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
		if (FRostersModel)
			connect(FRostersModel->instance(),SIGNAL(indexDataChanged(IRosterIndex *, int)),SLOT(onRostersModelIndexDataChanged(IRosterIndex *, int)));
	}

	plugin = APluginManager->pluginInterface("IStatusIcons").value(0,NULL);
	if (plugin)
	{
		FStatusIcons = qobject_cast<IStatusIcons *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IPresenceManager").value(0,NULL);
	if (plugin)
	{
		FPresenceManager = qobject_cast<IPresenceManager *>(plugin->instance());
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

	plugin = APluginManager->pluginInterface("IAccountManager").value(0,NULL);
	if (plugin)
	{
		FAccountManager = qobject_cast<IAccountManager *>(plugin->instance());
	}

	connect(Options::instance(),SIGNAL(optionsOpened()),SLOT(onOptionsOpened()));
	connect(Options::instance(),SIGNAL(optionsChanged(const OptionsNode &)),SLOT(onOptionsChanged(const OptionsNode &)));

	return FRostersModel!=NULL;
}

bool RostersViewPlugin::initObjects()
{
	Shortcuts::declareGroup(SCTG_ROSTERVIEW,tr("Contact-List"),SGO_ROSTERVIEW);
	Shortcuts::declareShortcut(SCT_ROSTERVIEW_TOGGLESHOWOFFLINE, tr("Show/Hide disconnected contacts"),QKeySequence::UnknownKey);

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
		FShowOfflineAction->setToolTip(tr("Show/Hide disconnected contacts"));
		FShowOfflineAction->setShortcutId(SCT_ROSTERVIEW_TOGGLESHOWOFFLINE);
		connect(FShowOfflineAction,SIGNAL(triggered(bool)),SLOT(onShowOfflineContactsAction(bool)));
		FMainWindowPlugin->mainWindow()->topToolBarChanger()->insertAction(FShowOfflineAction,TBG_MWTTB_ROSTERSVIEW);

		FMainWindowPlugin->mainWindow()->mainTabWidget()->insertTabPage(MWTP_ROSTERSVIEW,FRostersView);
	}

	if (FRostersModel)
	{
		FRostersView->setRostersModel(FRostersModel);
		FRostersModel->insertRosterDataHolder(RDHO_ROSTERSVIEW,this);
	}

	FRostersView->insertLabelHolder(RLHO_ROSTERSVIEW,this);
	FRostersView->insertLabelHolder(RLHO_ROSTERSVIEW_NOTIFY,FRostersView);

	registerExpandableRosterIndexKind(RIK_CONTACTS_ROOT,RDR_KIND);
	registerExpandableRosterIndexKind(RIK_STREAM_ROOT,RDR_PREP_BARE_JID);
	registerExpandableRosterIndexKind(RIK_GROUP,RDR_GROUP);
	registerExpandableRosterIndexKind(RIK_GROUP_ACCOUNTS,RDR_KIND);
	registerExpandableRosterIndexKind(RIK_GROUP_BLANK,RDR_KIND);
	registerExpandableRosterIndexKind(RIK_GROUP_NOT_IN_ROSTER,RDR_KIND);
	registerExpandableRosterIndexKind(RIK_GROUP_MY_RESOURCES,RDR_KIND);
	registerExpandableRosterIndexKind(RIK_GROUP_AGENTS,RDR_KIND);

	return true;
}

bool RostersViewPlugin::initSettings()
{
	Options::setDefaultValue(OPV_ROSTER_SHOWOFFLINE,true);
	Options::setDefaultValue(OPV_ROSTER_SHOWRESOURCE,false);
	Options::setDefaultValue(OPV_ROSTER_HIDESCROLLBAR,false);
	Options::setDefaultValue(OPV_ROSTER_MERGESTREAMS,true);
	Options::setDefaultValue(OPV_ROSTER_VIEWMODE,IRostersView::ViewFull);
	Options::setDefaultValue(OPV_ROSTER_SORTMODE,IRostersView::SortByStatus);

	if (FOptionsManager)
	{
		IOptionsDialogNode rosterNode = { ONO_ROSTERVIEW, OPN_ROSTERVIEW, MNI_ROSTERVIEW_OPTIONS, tr("Contacts List") };
		FOptionsManager->insertOptionsDialogNode(rosterNode);
		FOptionsManager->insertOptionsDialogHolder(this);
	}
	return true;
}

QMultiMap<int, IOptionsDialogWidget *> RostersViewPlugin::optionsDialogWidgets(const QString &ANodeId, QWidget *AParent)
{
	QMultiMap<int, IOptionsDialogWidget *> widgets;
	if (FOptionsManager && ANodeId == OPN_ROSTERVIEW)
	{
		widgets.insertMulti(OHO_ROSTER_VIEW, FOptionsManager->newOptionsDialogHeader(tr("Contacts list"),AParent));
		widgets.insertMulti(OWO_ROSTER_SHOWOFFLINE,FOptionsManager->newOptionsDialogWidget(Options::node(OPV_ROSTER_SHOWOFFLINE),tr("Show disconnected contacts"),AParent));
		widgets.insertMulti(OWO_ROSTER_MERGESTREAMS,FOptionsManager->newOptionsDialogWidget(Options::node(OPV_ROSTER_MERGESTREAMS),tr("Show contacts of all accounts in common list"),AParent));
		widgets.insertMulti(OWO_ROSTER_SHOWRESOURCE,FOptionsManager->newOptionsDialogWidget(Options::node(OPV_ROSTER_SHOWRESOURCE),tr("Show contact resource with highest priority"),AParent));
		widgets.insertMulti(OWO_ROSTER_HIDESCROLLBAR,FOptionsManager->newOptionsDialogWidget(Options::node(OPV_ROSTER_HIDESCROLLBAR),tr("Hide scroll bars in contact list window"),AParent));

		QComboBox *cmbViewMode = new QComboBox(AParent);
		cmbViewMode->addItem(tr("Full"), IRostersView::ViewFull);
		cmbViewMode->addItem(tr("Simplified"), IRostersView::ViewSimple);
		cmbViewMode->addItem(tr("Compact"), IRostersView::ViewCompact);
		widgets.insertMulti(OWO_ROSTER_VIEWMODE,FOptionsManager->newOptionsDialogWidget(Options::node(OPV_ROSTER_VIEWMODE),tr("Contacts list view:"),cmbViewMode,AParent));

		QComboBox *cmbSortMode = new QComboBox(AParent);
		cmbSortMode->addItem(tr("by status"), IRostersView::SortByStatus);
		cmbSortMode->addItem(tr("alphabetically"), IRostersView::SortAlphabetically);
		widgets.insertMulti(OWO_ROSTER_SORTMODE,FOptionsManager->newOptionsDialogWidget(Options::node(OPV_ROSTER_SORTMODE),tr("Sort contacts list:"),cmbSortMode,AParent));
	}
	return widgets;
}

QList<int> RostersViewPlugin::rosterDataRoles(int AOrder) const
{
	if (AOrder == RDHO_ROSTERSVIEW)
		return QList<int>() << Qt::DisplayRole << Qt::ForegroundRole << Qt::BackgroundColorRole << RDR_STATES_FORCE_ON << RDR_FORCE_VISIBLE;
	return QList<int>();
}

QVariant RostersViewPlugin::rosterData(int AOrder, const IRosterIndex *AIndex, int ARole) const
{
	IRostersModel *model = FRostersView->rostersModel();
	if (AOrder==RDHO_ROSTERSVIEW && model!=NULL)
	{
		if (AIndex->parentIndex() == model->rootIndex())
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
			case RDR_FORCE_VISIBLE:
				return 1;
			}
		}
		else if (model->isGroupKind(AIndex->kind()))
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
			if (AIndex->kind() != RIK_MY_RESOURCE)
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
		else if (ARole == RDR_FORCE_VISIBLE)
		{
			if (AIndex->kind() == RIK_STREAM_ROOT)
				return 1;
		}
	}
	return QVariant();
}

bool RostersViewPlugin::setRosterData(int AOrder, const QVariant &AValue, IRosterIndex *AIndex, int ARole)
{
	Q_UNUSED(AOrder); Q_UNUSED(AIndex); Q_UNUSED(ARole); Q_UNUSED(AValue);
	return false;
}

QList<quint32> RostersViewPlugin::rosterLabels(int AOrder, const IRosterIndex *AIndex) const
{
	QList<quint32> labels;
	if (AOrder == RLHO_ROSTERSVIEW)
	{
		if (!AIndex->data(RDR_STATUS).toString().isEmpty())
		{
			if (AIndex->kind()==RIK_STREAM_ROOT && AIndex->data(RDR_SHOW).toInt()==IPresence::Error)
				labels.append(RLID_ROSTERSVIEW_STATUS);
			else if (FShowStatus)
				labels.append(RLID_ROSTERSVIEW_STATUS);
		}

		if (AIndex->data(RDR_RESOURCES).toStringList().count()>1)
		{
			labels.append(RLID_ROSTERSVIEW_RESOURCES);
		}

		if (FRostersModel)
		{
			if (AIndex->parentIndex() == FRostersModel->rootIndex())
				labels.append(AdvancedDelegateItem::DisplayId);
			else if (FRostersModel->isGroupKind(AIndex->kind()))
				labels.append(AdvancedDelegateItem::DisplayId);
		}
	}
	return labels;
}

AdvancedDelegateItem RostersViewPlugin::rosterLabel(int AOrder, quint32 ALabelId, const IRosterIndex *AIndex) const
{
	if (AOrder == RLHO_ROSTERSVIEW)
	{
		if (ALabelId == AdvancedDelegateItem::DisplayId)
		{
			AdvancedDelegateItem displayLabel;
			displayLabel.d->id = AdvancedDelegateItem::DisplayId;
			displayLabel.d->kind = AdvancedDelegateItem::Display;
			displayLabel.d->data = AIndex->data(Qt::DisplayRole);
			if (AIndex->parentIndex() == FRostersModel->rootIndex())
				displayLabel.d->hints.insert(AdvancedDelegateItem::FontWeight,QFont::Bold);
			else if (FRostersModel->isGroupKind(AIndex->kind()))
				displayLabel.d->hints.insert(AdvancedDelegateItem::FontWeight,QFont::DemiBold);
			return displayLabel;
		}
		else if (ALabelId == RLID_ROSTERSVIEW_RESOURCES)
		{
			AdvancedDelegateItem resourceLabel;
			resourceLabel.d->id = ALabelId;
			resourceLabel.d->kind = AdvancedDelegateItem::CustomData;
			resourceLabel.d->data = QString("(%1)").arg(AIndex->data(RDR_RESOURCES).toStringList().count());
			resourceLabel.d->hints.insert(AdvancedDelegateItem::FontSizeDelta,-1);
			resourceLabel.d->hints.insert(AdvancedDelegateItem::Foreground,FRostersView->palette().color(QPalette::Disabled, QPalette::Text));
			return resourceLabel;
		}
		else if (ALabelId == RLID_ROSTERSVIEW_STATUS)
		{
			AdvancedDelegateItem statusLabel;
			statusLabel.d->id = RLID_ROSTERSVIEW_STATUS;
			statusLabel.d->kind = AdvancedDelegateItem::CustomData;
			statusLabel.d->data = AIndex->data(RDR_STATUS).toString();
			statusLabel.d->hints.insert(AdvancedDelegateItem::FontSizeDelta,-1);
			statusLabel.d->hints.insert(AdvancedDelegateItem::FontStyle,QFont::StyleItalic);
			return statusLabel;
		}
	}
	return AdvancedDelegateItem();
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

void RostersViewPlugin::registerExpandableRosterIndexKind(int AKind, int AUniqueRole, bool ADefaultExpanded)
{
	if (!FExpandableKinds.contains(AKind))
	{
		LOG_DEBUG(QString("Expandable roster index registered, kind=%1, role=%2, default=%3").arg(AKind).arg(AUniqueRole).arg(ADefaultExpanded));
		FExpandableKinds.insert(AKind,AUniqueRole);
		FExpandableDefaults.insert(AKind,ADefaultExpanded);
	}
}

QString RostersViewPlugin::rootExpandId(const QModelIndex &AIndex) const
{
	QModelIndex index = AIndex;
	while (index.parent().isValid())
		index = index.parent();
	return indexExpandId(index);
}

QString RostersViewPlugin::indexExpandId(const QModelIndex &AIndex) const
{
	int role = FExpandableKinds.value(AIndex.data(RDR_KIND).toInt());
	return role>0 ? AIndex.data(role).toString() : QString::null;
}

void RostersViewPlugin::loadExpandState(const QModelIndex &AIndex)
{
	QString indexId = indexExpandId(AIndex);
	if (!indexId.isEmpty())
	{
		QString rootId = rootExpandId(AIndex);
		bool defExpanded = FExpandableDefaults.value(AIndex.data(RDR_KIND).toInt(),true);
		bool isExpanded = FExpandStates.value(rootId).value(indexId,defExpanded);
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
			bool expanded = FRostersView->isExpanded(AIndex);
			bool defExpanded = FExpandableDefaults.value(AIndex.data(RDR_KIND).toInt(),true);
			if (expanded != defExpanded)
				FExpandStates[rootId][indexId] = expanded;
			else
				FExpandStates[rootId].remove(indexId);
		}
	}
}

void RostersViewPlugin::onViewDestroyed(QObject *AObject)
{
	Q_UNUSED(AObject);
	FRostersView = NULL;
}

void RostersViewPlugin::onViewModelAboutToBeReset()
{
	if (FRostersView->currentIndex().isValid())
	{
		FViewSavedState.currentIndex = FRostersView->rostersModel()->rosterIndexFromModelIndex(FRostersView->mapToModel(FRostersView->currentIndex()));
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
		FRostersView->setCurrentIndex(FRostersView->mapFromModel(FRostersView->rostersModel()->modelIndexFromRosterIndex(FViewSavedState.currentIndex)));
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

void RostersViewPlugin::onRostersViewIndexContextMenuAboutToShow()
{
	Menu *menu = qobject_cast<Menu *>(sender());
	if (menu)
	{
		QSet<Action *> streamsActions = FStreamsContextMenuActions.take(menu);
		QSet<Action *> rootActions = menu->groupActions().toSet() - streamsActions;
		foreach(Action *streamAction, streamsActions)
		{
			foreach(Action *rootAction, rootActions)
				if (streamAction->text() == rootAction->text())
					streamAction->setVisible(false);
		}
	}
	FStreamsContextMenuActions.clear();
}

void RostersViewPlugin::onRostersModelIndexDataChanged(IRosterIndex *AIndex, int ARole)
{
	if (ARole == RDR_SHOW)
	{
		if (AIndex->kind() == RIK_STREAM_ROOT)
			emit rosterDataChanged(AIndex,RDR_FORCE_VISIBLE);
	}
	else if (ARole == RDR_STATUS)
	{
		if (!FShowStatus)
		{
			if (AIndex->kind()==RIK_STREAM_ROOT && AIndex->data(RDR_SHOW).toInt()==IPresence::Error)
				emit rosterLabelChanged(RLID_ROSTERSVIEW_STATUS,AIndex);
		}
		else
		{
			emit rosterLabelChanged(RLID_ROSTERSVIEW_STATUS,AIndex);
		}
	}
	else if (ARole == RDR_RESOURCES)
	{
		emit rosterLabelChanged(RLID_ROSTERSVIEW_RESOURCES,AIndex);
	}
}

void RostersViewPlugin::onRostersViewClipboardMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu)
{
	if (ALabelId == AdvancedDelegateItem::DisplayId)
	{
		foreach(IRosterIndex *index, AIndexes)
		{
			QString name = index->data(RDR_NAME).toString().trimmed();
			if (!name.isEmpty())
			{
				Action *nameAction = new Action(AMenu);
				nameAction->setText(TextManager::getElidedString(name,Qt::ElideRight,50));
				nameAction->setData(ADR_CLIPBOARD_DATA,name);
				connect(nameAction,SIGNAL(triggered(bool)),SLOT(onCopyToClipboardActionTriggered(bool)));
				AMenu->addAction(nameAction, AG_RVCBM_NAME, true);
			}

			Jid jid = index->data(RDR_FULL_JID).toString();
			if (!jid.isEmpty())
			{
				Action *bareJidAction = new Action(AMenu);
				bareJidAction->setText(jid.uBare());
				bareJidAction->setData(ADR_CLIPBOARD_DATA, jid.uBare());
				connect(bareJidAction,SIGNAL(triggered(bool)),SLOT(onCopyToClipboardActionTriggered(bool)));
				AMenu->addAction(bareJidAction, AG_RVCBM_JABBERID, true);
			}

			QStringList resources = index->data(RDR_RESOURCES).toStringList();
			IPresence *presence = FPresenceManager!=NULL ? FPresenceManager->findPresence(index->data(RDR_STREAM_JID).toString()) : NULL;
			foreach(const QString &resource, resources)
			{
				IPresenceItem pitem =presence!=NULL ? presence->findItem(resource) : IPresenceItem();
				if (!pitem.isNull())
				{
					if (!pitem.itemJid.resource().isEmpty())
					{
						Action *fullJidAction = new Action(AMenu);
						fullJidAction->setText(pitem.itemJid.uFull());
						fullJidAction->setData(ADR_CLIPBOARD_DATA, pitem.itemJid.uFull());
						connect(fullJidAction,SIGNAL(triggered(bool)),SLOT(onCopyToClipboardActionTriggered(bool)));
						AMenu->addAction(fullJidAction, AG_RVCBM_JABBERID, true);
					}

					if (!pitem.status.isEmpty())
					{
						Action *statusAction = new Action(AMenu);
						statusAction->setText(TextManager::getElidedString(pitem.status,Qt::ElideRight,50));
						statusAction->setData(ADR_CLIPBOARD_DATA,pitem.status);
						connect(statusAction,SIGNAL(triggered(bool)),SLOT(onCopyToClipboardActionTriggered(bool)));
						AMenu->addAction(statusAction, AG_RVCBM_STATUS, true);
					}
				}
			}

			if (index->kind() == RIK_CONTACTS_ROOT)
			{
				QList<IRosterIndex *> streamIndexes;
				foreach(const Jid &streamJid, FRostersView->rostersModel()->streams())
					streamIndexes.append(FRostersView->rostersModel()->streamIndex(streamJid));
				FRostersView->clipboardMenuForIndex(streamIndexes,NULL,AMenu);
			}
		}
	}
}

void RostersViewPlugin::onRostersViewIndexToolTips(IRosterIndex *AIndex, quint32 ALabelId, QMap<int, QString> &AToolTips)
{
	if (ALabelId == AdvancedDelegateItem::DisplayId)
	{
		Jid streamJid = AIndex->data(RDR_STREAM_JID).toString();
		Jid contactJid = AIndex->data(RDR_FULL_JID).toString();
		QStringList resources = AIndex->data(RDR_RESOURCES).toStringList();

		if (AIndex->kind() == RIK_CONTACTS_ROOT)
		{
			QStringList avatarsToolTips;
			QStringList streamsToolTips;
			QList<Jid> streams = FRostersModel!=NULL ? FRostersModel->streams() : QList<Jid>();
			foreach(const Jid &streamJid, streams)
			{
				QMap<int, QString> toolTips;
				FRostersView->toolTipsForIndex(FRostersView->rostersModel()->streamIndex(streamJid),NULL,toolTips);
				if (!toolTips.isEmpty())
				{
					QString avatarToolTip = toolTips.take(RTTO_AVATAR_IMAGE);
					if (!avatarToolTip.isEmpty() && !avatarsToolTips.contains(avatarToolTip))
						avatarsToolTips.append(avatarToolTip);

					for (int resIndex=0; resIndex<10; resIndex++)
					{
						toolTips.remove(RTTO_ROSTERSVIEW_RESOURCE_TOPLINE+(resIndex*100));
						toolTips.remove(RTTO_ROSTERSVIEW_RESOURCE_BOTTOMLINE+(resIndex*100));
					}

					QString tooltip = QString("<span>%1</span>").arg(QStringList(toolTips.values()).join("<p/><nbsp>"));
					streamsToolTips.append(tooltip);
				}
			}

			if (!avatarsToolTips.isEmpty())
				AToolTips.insert(RTTO_AVATAR_IMAGE,QString("<span>%1</span>").arg(avatarsToolTips.join("<nbsp>")));
			if (!streamsToolTips.isEmpty())
				AToolTips.insert(RTTO_ROSTERSVIEW_INFO_STREAMS,QString("<span>%1</span>").arg(streamsToolTips.join("<hr><p/><nbsp>")));
		}
		else
		{
			QString name = AIndex->data(RDR_NAME).toString();
			if (!name.isEmpty())
				AToolTips.insert(RTTO_ROSTERSVIEW_INFO_NAME,"<big><b>" + name.toHtmlEscaped() + "</b></big>");

			if (streamJid.isValid() && AIndex->kind()!=RIK_STREAM_ROOT && FRostersModel && FRostersModel->streamsLayout()==IRostersModel::LayoutMerged)
			{
				IAccount *account = FAccountManager!=NULL ? FAccountManager->findAccountByStream(streamJid) : NULL;
				AToolTips.insert(RTTO_ROSTERSVIEW_INFO_ACCOUNT,tr("<b>Account:</b> %1").arg(account!=NULL ? account->name().toHtmlEscaped() : streamJid.uBare()));
			}

			if (!contactJid.isEmpty())
				AToolTips.insert(RTTO_ROSTERSVIEW_INFO_JABBERID,tr("<b>Jabber ID:</b> %1").arg(contactJid.uBare().toHtmlEscaped()));

			QString ask = AIndex->data(RDR_SUBSCRIPTION_ASK).toString();
			QString subscription = AIndex->data(RDR_SUBSCRIBTION).toString();
			if (!subscription.isEmpty())
			{
				QString subsName = tr("Absent");
				if (subscription == SUBSCRIPTION_BOTH)
					subsName = tr("Mutual");
				else if (subscription == SUBSCRIPTION_TO)
					subsName = tr("Provided to you");
				else if (subsName == SUBSCRIPTION_FROM)
					subsName = tr("Provided from you");

				if (ask == SUBSCRIPTION_SUBSCRIBE)
					AToolTips.insert(RTTO_ROSTERSVIEW_INFO_SUBCRIPTION,tr("<b>Subscription:</b> %1, request sent").arg(subsName.toHtmlEscaped()));
				else
					AToolTips.insert(RTTO_ROSTERSVIEW_INFO_SUBCRIPTION,tr("<b>Subscription:</b> %1").arg(subsName.toHtmlEscaped()));
			}
		}

		if (streamJid.isValid() && !resources.isEmpty())
		{
			AToolTips.insert(RTTO_ROSTERSVIEW_RESOURCE_TOPLINE,"<hr>");

			IPresence *presence = FPresenceManager!=NULL ? FPresenceManager->findPresence(AIndex->data(RDR_STREAM_JID).toString()) : NULL;
			for(int resIndex=0; resIndex<10 && resIndex<resources.count(); resIndex++)
			{
				int orderShift = resIndex*100;
				IPresenceItem pItem = presence!=NULL ? presence->findItem(resources.at(resIndex)) : IPresenceItem();
				if (!pItem.isNull())
				{
					QString resource = !pItem.itemJid.resource().isEmpty() ? pItem.itemJid.resource() : pItem.itemJid.uBare();
					QString statusIcon = FStatusIcons!=NULL ? FStatusIcons->iconFileName(streamJid,pItem.itemJid) : QString::null;
					AToolTips.insert(RTTO_ROSTERSVIEW_RESOURCE_NAME+orderShift,QString("<img src='%1'> %2 (%3)").arg(statusIcon).arg(resource.toHtmlEscaped()).arg(pItem.priority));

					if (!pItem.status.isEmpty())
						AToolTips.insert(RTTO_ROSTERSVIEW_RESOURCE_STATUS_TEXT+orderShift,pItem.status.toHtmlEscaped().replace('\n',"<br>"));

					if (resIndex < resources.count()-1)
						AToolTips.insert(RTTO_ROSTERSVIEW_RESOURCE_MIDDLELINE+orderShift,"<hr>");
				}
			}

			AToolTips.insert(RTTO_ROSTERSVIEW_RESOURCE_BOTTOMLINE,"<hr>");
		}
		else if (streamJid.isValid() && contactJid.isValid() && AIndex->data(RDR_SHOW).toInt()!=IPresence::Offline)
		{
			AToolTips.insert(RTTO_ROSTERSVIEW_RESOURCE_TOPLINE,"<hr>");

			int show = AIndex->data(RDR_SHOW).toInt();
			int priority = AIndex->data(RDR_PRIORITY).toInt();
			QString subscription = AIndex->data(RDR_SUBSCRIBTION).toString();
			bool subscription_ask = AIndex->data(RDR_SUBSCRIPTION_ASK).toString() == SUBSCRIPTION_SUBSCRIBE;
			QString resource = !contactJid.resource().isEmpty() ? contactJid.resource() : contactJid.uBare();

			QString statusIconSet = FStatusIcons!=NULL ? FStatusIcons->iconsetByJid(contactJid) : QString::null;
			QString statusIconKey = FStatusIcons!=NULL ? FStatusIcons->iconKeyByStatus(show,subscription,subscription_ask) : QString::null;
			QString statusIconFile = FStatusIcons!=NULL ? FStatusIcons->iconFileName(statusIconSet,statusIconKey) : QString::null;
			AToolTips.insert(RTTO_ROSTERSVIEW_RESOURCE_NAME,QString("<img src='%1'> %2 (%3)").arg(statusIconFile).arg(resource.toHtmlEscaped()).arg(priority));

			QString statusText = AIndex->data(RDR_STATUS).toString();
			if (!statusText.isEmpty())
				AToolTips.insert(RTTO_ROSTERSVIEW_RESOURCE_STATUS_TEXT,statusText.toHtmlEscaped().replace('\n',"<br>"));

			AToolTips.insert(RTTO_ROSTERSVIEW_RESOURCE_BOTTOMLINE,"<hr>");
		}
	}
}

void RostersViewPlugin::onRostersViewIndexContextMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu)
{
	if (AIndexes.count()==1 && AIndexes.first()->kind()==RIK_CONTACTS_ROOT && ALabelId==AdvancedDelegateItem::DisplayId)
	{
		QList<IRosterIndex *> streamIndexes;
		QStringList streams = AIndexes.first()->data(RDR_STREAMS).toStringList();
		foreach(const Jid &streamJid, streams)
		{
			IRosterIndex *sindex = FRostersView->rostersModel()->streamIndex(streamJid);
			streamIndexes.append(sindex);

			if (streams.count() > 1)
			{
				Menu *streamMenu = new Menu(AMenu);
				streamMenu->setIcon(sindex->data(Qt::DecorationRole).value<QIcon>());
				streamMenu->setTitle(sindex->data(Qt::DisplayRole).toString());

				FRostersView->contextMenuForIndex(QList<IRosterIndex *>()<<sindex,NULL,streamMenu);
				AMenu->addAction(streamMenu->menuAction(),AG_RVCM_ROSTERSVIEW_STREAMS,true);
			}
		}

		QSet<Action *> curActions = AMenu->groupActions().toSet();
		FRostersView->contextMenuForIndex(streamIndexes,NULL,AMenu);
		connect(AMenu,SIGNAL(aboutToShow()),SLOT(onRostersViewIndexContextMenuAboutToShow()));
		FStreamsContextMenuActions[AMenu] = AMenu->groupActions().toSet() - curActions;
	}
}

void RostersViewPlugin::onRestoreExpandState()
{
	restoreExpandState();
	FStartRestoreExpandState = false;
}

void RostersViewPlugin::onOptionsOpened()
{
	onOptionsChanged(Options::node(OPV_ROSTER_VIEWMODE));
	onOptionsChanged(Options::node(OPV_ROSTER_SORTMODE));
	onOptionsChanged(Options::node(OPV_ROSTER_SHOWOFFLINE));
	onOptionsChanged(Options::node(OPV_ROSTER_SHOWRESOURCE));
	onOptionsChanged(Options::node(OPV_ROSTER_HIDESCROLLBAR));
	onOptionsChanged(Options::node(OPV_ROSTER_MERGESTREAMS));
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
	else if (ANode.path() == OPV_ROSTER_SORTMODE)
	{
		FSortFilterProxyModel->invalidate();
	}
	else if (ANode.path() == OPV_ROSTER_HIDESCROLLBAR)
	{
		FRostersView->setVerticalScrollBarPolicy(ANode.value().toBool() ? Qt::ScrollBarAlwaysOff : Qt::ScrollBarAsNeeded);
		FRostersView->setHorizontalScrollBarPolicy(ANode.value().toBool() ? Qt::ScrollBarAlwaysOff : Qt::ScrollBarAsNeeded);
	}
	else if (ANode.path() == OPV_ROSTER_VIEWMODE)
	{
		FShowStatus = ANode.value().toInt() == IRostersView::ViewFull;
		emit rosterLabelChanged(RLID_ROSTERSVIEW_STATUS);
	}
	else if (ANode.path() == OPV_ROSTER_MERGESTREAMS)
	{
		if (FRostersView->rostersModel())
			FRostersView->rostersModel()->setStreamsLayout(ANode.value().toBool() ? IRostersModel::LayoutMerged : IRostersModel::LayoutSeparately);
	}
}

void RostersViewPlugin::onCopyToClipboardActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
		QApplication::clipboard()->setText(action->data(ADR_CLIPBOARD_DATA).toString());
}

void RostersViewPlugin::onShowOfflineContactsAction(bool)
{
	OptionsNode node = Options::node(OPV_ROSTER_SHOWOFFLINE);
	node.setValue(!node.value().toBool());
}

