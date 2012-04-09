#include "discoitemswindow.h"

#include <QRegExp>
#include <QLineEdit>

bool SortFilterProxyModel::hasChildren( const QModelIndex &AParent ) const
{
	if (sourceModel() && sourceModel()->canFetchMore(mapToSource(AParent)))
		return sourceModel()->hasChildren(mapToSource(AParent));
	return QSortFilterProxyModel::hasChildren(AParent);
}

bool SortFilterProxyModel::filterAcceptsRow(int ARow, const QModelIndex &AParent) const
{
	bool accept = !AParent.isValid() || filterRegExp().isEmpty();
	if (!accept)
	{
		QModelIndex index = sourceModel()->index(ARow,0,AParent);
		for (int row=0; !accept && row<sourceModel()->rowCount(index); row++)
			accept = filterAcceptsRow(row,index);

		accept = accept || index.data(DIDR_NAME).toString().contains(filterRegExp());
		accept = accept || index.data(DIDR_JID).toString().contains(filterRegExp());
		accept = accept || index.data(DIDR_NODE).toString().contains(filterRegExp());
	}
	return accept;
}

DiscoItemsWindow::DiscoItemsWindow(IServiceDiscovery *ADiscovery, const Jid &AStreamJid, QWidget *AParent) : QMainWindow(AParent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose,true);
	setWindowTitle(tr("Service Discovery - %1").arg(AStreamJid.uFull()));
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this,MNI_SDISCOVERY_DISCOVER,0,0,"windowIcon");

	FDataForms = NULL;
	FVCardPlugin = NULL;
	FRosterChanger = NULL;

	FDiscovery = ADiscovery;
	FCurrentStep = -1;
	FStreamJid = AStreamJid;

	Action *closeAction = new Action(this);
	closeAction->setShortcutId(SCT_DISCOWINDOW_CLOSEWINDOW);
	connect(closeAction,SIGNAL(triggered()),SLOT(close()));
	addAction(closeAction);

	FToolBarChanger = new ToolBarChanger(ui.tlbToolBar);

	FActionsBarChanger = new ToolBarChanger(new QToolBar(this));
	FActionsBarChanger->setManageVisibility(false);
	FActionsBarChanger->setSeparatorsVisible(false);
	FActionsBarChanger->toolBar()->setIconSize(this->iconSize());
	FActionsBarChanger->toolBar()->setOrientation(Qt::Vertical);
	FActionsBarChanger->toolBar()->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	FActionsBarChanger->toolBar()->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

	ui.grbActions->setLayout(new QVBoxLayout);
	ui.grbActions->layout()->setMargin(2);
	ui.grbActions->layout()->addWidget(FActionsBarChanger->toolBar());

	connect(ui.cmbJid->lineEdit(),SIGNAL(returnPressed()),SLOT(onComboReturnPressed()));
	connect(ui.cmbNode->lineEdit(),SIGNAL(returnPressed()),SLOT(onComboReturnPressed()));

	FModel = new DiscoItemsModel(FDiscovery,FStreamJid,this);
	FProxy = new SortFilterProxyModel(FModel);
	FProxy->setSortCaseSensitivity(Qt::CaseInsensitive);
	FProxy->setSortLocaleAware(true);
	FProxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
	FProxy->setFilterKeyColumn(DiscoItemsModel::COL_NAME);
	FProxy->setSourceModel(FModel);
	ui.trvItems->setModel(FProxy);
	ui.trvItems->setSortingEnabled(true);

	FHeader = ui.trvItems->header();
	FHeader->setClickable(true);
	FHeader->setResizeMode(DiscoItemsModel::COL_NAME,QHeaderView::Interactive);
	FHeader->setResizeMode(DiscoItemsModel::COL_JID,QHeaderView::Interactive);
	FHeader->setResizeMode(DiscoItemsModel::COL_NODE,QHeaderView::Stretch);
	FHeader->setSortIndicator(DiscoItemsModel::COL_NAME,Qt::AscendingOrder);

	FSearchTimer.setSingleShot(true);
	FSearchTimer.setInterval(1000);
	connect(&FSearchTimer,SIGNAL(timeout()),SLOT(onSearchTimerTimeout()));
	connect(ui.lneSearch,SIGNAL(textChanged(const QString &)),&FSearchTimer,SLOT(start()));
	connect(ui.lneSearch,SIGNAL(editingFinished()),&FSearchTimer,SLOT(stop()));
	connect(ui.lneSearch,SIGNAL(editingFinished()),SLOT(onSearchTimerTimeout()));

	connect(ui.trvItems,SIGNAL(customContextMenuRequested(const QPoint &)),SLOT(onViewContextMenu(const QPoint &)));
	connect(ui.trvItems->selectionModel(),SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
	        SLOT(onCurrentIndexChanged(const QModelIndex &, const QModelIndex &)));

	connect(FDiscovery->instance(),SIGNAL(discoInfoReceived(const IDiscoInfo &)),SLOT(onDiscoInfoReceived(const IDiscoInfo &)));
	connect(FDiscovery->instance(),SIGNAL(discoItemsReceived(const IDiscoItems &)),SLOT(onDiscoItemsReceived(const IDiscoItems &)));

	initialize();
	createToolBarActions();

	if (!restoreGeometry(Options::fileValue("servicediscovery.itemswindow.geometry",FStreamJid.pBare()).toByteArray()))
		setGeometry(WidgetManager::alignGeometry(QSize(800,480),this));
	restoreState(Options::fileValue("servicediscovery.itemswindow.state",FStreamJid.pBare()).toByteArray());
	
	if (!FHeader->restoreState(Options::fileValue("servicediscovery.itemswindow.header-state",FStreamJid.pBare()).toByteArray()))
	{
		FHeader->resizeSection(DiscoItemsModel::COL_NAME,300);
		FHeader->resizeSection(DiscoItemsModel::COL_JID,200);
		FHeader->resizeSection(DiscoItemsModel::COL_NODE,100);
	}
}

