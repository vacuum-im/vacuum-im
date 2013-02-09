#ifndef ROSTERSVIEWPLUGIN_H
#define ROSTERSVIEWPLUGIN_H

#include <definitions/optionvalues.h>
#include <definitions/optionnodes.h>
#include <definitions/optionnodeorders.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/actiongroups.h>
#include <definitions/toolbargroups.h>
#include <definitions/mainwindowtabpages.h>
#include <definitions/rosterindexkinds.h>
#include <definitions/rosterindexroles.h>
#include <definitions/rosterproxyorders.h>
#include <definitions/rosterdataholderorders.h>
#include <definitions/rosterlabelholderorders.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/shortcuts.h>
#include <definitions/shortcutgrouporders.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/irostersview.h>
#include <interfaces/iroster.h>
#include <interfaces/imainwindow.h>
#include <interfaces/irostersmodel.h>
#include <interfaces/iaccountmanager.h>
#include <interfaces/ioptionsmanager.h>
#include <utils/action.h>
#include <utils/options.h>
#include <utils/shortcuts.h>
#include "rostersview.h"
#include "sortfilterproxymodel.h"

class RostersViewPlugin :
	public QObject,
	public IPlugin,
	public IRostersViewPlugin,
	public IOptionsHolder,
	public IRosterDataHolder
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IRostersViewPlugin IOptionsHolder IRosterDataHolder);
public:
	RostersViewPlugin();
	~RostersViewPlugin();
	virtual QObject *instance() { return this; }
	//IPlugin
	virtual QUuid pluginUuid() const { return ROSTERSVIEW_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin() { return true; }
	//IOptionsHolder
	virtual QMultiMap<int, IOptionsWidget *> optionsWidgets(const QString &ANodeId, QWidget *AParent);
	//IRosterDataHolder
	virtual QList<int> rosterDataRoles(int AOrder) const;
	virtual QVariant rosterData(int AOrder, const IRosterIndex *AIndex, int ARole) const;
	virtual bool setRosterData(int AOrder, const QVariant &AValue, IRosterIndex *AIndex, int ARole);
	//IRostersViewPlugin
	virtual IRostersView *rostersView();
	virtual void startRestoreExpandState();
	virtual void restoreExpandState(const QModelIndex &AParent = QModelIndex());
	virtual void registerExpandableRosterIndexKind(int AKind, int AUniqueRole);
signals:
	//IRosterDataHolder
	void rosterDataChanged(IRosterIndex *AIndex, int ARole);
protected:
	QString rootExpandId(const QModelIndex &AIndex) const;
	QString indexExpandId(const QModelIndex &AIndex) const;
	void loadExpandState(const QModelIndex &AIndex);
	void saveExpandState(const QModelIndex &AIndex);
protected slots:
	void onRostersViewDestroyed(QObject *AObject);
	void onViewModelAboutToBeReset();
	void onViewModelReset();
	void onViewModelAboutToBeChanged(QAbstractItemModel *AModel);
	void onViewModelChanged(QAbstractItemModel *AModel);
	void onViewRowsInserted(const QModelIndex &AParent, int AStart, int AEnd);
	void onViewRowsAboutToBeRemoved(const QModelIndex &AParent, int AStart, int AEnd);
	void onViewIndexCollapsed(const QModelIndex &AIndex);
	void onViewIndexExpanded(const QModelIndex &AIndex);
	void onRestoreExpandState();
	void onOptionsOpened();
	void onOptionsChanged(const OptionsNode &ANode);
	void onShowOfflineContactsAction(bool AChecked);
private:
	IRostersModel *FRostersModel;
	IMainWindowPlugin *FMainWindowPlugin;
	IOptionsManager *FOptionsManager;
private:
	bool FStartRestoreExpandState;
	QMap<int, int> FExpandableKinds;
	QMap<QString, QHash<QString,bool> > FExpandStates;
private:
	bool FShowResource;
	Action *FShowOfflineAction;
	RostersView *FRostersView;
	QAbstractItemModel *FLastModel;
	SortFilterProxyModel *FSortFilterProxyModel;
	struct { int sliderPos; IRosterIndex *currentIndex; } FViewSavedState;
};

#endif // ROSTERSVIEWPLUGIN_H
