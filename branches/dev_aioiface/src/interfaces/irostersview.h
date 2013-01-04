#ifndef IROSTERSVIEW_H
#define IROSTERSVIEW_H

#include <QMap>
#include <QVariant>
#include <QTreeView>
#include <QStringList>
#include <QAbstractProxyModel>
#include <interfaces/irostersmodel.h>
#include <utils/menu.h>
#include <utils/advanceditemdelegate.h>

#define ROSTERSVIEW_UUID "{BDD12B32-9C88-4e3c-9B36-2DCB5075288F}"

struct IRostersNotify
{
	enum Flags {
		Blink          = 0x01,
		AllwaysVisible = 0x02,
		ExpandParents  = 0x04,
		HookClicks     = 0x08
	};
	IRostersNotify() {
		order = -1;
		flags = 0;
		timeout = 0;
	}
	int order;
	int flags;
	int timeout;
	QIcon icon;
	QString footer;
	QBrush background;
};

class IRostersLabelHolder
{
public:
	virtual QObject *instance() = 0;
	virtual QList<quint32> rosterLabels(int AOrder, const IRosterIndex *AIndex) const =0;
	virtual AdvancedDelegateItem rosterLabel(int AOrder, quint32 ALabelId, const IRosterIndex *AIndex) const =0;
protected:
	virtual void rosterLabelChanged(quint32 ALabelId, IRosterIndex *AIndex = NULL) =0;
};

class IRostersClickHooker
{
public:
	virtual bool rosterIndexSingleClicked(int AOrder, IRosterIndex *AIndex, const QMouseEvent *AEvent) =0;
	virtual bool rosterIndexDoubleClicked(int AOrder, IRosterIndex *AIndex, const QMouseEvent *AEvent) =0;
};

class IRostersKeyHooker
{
public:
	virtual bool rosterKeyPressed(int AOrder, const QList<IRosterIndex *> &AIndexes, const QKeyEvent *AEvent) =0;
	virtual bool rosterKeyReleased(int AOrder, const QList<IRosterIndex *> &AIndexes, const QKeyEvent *AEvent) =0;
};

class IRostersDragDropHandler
{
public:
	virtual Qt::DropActions rosterDragStart(const QMouseEvent *AEvent, IRosterIndex *AIndex, QDrag *ADrag) =0;
	virtual bool rosterDragEnter(const QDragEnterEvent *AEvent) =0;
	virtual bool rosterDragMove(const QDragMoveEvent *AEvent, IRosterIndex *AHover) =0;
	virtual void rosterDragLeave(const QDragLeaveEvent *AEvent) =0;
	virtual bool rosterDropAction(const QDropEvent *AEvent, IRosterIndex *AIndex, Menu *AMenu) =0;
};

class IRostersEditHandler
{
public:
	virtual quint32 rosterEditLabel(int AOrder, int ADataRole, const QModelIndex &AIndex) const =0;
	virtual AdvancedDelegateEditProxy *rosterEditProxy(int AOrder, int ADataRole, const QModelIndex &AIndex) =0;
};