DiscoItemsWindow::~DiscoItemsWindow()
{
	Options::setFileValue(saveState(),"servicediscovery.itemswindow.state",FStreamJid.pBare());
	Options::setFileValue(saveGeometry(),"servicediscovery.itemswindow.geometry",FStreamJid.pBare());
	Options::setFileValue(FHeader->saveState(),"servicediscovery.itemswindow.header-state",FStreamJid.pBare());

	emit windowDestroyed(this);
}

void DiscoItemsWindow::discover(const Jid AContactJid, const QString &ANode)
{
	ui.cmbJid->setEditText(AContactJid.uFull());
	ui.cmbNode->setEditText(ANode);

	while (FModel->rowCount() > 0)
		FModel->removeTopLevelItem(0);

	QPair<Jid,QString>step(AContactJid,ANode);
	if (step != FDiscoverySteps.value(FCurrentStep))
		FDiscoverySteps.insert(++FCurrentStep,step);

	if (ui.cmbJid->findText(ui.cmbJid->currentText()) < 0)
		ui.cmbJid->addItem(ui.cmbJid->currentText());
	if (ui.cmbNode->findText(ui.cmbNode->currentText()) < 0)
		ui.cmbNode->addItem(ui.cmbNode->currentText());

	FModel->appendTopLevelItem(AContactJid,ANode);
	ui.trvItems->expand(ui.trvItems->model()->index(0,0));
	ui.trvItems->setCurrentIndex(ui.trvItems->model()->index(0,0));

	emit discoverChanged(AContactJid,ANode);
}

void DiscoItemsWindow::initialize()
{
	IPlugin *plugin = FDiscovery->pluginManager()->pluginInterface("IRosterChanger").value(0,NULL);
	if (plugin)
		FRosterChanger = qobject_cast<IRosterChanger *>(plugin->instance());

	plugin = FDiscovery->pluginManager()->pluginInterface("IVCardPlugin").value(0,NULL);
	if (plugin)
		FVCardPlugin = qobject_cast<IVCardPlugin *>(plugin->instance());

	plugin = FDiscovery->pluginManager()->pluginInterface("IDataForms").value(0,NULL);
	if (plugin)
		FDataForms = qobject_cast<IDataForms *>(plugin->instance());
}

