#include "rostersearch.h"

RosterSearch::RosterSearch()
{
	FRostersViewPlugin = NULL;
	FMainWindow = NULL;

	FSearchEdit = NULL;
	FFieldsMenu = NULL;
	FSearchToolBarChanger = NULL;

	FEditTimeout.setSingleShot(true);
	FEditTimeout.setInterval(1000);
	connect(&FEditTimeout,SIGNAL(timeout()),SLOT(onEditTimedOut()));

	setDynamicSortFilter(false);
	setFilterCaseSensitivity(Qt::CaseInsensitive);

	FEnableAction = new Action(this);
	FEnableAction->setIcon(RSR_STORAGE_MENUICONS,MNI_ROSTERSEARCH_MENU);
	FEnableAction->setToolTip(tr("Show search toolbar"));
	FEnableAction->setCheckable(true);
	FEnableAction->setChecked(false);
	connect(FEnableAction,SIGNAL(triggered(bool)),SLOT(onEnableActionTriggered(bool)));

	QToolBar *searchToolBar = new QToolBar(tr("Search toolbar"));
	searchToolBar->setAllowedAreas(Qt::TopToolBarArea);
	searchToolBar->setMovable(false);
	FSearchToolBarChanger = new ToolBarChanger(searchToolBar);
	FSearchToolBarChanger->setManageVisibility(false);
	FSearchToolBarChanger->setSeparatorsVisible(false);
	FSearchToolBarChanger->toolBar()->setVisible(false);

	FFieldsMenu = new Menu(searchToolBar);
	FFieldsMenu->setIcon(RSR_STORAGE_MENUICONS,MNI_ROSTERSEARCH_MENU);
	FSearchToolBarChanger->insertAction(FFieldsMenu->menuAction());

	FSearchEdit = new QLineEdit(searchToolBar);
	FSearchEdit->setToolTip(tr("Search by regular expression"));
	connect(FSearchEdit,SIGNAL(textChanged(const QString &)),&FEditTimeout,SLOT(start()));
	FSearchToolBarChanger->insertWidget(FSearchEdit);
}

RosterSearch::~RosterSearch()
{

}

void RosterSearch::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Roster Search");
	APluginInfo->description = tr("Allows to search for contacts in the roster");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(ROSTERSVIEW_UUID);
	APluginInfo->dependences.append(MAINWINDOW_UUID);
}

bool RosterSearch::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
	IPlugin *plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
	if (plugin)
		FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IMainWindowPlugin").value(0,NULL);
	if (plugin)
	{
		IMainWindowPlugin *mainWindowPlugin = qobject_cast<IMainWindowPlugin *>(plugin->instance());
		if (mainWindowPlugin)
		{
			FMainWindow = mainWindowPlugin->mainWindow();
		}
	}

	connect(Options::instance(),SIGNAL(optionsOpened()),SLOT(onOptionsOpened()));
	connect(Options::instance(),SIGNAL(optionsClosed()),SLOT(onOptionsClosed()));

	return FRostersViewPlugin!=NULL && FMainWindow!=NULL;
}

bool RosterSearch::initObjects()
{
	if (FMainWindow)
	{
		FMainWindow->topToolBarChanger()->insertAction(FEnableAction,TBG_MWTTB_ROSTERSEARCH);

		FMainWindow->instance()->addToolBar(FSearchToolBarChanger->toolBar());
		FMainWindow->instance()->insertToolBarBreak(FSearchToolBarChanger->toolBar());
	}

	insertSearchField(RDR_NAME,tr("Name"));
	insertSearchField(RDR_STATUS,tr("Status"));
	insertSearchField(RDR_JID,tr("Jabber ID"));
	insertSearchField(RDR_GROUP,tr("Group"));

	return true;
}

bool RosterSearch::initSettings()
{
	Options::setDefaultValue(OPV_ROSTER_SEARCH_ENABLED,false);
	Options::setDefaultValue(OPV_ROSTER_SEARCH_FIELDEBANLED,true);
	return true;
}

void RosterSearch::startSearch()
{
	setFilterRegExp(FSearchEdit->text());
	invalidate();
	if (FRostersViewPlugin)
		FRostersViewPlugin->restoreExpandState();
	emit searchResultUpdated();
}

QString RosterSearch::searchPattern() const
{
	return filterRegExp().pattern();
}

void RosterSearch::setSearchPattern(const QString &APattern)
{
	FSearchEdit->setText(APattern);
	emit searchPatternChanged(APattern);
}

bool RosterSearch::isSearchEnabled() const
{
	return FEnableAction->isChecked();
}

