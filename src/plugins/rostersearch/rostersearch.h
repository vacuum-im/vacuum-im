#ifndef ROSTERSEARCH_H
#define ROSTERSEARCH_H

#include <QTimer>
#include <QLineEdit>
#include <QSortFilterProxyModel>
#include <definitions/actiongroups.h>
#include <definitions/toolbargroups.h>
#include <definitions/rosterindextyperole.h>
#include <definitions/rosterproxyorders.h>
#include <definitions/rosterclickhookerorders.h>
#include <definitions/rosterkeyhookerorders.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/optionvalues.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/irostersearch.h>
#include <interfaces/imainwindow.h>
#include <interfaces/irostersview.h>
#include <utils/action.h>
#include <utils/options.h>
#include <utils/toolbarchanger.h>

class RosterSearch :
			public QSortFilterProxyModel,
			public IPlugin,
			public IRosterSearch,
			public IRostersClickHooker,
			public IRostersKeyHooker
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IRosterSearch IRostersClickHooker IRostersKeyHooker);
public:
	RosterSearch();
	~RosterSearch();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return ROSTERSEARCH_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin() { return true; }
	//IRostersClickHooker
	virtual bool rosterIndexSingleClicked(int AOrder, IRosterIndex *AIndex, QMouseEvent *AEvent);
	virtual bool rosterIndexDoubleClicked(int AOrder, IRosterIndex *AIndex, QMouseEvent *AEvent);
	//IRostersKeyHooker
	virtual bool rosterKeyPressed(int AOrder, const QList<IRosterIndex *> &AIndexes, QKeyEvent *AEvent);
	virtual bool rosterKeyReleased(int AOrder, const QList<IRosterIndex *> &AIndexes, QKeyEvent *AEvent);
	//IRosterSearch
	virtual void startSearch();
	virtual QString searchPattern() const;
	virtual void setSearchPattern(const QString &APattern);
	virtual bool isSearchEnabled() const;
	virtual void setSearchEnabled(bool AEnabled);
	virtual void insertSearchField(int ADataRole, const QString &AName);
	virtual Menu *searchFieldsMenu() const;
	virtual QList<int> searchFields() const;
	virtual bool isSearchFieldEnabled(int ADataRole) const;
	virtual void setSearchFieldEnabled(int ADataRole, bool AEnabled);
	virtual void removeSearchField(int ADataRole);
signals:
	void searchResultUpdated();
	void searchStateChanged(bool AEnabled);
	void searchPatternChanged(const QString &APattern);
	void searchFieldInserted(int ADataRole, const QString &AName);
	void searchFieldChanged(int ADataRole);
	void searchFieldRemoved(int ADataRole);
protected:
	virtual bool filterAcceptsRow(int ARow, const QModelIndex &AParent) const;
protected slots:
	void onFieldActionTriggered(bool);
	void onEnableActionTriggered(bool AChecked);
	void onEditTimedOut();
	void onOptionsOpened();
	void onOptionsClosed();
private:
	IMainWindow *FMainWindow;
	IRostersViewPlugin *FRostersViewPlugin;
private:
	bool FSearchStarted;
	bool FLastShowOffline;
	Menu *FFieldsMenu;
	QTimer FEditTimeout;
	Action *FEnableAction;
	QLineEdit *FSearchEdit;
	ToolBarChanger *FSearchToolBarChanger;
	QMap<int,Action *> FFieldActions;
};

#endif // ROSTERSEARCH_H