void DiscoItemsWindow::createToolBarActions()
{
	FMoveBack = new Action(FToolBarChanger);
	FMoveBack->setText(tr("Back"));
	FMoveBack->setIcon(RSR_STORAGE_MENUICONS,MNI_SDISCOVERY_ARROW_LEFT);
	FMoveBack->setShortcutId(SCT_DISCOWINDOW_BACK);
	FToolBarChanger->insertAction(FMoveBack,TBG_DIWT_DISCOVERY_NAVIGATE);
	connect(FMoveBack,SIGNAL(triggered(bool)),SLOT(onToolBarActionTriggered(bool)));

	FMoveForward = new Action(FToolBarChanger);
	FMoveForward->setText(tr("Forward"));
	FMoveForward->setIcon(RSR_STORAGE_MENUICONS,MNI_SDISCOVERY_ARROW_RIGHT);
	FMoveForward->setShortcutId(SCT_DISCOWINDOW_BACK);
	FToolBarChanger->insertAction(FMoveForward,TBG_DIWT_DISCOVERY_NAVIGATE);
	connect(FMoveForward,SIGNAL(triggered(bool)),SLOT(onToolBarActionTriggered(bool)));

	FDiscoverCurrent = new Action(FToolBarChanger);
	FDiscoverCurrent->setText(tr("Discover"));
	FDiscoverCurrent->setIcon(RSR_STORAGE_MENUICONS,MNI_SDISCOVERY_DISCOVER);
	FDiscoverCurrent->setShortcutId(SCT_DISCOWINDOW_DISCOVER);
	FToolBarChanger->insertAction(FDiscoverCurrent,TBG_DIWT_DISCOVERY_DEFACTIONS);
	connect(FDiscoverCurrent,SIGNAL(triggered(bool)),SLOT(onToolBarActionTriggered(bool)));

	FReloadCurrent = new Action(FToolBarChanger);
	FReloadCurrent->setText(tr("Reload"));
	FReloadCurrent->setIcon(RSR_STORAGE_MENUICONS,MNI_SDISCOVERY_RELOAD);
	FReloadCurrent->setShortcutId(SCT_DISCOWINDOW_RELOAD);
	FToolBarChanger->insertAction(FReloadCurrent,TBG_DIWT_DISCOVERY_DEFACTIONS);
	connect(FReloadCurrent,SIGNAL(triggered(bool)),SLOT(onToolBarActionTriggered(bool)));

	FDiscoInfo = new Action(FToolBarChanger);
	FDiscoInfo->setText(tr("Disco info"));
	FDiscoInfo->setIcon(RSR_STORAGE_MENUICONS,MNI_SDISCOVERY_DISCOINFO);
	FDiscoInfo->setShortcutId(SCT_DISCOWINDOW_SHOWDISCOINFO);
	FToolBarChanger->insertAction(FDiscoInfo,TBG_DIWT_DISCOVERY_ACTIONS);
	connect(FDiscoInfo,SIGNAL(triggered(bool)),SLOT(onToolBarActionTriggered(bool)));

	FAddContact = new Action(FToolBarChanger);
	FAddContact->setText(tr("Add Contact"));
	FAddContact->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_ADD_CONTACT);
	FAddContact->setShortcutId(SCT_DISCOWINDOW_ADDCONTACT);
	FToolBarChanger->insertAction(FAddContact,TBG_DIWT_DISCOVERY_ACTIONS);
	connect(FAddContact,SIGNAL(triggered(bool)),SLOT(onToolBarActionTriggered(bool)));

	FShowVCard = new Action(FToolBarChanger);
	FShowVCard->setText(tr("vCard"));
	FShowVCard->setIcon(RSR_STORAGE_MENUICONS,MNI_VCARD);
	FShowVCard->setShortcutId(SCT_DISCOWINDOW_SHOWVCARD);
	FToolBarChanger->insertAction(FShowVCard,TBG_DIWT_DISCOVERY_ACTIONS);
	connect(FShowVCard,SIGNAL(triggered(bool)),SLOT(onToolBarActionTriggered(bool)));

	updateToolBarActions();
}

void DiscoItemsWindow::updateToolBarActions()
{
	FMoveBack->setEnabled(FCurrentStep > 0);
	FMoveForward->setEnabled(FCurrentStep < FDiscoverySteps.count()-1);
	FDiscoverCurrent->setEnabled(ui.trvItems->currentIndex().isValid() && ui.trvItems->currentIndex().parent().isValid());
	FReloadCurrent->setEnabled(ui.trvItems->currentIndex().isValid());
	FDiscoInfo->setEnabled(ui.trvItems->currentIndex().isValid());
	FAddContact->setEnabled(FRosterChanger != NULL);
	FShowVCard->setEnabled(FVCardPlugin != NULL);
}

void DiscoItemsWindow::updateActionsBar()
{
	foreach(QAction *handle, FActionsBarChanger->groupItems(TBG_DIWT_DISCOVERY_FEATURE_ACTIONS))
	{
		delete FActionsBarChanger->handleAction(handle);
		FActionsBarChanger->removeItem(handle);
	}

	QModelIndex index = ui.trvItems->currentIndex();
	if (index.isValid())
	{
		IDiscoInfo dinfo = FDiscovery->discoInfo(FStreamJid,index.data(DIDR_JID).toString(),index.data(DIDR_NODE).toString());
		foreach(QString feature, dinfo.features)
		{
			foreach(Action *action, FDiscovery->createFeatureActions(FStreamJid,feature,dinfo,this))
			{
				QToolButton *button = FActionsBarChanger->insertAction(action,TBG_DIWT_DISCOVERY_FEATURE_ACTIONS);
				button->setPopupMode(QToolButton::InstantPopup);
				button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
				button->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
			}
		}
	}
}

void DiscoItemsWindow::onDiscoInfoReceived(const IDiscoInfo &ADiscoInfo)
{
	if (ADiscoInfo.streamJid==FStreamJid && ADiscoInfo.contactJid==ui.trvItems->currentIndex().data(DIDR_JID).toString())
		updateActionsBar();
}