void RosterSearch::setSearchEnabled(bool AEnabled)
{
	FEnableAction->setChecked(AEnabled);
	if (FRostersViewPlugin)
	{
		if (AEnabled)
			FRostersViewPlugin->rostersView()->insertProxyModel(this,RPO_ROSTERSEARCH_FILTER);
		else
			FRostersViewPlugin->rostersView()->removeProxyModel(this);
	}
	FSearchToolBarChanger->toolBar()->setVisible(AEnabled);
	emit searchStateChanged(AEnabled);
}

void RosterSearch::insertSearchField(int ADataRole, const QString &AName)
{
	Action *action = FFieldActions.value(ADataRole,NULL);
	if (action == NULL)
	{
		action = new Action(FFieldsMenu);
		action->setData(Action::DR_SortString,QString("%1").arg(ADataRole,5,10,QChar('0')));
		connect(action,SIGNAL(triggered(bool)),SLOT(onFieldActionTriggered(bool)));
		FFieldActions.insert(ADataRole,action);
		FFieldsMenu->addAction(action,AG_DEFAULT,true);
	}
	action->setText(AName);
	action->setCheckable(true);
	action->setChecked(true);
	emit searchFieldInserted(ADataRole,AName);
}

Menu *RosterSearch::searchFieldsMenu() const
{
	return FFieldsMenu;
}

QList<int> RosterSearch::searchFields() const
{
	return FFieldActions.keys();
}

bool RosterSearch::isSearchFieldEnabled(int ADataRole) const
{
	return FFieldActions.contains(ADataRole) && FFieldActions.value(ADataRole)->isChecked();
}

void RosterSearch::setSearchFieldEnabled(int ADataRole, bool AEnabled)
{
	if (FFieldActions.contains(ADataRole))
	{
		FFieldActions.value(ADataRole)->setChecked(AEnabled);
		emit searchFieldChanged(ADataRole);
	}
}

void RosterSearch::removeSearchField(int ADataRole)
{
	if (FFieldActions.contains(ADataRole))
	{
		Action *action = FFieldActions.take(ADataRole);
		FFieldsMenu->removeAction(action);
		delete action;
		emit searchFieldRemoved(ADataRole);
	}
}

bool RosterSearch::filterAcceptsRow(int ARow, const QModelIndex &AParent) const
{
	if (!searchPattern().isEmpty())
	{
		QModelIndex index = sourceModel()!=NULL ? sourceModel()->index(ARow,0,AParent) : QModelIndex();
		switch (index.data(RDR_TYPE).toInt())
		{
		case RIT_CONTACT:
		case RIT_AGENT:
		case RIT_MY_RESOURCE:
		{
			bool accept = true;
			foreach(int dataField, FFieldActions.keys())
			{
				if (isSearchFieldEnabled(dataField))
				{
					accept = false;
					if (filterRegExp().indexIn(index.data(dataField).toString())>=0)
						return true;
				}
			}
			return accept;
		}
		case RIT_GROUP:
		case RIT_GROUP_AGENTS:
		case RIT_GROUP_BLANK:
		case RIT_GROUP_NOT_IN_ROSTER:
		{
			for (int childRow = 0; index.child(childRow,0).isValid(); childRow++)
				if (filterAcceptsRow(childRow,index))
					return true;
			return false;
		}
		}
	}
	return true;
}

void RosterSearch::onFieldActionTriggered(bool)
{
	startSearch();
}

void RosterSearch::onEnableActionTriggered(bool AChecked)
{
	setSearchEnabled(AChecked);
}

void RosterSearch::onEditTimedOut()
{
	emit searchPatternChanged(FSearchEdit->text());
	startSearch();
}

void RosterSearch::onOptionsOpened()
{
	setSearchEnabled(Options::node(OPV_ROSTER_SEARCH_ENABLED).value().toBool());
	foreach(int dataRole, FFieldActions.keys())
		setSearchFieldEnabled(dataRole,Options::node(OPV_ROSTER_SEARCH_FIELDEBANLED,QString::number(dataRole)).value().toBool());
}

void RosterSearch::onOptionsClosed()
{
	Options::node(OPV_ROSTER_SEARCH_ENABLED).setValue(isSearchEnabled());
	foreach(int dataRole, FFieldActions.keys())
		Options::node(OPV_ROSTER_SEARCH_FIELDEBANLED,QString::number(dataRole)).setValue(isSearchFieldEnabled(dataRole));
}

Q_EXPORT_PLUGIN2(plg_rostersearch, RosterSearch)