class IRostersView
{
public:
	//--RostersModel
	virtual QTreeView *instance() = 0;
	virtual IRostersModel *rostersModel() const =0;
	virtual void setRostersModel(IRostersModel *AModel) =0;
	virtual bool repaintRosterIndex(IRosterIndex *AIndex) =0;
	virtual void expandIndexParents(IRosterIndex *AIndex) =0;
	virtual void expandIndexParents(const QModelIndex &AIndex) =0;
	virtual bool editRosterIndex(IRosterIndex *AIndex, int ADataRole) =0;
	virtual bool singleClickOnIndex(IRosterIndex *AIndex, const QMouseEvent *AEvent) =0;
	virtual bool doubleClickOnIndex(IRosterIndex *AIndex, const QMouseEvent *AEvent) =0;
	virtual bool keyPressForIndex(const QList<IRosterIndex *> &AIndexes, const QKeyEvent *AEvent) =0;
	virtual bool keyReleaseForIndex(const QList<IRosterIndex *> &AIndexes, const QKeyEvent *AEvent) =0;
	virtual void toolTipsForIndex(IRosterIndex *AIndex, const QHelpEvent *AEvent, QMap<int,QString> &AToolTips) =0;
	virtual void contextMenuForIndex(const QList<IRosterIndex *> &AIndexes, const QContextMenuEvent *AEvent, Menu *AMenu) =0;
	virtual void clipboardMenuForIndex(const QList<IRosterIndex *> &AIndexes, const QContextMenuEvent *AEvent, Menu *AMenu) =0;
	//IndexSelection
	virtual bool hasMultiSelection() const =0;
	virtual bool isSelectionAcceptable(const QList<IRosterIndex *> &AIndexes) =0;
	virtual QList<IRosterIndex *> selectedRosterIndexes() const =0;
	virtual bool setSelectedRosterIndexes(const QList<IRosterIndex *> &AIndexes, bool APartial=true) =0;
	virtual QMap<int, QStringList > indexesRolesMap(const QList<IRosterIndex *> &AIndexes, const QList<int> &ARoles, int AUniqueRole=-1) const =0;
	//ProxyModels
	virtual void insertProxyModel(QAbstractProxyModel *AProxyModel, int AOrder) =0;
	virtual QList<QAbstractProxyModel *> proxyModels() const =0;
	virtual void removeProxyModel(QAbstractProxyModel *AProxyModel) =0;
	virtual QModelIndex mapToModel(const QModelIndex &AProxyIndex) const=0;
	virtual QModelIndex mapFromModel(const QModelIndex &AModelIndex) const=0;
	virtual QModelIndex mapToProxy(QAbstractProxyModel *AProxyModel, const QModelIndex &AModelIndex) const=0;
	virtual QModelIndex mapFromProxy(QAbstractProxyModel *AProxyModel, const QModelIndex &AProxyIndex) const=0;
	//IndexLabel
	virtual AdvancedDelegateItem registeredLabel(quint32 ALabelId) const =0;
	virtual quint32 registerLabel(const AdvancedDelegateItem &ALabel) =0;
	virtual void insertLabel(quint32 ALabelId, IRosterIndex *AIndex) =0;
	virtual void removeLabel(quint32 ALabelId, IRosterIndex *AIndex = NULL) =0;
	virtual quint32 labelAt(const QPoint &APoint, const QModelIndex &AIndex) const =0;
	virtual QRect labelRect(quint32 ALabeld, const QModelIndex &AIndex) const =0;
	//IndexNotify
	virtual int activeNotify(IRosterIndex *AIndex) const =0;
	virtual QList<int> notifyQueue(IRosterIndex *AIndex) const =0;
	virtual IRostersNotify notifyById(int ANotifyId) const =0;
	virtual QList<IRosterIndex *> notifyIndexes(int ANotifyId) const =0;
	virtual int insertNotify(const IRostersNotify &ANotify, const QList<IRosterIndex *> &AIndexes) =0;
	virtual void activateNotify(int ANotifyId) =0;
	virtual void removeNotify(int ANotifyId) =0;
	//DragDropHandlers
	virtual QList<IRostersDragDropHandler *> dragDropHandlers() const =0;
	virtual void insertDragDropHandler(IRostersDragDropHandler *AHandler) =0;
	virtual void removeDragDropHandler(IRostersDragDropHandler *AHandler) =0;
	//LabelHolders
	virtual QMultiMap<int, IRostersLabelHolder *> labelHolders() const =0;
	virtual void insertLabelHolder(int AOrder, IRostersLabelHolder *AHolder) =0;
	virtual void removeLabelHolder(int AOrder, IRostersLabelHolder *AHolder) =0;
	//ClickHookers
	virtual QMultiMap<int, IRostersClickHooker *> clickHookers() const =0;
	virtual void insertClickHooker(int AOrder, IRostersClickHooker *AHooker) =0;
	virtual void removeClickHooker(int AOrder, IRostersClickHooker *AHooker) =0;
	//KeyHookers
	virtual QMultiMap<int, IRostersKeyHooker *> keyHookers() const =0;
	virtual void insertKeyHooker(int AOrder, IRostersKeyHooker *AHooker) =0;
	virtual void removeKeyHooker(int AOrder, IRostersKeyHooker *AHooker) =0;
	//EditHandlers
	virtual QMultiMap<int, IRostersEditHandler *> editHandlers() const =0;
	virtual void insertEditHandler(int AOrder, IRostersEditHandler *AHandler) =0;
	virtual void removeEditHandler(int AOrder, IRostersEditHandler *AHandler) =0;
protected:
	virtual void modelAboutToBeSeted(IRostersModel *AIndex) =0;
	virtual void modelSeted(IRostersModel *AIndex) =0;
	virtual void proxyModelAboutToBeInserted(QAbstractProxyModel *AProxyModel, int AOrder) =0;
	virtual void proxyModelInserted(QAbstractProxyModel *AProxyModel) =0;
	virtual void proxyModelAboutToBeRemoved(QAbstractProxyModel *AProxyModel) =0;
	virtual void proxyModelRemoved(QAbstractProxyModel *AProxyModel) =0;
	virtual void viewModelAboutToBeChanged(QAbstractItemModel *AModel) =0;
	virtual void viewModelChanged(QAbstractItemModel *AModel) =0;
	virtual void indexMultiSelection(const QList<IRosterIndex *> &ASelected, bool &AAccepted) =0;
	virtual void indexContextMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu) =0;
	virtual void indexClipboardMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu) =0;
	virtual void indexToolTips(IRosterIndex *AIndex, quint32 ALabelId, QMap<int,QString> &AToolTips) =0;
	virtual void notifyInserted(int ANotifyId) =0;
	virtual void notifyActivated(int ANotifyId) =0;
	virtual void notifyRemoved(int ANotifyId) =0;
};

class IRostersViewPlugin
{
public:
	virtual QObject *instance() = 0;
	virtual IRostersView *rostersView() =0;
	virtual void startRestoreExpandState() =0;
	virtual void restoreExpandState(const QModelIndex &AParent = QModelIndex()) =0;
	virtual void registerExpandableRosterIndexType(int AType, int AUniqueRole) =0;
};

Q_DECLARE_INTERFACE(IRostersLabelHolder,"Vacuum.Plugin.IRostersLabelHolder/1.0");
Q_DECLARE_INTERFACE(IRostersClickHooker,"Vacuum.Plugin.IRostersClickHooker/1.3");
Q_DECLARE_INTERFACE(IRostersKeyHooker,"Vacuum.Plugin.IRostersKeyHooker/1.2");
Q_DECLARE_INTERFACE(IRostersDragDropHandler,"Vacuum.Plugin.IRostersDragDropHandler/1.1");
Q_DECLARE_INTERFACE(IRostersEditHandler,"Virtus.Plugin.IRostersEditHandler/1.1")
Q_DECLARE_INTERFACE(IRostersView,"Vacuum.Plugin.IRostersView/1.5");
Q_DECLARE_INTERFACE(IRostersViewPlugin,"Vacuum.Plugin.IRostersViewPlugin/1.5");

#endif //IROSTERSVIEW_H