void DiscoItemsWindow::onDiscoItemsReceived(const IDiscoItems &ADiscoItems)
{
	if (ADiscoItems.streamJid==FStreamJid && ADiscoItems.contactJid==ui.trvItems->currentIndex().data(DIDR_JID).toString())
		updateActionsBar();
}

void DiscoItemsWindow::onViewContextMenu(const QPoint &APos)
{
	QModelIndex index = ui.trvItems->indexAt(APos);
	if (index.isValid())
	{
		ui.trvItems->setCurrentIndex(index);
		Menu *menu = new Menu(this);
		menu->setAttribute(Qt::WA_DeleteOnClose,true);

		menu->addAction(FDiscoverCurrent,TBG_DIWT_DISCOVERY_DEFACTIONS,false);
		menu->addAction(FReloadCurrent,TBG_DIWT_DISCOVERY_DEFACTIONS,false);
		menu->addAction(FDiscoInfo,TBG_DIWT_DISCOVERY_ACTIONS,false);
		menu->addAction(FAddContact,TBG_DIWT_DISCOVERY_ACTIONS,false);
		menu->addAction(FShowVCard,TBG_DIWT_DISCOVERY_ACTIONS,false);

		IDiscoInfo dinfo = FDiscovery->discoInfo(FStreamJid,index.data(DIDR_JID).toString(),index.data(DIDR_NODE).toString());
		foreach(QString feature, dinfo.features)
		{
			foreach(Action *action, FDiscovery->createFeatureActions(FStreamJid,feature,dinfo,menu))
				menu->addAction(action,TBG_DIWT_DISCOVERY_FEATURE_ACTIONS,true);
		}
		emit indexContextMenu(index,menu);
		menu->popup(ui.trvItems->viewport()->mapToGlobal(APos));
	}
}

void DiscoItemsWindow::onCurrentIndexChanged(QModelIndex ACurrent, QModelIndex APrevious)
{
	if (ACurrent.parent()!=APrevious.parent() || ACurrent.row()!=APrevious.row())
	{
		FModel->fetchIndex(FProxy->mapToSource(ACurrent),true,false);
		updateToolBarActions();
		updateActionsBar();
		emit currentIndexChanged(ACurrent);
	}
}

void DiscoItemsWindow::onToolBarActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action == FMoveBack)
	{
		if (FCurrentStep > 0)
		{
			QPair<Jid,QString> step = FDiscoverySteps.at(--FCurrentStep);
			discover(step.first,step.second);
		}
	}
	else if (action == FMoveForward)
	{
		if (FCurrentStep < FDiscoverySteps.count()-1)
		{
			QPair<Jid,QString> step = FDiscoverySteps.at(++FCurrentStep);
			discover(step.first,step.second);
		}
	}
	else if (action == FDiscoverCurrent)
	{
		QModelIndex index = ui.trvItems->currentIndex();
		if (index.isValid() && index.parent().isValid())
		{
			Jid itemJid = index.data(DIDR_JID).toString();
			QString itemNode = index.data(DIDR_NODE).toString();
			discover(itemJid,itemNode);
		}
	}
	else if (action == FReloadCurrent)
	{
		FModel->loadIndex(FProxy->mapToSource(ui.trvItems->currentIndex()),true,true);
	}
	else if (action == FDiscoInfo)
	{
		QModelIndex index = ui.trvItems->currentIndex();
		if (index.isValid())
		{
			Jid itemJid = index.data(DIDR_JID).toString();
			QString itemNode = index.data(DIDR_NODE).toString();
			FDiscovery->showDiscoInfo(FStreamJid,itemJid,itemNode,this);
		}
	}
	else if (action == FAddContact)
	{
		QModelIndex index = ui.trvItems->currentIndex();
		if (index.isValid())
		{
			IAddContactDialog *dialog = FRosterChanger->showAddContactDialog(FStreamJid);
			if (dialog)
			{
				dialog->setContactJid(index.data(DIDR_JID).toString());
				dialog->setNickName(index.data(DIDR_NAME).toString());
			}
		}
	}
	else if (action == FShowVCard)
	{
		QModelIndex index = ui.trvItems->currentIndex();
		if (index.isValid())
		{
			Jid itemJid = index.data(DIDR_JID).toString();
			FVCardPlugin->showVCardDialog(FStreamJid,itemJid);
		}
	}
}

void DiscoItemsWindow::onComboReturnPressed()
{
	Jid itemJid = Jid::fromUserInput(ui.cmbJid->currentText().trimmed());
	QString itemNode = ui.cmbNode->currentText().trimmed();
	if (itemJid.isValid() && FDiscoverySteps.value(FCurrentStep) != qMakePair(itemJid,itemNode))
		discover(itemJid,itemNode);
}

void DiscoItemsWindow::onSearchTimerTimeout()
{
	FProxy->setFilterRegExp(ui.lneSearch->text());
}
