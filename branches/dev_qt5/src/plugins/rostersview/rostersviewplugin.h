#ifndef ROSTERSVIEWPLUGIN_H
#define ROSTERSVIEWPLUGIN_H

#include <interfaces/ipluginmanager.h>
#include <interfaces/irostersview.h>
#include <interfaces/iroster.h>
#include <interfaces/ipresence.h>
#include <interfaces/imainwindow.h>
#include <interfaces/irostersmodel.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/iaccountmanager.h>
#include <interfaces/istatusicons.h>
#include "rostersview.h"
#include "sortfilterproxymodel.h"

class RostersViewPlugin :
	public QObject,
	public IPlugin,
	public IRostersViewPlugin,
	public IOptionsHolder,
	public IRosterDataHolder,
	public IRostersLabelHolder
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IRostersViewPlugin IOptionsHolder IRosterDataHolder IRostersLabelHolder);
	Q_PLUGIN_METADATA(IID "org.jrudevels.vacuum.IRostersViewPlugin");
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
	//IRostersLabelHolder
	virtual QList<quint32> rosterLabels(int AOrder, const IRosterIndex *AIndex) const;
	virtual AdvancedDelegateItem rosterLabel(int AOrder, quint32 ALabelId, const IRosterIndex *AIndex) const;
	//IRostersViewPlugin
	virtual IRostersView *rostersView();
	virtual void startRestoreExpandState();
	virtual void restoreExpandState(const QModelIndex &AParent = QModelIndex());
	virtual void registerExpandableRosterIndexKind(int AKind, int AUniqueRole);
signals:
	//IRosterDataHolder
	void rosterDataChanged(IRosterIndex *AIndex, int ARole);
	//IRostersLabelHolder
	void rosterLabelChanged(quint32 ALabelId, IRosterIndex *AIndex = NULL);
protected:
	QString rootExpandId(const QModelIndex &AIndex) const;
	QString indexExpandId(const QModelIndex &AIndex) const;
	void loadExpandState(const QModelIndex &AIndex);
	void saveExpandState(const QModelIndex &AIndex);
protected slots:
	void onViewDestroyed(QObject *AObject);
	void onViewModelAboutToBeReset();
	void onViewModelReset();
	void onViewModelAboutToBeChanged(QAbstractItemModel *AModel);
	void onViewModelChanged(QAbstractItemModel *AModel);
	void onViewRowsInserted(const QModelIndex &AParent, int AStart, int AEnd);
	void onViewRowsAboutToBeRemoved(const QModelIndex &AParent, int AStart, int AEnd);
	void onViewIndexCollapsed(const QModelIndex &AIndex);
	void onViewIndexExpanded(const QModelIndex &AIndex);
protected slots:
	void onRostersViewIndexContextMenuAboutToShow();
	void onRostersModelIndexDataChanged(IRosterIndex *AIndex, int ARole);
	void onRostersViewClipboardMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu);
	void onRostersViewIndexToolTips(IRosterIndex *AIndex, quint32 ALabelId, QMap<int, QString> &AToolTips);
	void onRostersViewIndexContextMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu);
protected slots:
	void onRestoreExpandState();
	void onOptionsOpened();
	void onOptionsChanged(const OptionsNode &ANode);
	void onCopyToClipboardActionTriggered(bool);
	void onShowOfflineContactsAction(bool AChecked);
	void onShortcutActivated(const QString &AId, QWidget *AWidget);
private:
	IStatusIcons *FStatusIcons;
	IRostersModel *FRostersModel;
	IPresencePlugin *FPresencePlugin;
	IOptionsManager *FOptionsManager;
	IAccountManager *FAccountManager;
	IMainWindowPlugin *FMainWindowPlugin;
private:
	bool FStartRestoreExpandState;
	QMap<int, int> FExpandableKinds;
	QMap<QString, QHash<QString,bool> > FExpandStates;
private:
	bool FShowStatus;
	bool FShowResource;
	bool FShowMergedStreams;
	Action *FShowOfflineAction;
	RostersView *FRostersView;
	QAbstractItemModel *FLastModel;
	SortFilterProxyModel *FSortFilterProxyModel;
	QMap<Menu *, QSet<Action *> > FStreamsContextMenuActions;
	struct { int sliderPos; IRosterIndex *currentIndex; } FViewSavedState;
};

#endif // ROSTERSVIEWPLUGIN_H
