#ifndef ROSTERSVIEW_H
#define ROSTERSVIEW_H

#include <QTimer>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/actiongroups.h>
#include <definitions/optionvalues.h>
#include <definitions/shortcuts.h>
#include <definitions/rostertooltiporders.h>
#include <definitions/rosterlabelorders.h>
#include <definitions/rosterindextyperole.h>
#include <definitions/rosterfootertextorders.h>
#include <definitions/rosterdragdropmimetypes.h>
#include <interfaces/irostersview.h>
#include <interfaces/irostersmodel.h>
#include <utils/options.h>
#include <utils/shortcuts.h>
#include "rosterindexdelegate.h"

struct NotifyItem
{
	int notifyId;
	int order;
	int flags;
	QIcon icon;
	QString toolTip;
	QList<IRosterIndex *> indexes;
};

class RostersView :
			public QTreeView,
			public IRostersView
{
	Q_OBJECT;
	Q_INTERFACES(IRostersView);
public:
	RostersView(QWidget *AParent = NULL);
	~RostersView();
	virtual QTreeView *instance() { return this; }
	//IRostersView
	virtual IRostersModel *rostersModel() const { return FRostersModel; }
	virtual void setRostersModel(IRostersModel *AModel);
	virtual bool repaintRosterIndex(IRosterIndex *AIndex);
	virtual void expandIndexParents(IRosterIndex *AIndex);
	virtual void expandIndexParents(const QModelIndex &AIndex);
	//--ProxyModels
	virtual void insertProxyModel(QAbstractProxyModel *AProxyModel, int AOrder);
	virtual QList<QAbstractProxyModel *> proxyModels() const;
	virtual void removeProxyModel(QAbstractProxyModel *AProxyModel);
	virtual QModelIndex mapToModel(const QModelIndex &AProxyIndex) const;
	virtual QModelIndex mapFromModel(const QModelIndex &AModelIndex) const;
	virtual QModelIndex mapToProxy(QAbstractProxyModel *AProxyModel, const QModelIndex &AModelIndex) const;
	virtual QModelIndex mapFromProxy(QAbstractProxyModel *AProxyModel, const QModelIndex &AProxyIndex) const;
	//--IndexLabel
	virtual int createIndexLabel(int AOrder, const QVariant &ALabel, int AFlags = 0);
	virtual void updateIndexLabel(int ALabelId, const QVariant &ALabel, int AFlags = 0);
	virtual void insertIndexLabel(int ALabelId, IRosterIndex *AIndex);
	virtual void removeIndexLabel(int ALabelId, IRosterIndex *AIndex);
	virtual void destroyIndexLabel(int ALabelId);
	virtual int labelAt(const QPoint &APoint, const QModelIndex &AIndex) const;
	virtual QRect labelRect(int ALabeld, const QModelIndex &AIndex) const;
	//--IndexNotify
	virtual int appendNotify(QList<IRosterIndex *> AIndexes, int AOrder, const QIcon &AIcon, const QString &AToolTip, int AFlags=0);
	virtual QList<int> indexNotifies(IRosterIndex *Index, int AOrder) const;
	virtual void updateNotify(int ANotifyId, const QIcon &AIcon, const QString &AToolTip, int AFlags=0);
	virtual void removeNotify(int ANotifyId);
	//--ClickHookers
	virtual void insertClickHooker(int AOrder, IRostersClickHooker *AHooker);
	virtual void removeClickHooker(int AOrder, IRostersClickHooker *AHooker);
	//--DragDrop
	virtual void insertDragDropHandler(IRostersDragDropHandler *AHandler);
	virtual void removeDragDropHandler(IRostersDragDropHandler *AHandler);
	//--FooterText
	virtual void insertFooterText(int AOrderAndId, const QVariant &AValue, IRosterIndex *AIndex);
	virtual void removeFooterText(int AOrderAndId, IRosterIndex *AIndex);
	//--ContextMenu
	virtual void contextMenuForIndex(IRosterIndex *AIndex, int ALabelId, Menu *AMenu);
	//--ClipboardMenu
	virtual void clipboardMenuForIndex(IRosterIndex *AIndex, Menu *AMenu);
signals:
	void modelAboutToBeSeted(IRostersModel *AModel);
	void modelSeted(IRostersModel *AModel);
	void proxyModelAboutToBeInserted(QAbstractProxyModel *AProxyModel, int AOrder);
	void proxyModelInserted(QAbstractProxyModel *AProxyModel);
	void proxyModelAboutToBeRemoved(QAbstractProxyModel *AProxyModel);
	void proxyModelRemoved(QAbstractProxyModel *AProxyModel);
	void viewModelAboutToBeChanged(QAbstractItemModel *AModel);
	void viewModelChanged(QAbstractItemModel *AModel);
	void indexContextMenu(IRosterIndex *AIndex, Menu *AMenu);
	void indexClipboardMenu(IRosterIndex *AIndex, Menu *AMenu);
	void labelContextMenu(IRosterIndex *AIndex, int ALabelId, Menu *AMenu);
	void labelToolTips(IRosterIndex *AIndex, int ALabelId, QMultiMap<int,QString> &AToolTips);
	void labelClicked(IRosterIndex *AIndex, int ALabelId);
	void labelDoubleClicked(IRosterIndex *AIndex, int ALabelId, bool &AAccepted);
	void notifyContextMenu(IRosterIndex *AIndex, int ANotifyId, Menu *AMenu);
	void notifyActivated(IRosterIndex *AIndex, int ANotifyId);
	void notifyRemovedByIndex(IRosterIndex *AIndex, int ANotifyId);
	void dragDropHandlerInserted(IRostersDragDropHandler *AHandler);
	void dragDropHandlerRemoved(IRostersDragDropHandler *AHandler);
public:
	void updateStatusText(IRosterIndex *AIndex = NULL);
protected:
	QStyleOptionViewItemV4 indexOption(const QModelIndex &AIndex) const;
	void appendBlinkLabel(int ALabelId);
	void removeBlinkLabel(int ALabelId);
	QString intId2StringId(int AIntId);
	void removeLabels();
	void setDropIndicatorRect(const QRect &ARect);
protected:
	//QTreeView
	virtual void drawBranches(QPainter *APainter, const QRect &ARect, const QModelIndex &AIndex) const;
	//QAbstractItemView
	virtual bool viewportEvent(QEvent *AEvent);
	virtual void resizeEvent(QResizeEvent *AEvent);
	//QWidget
	virtual void paintEvent(QPaintEvent *AEvent);
	virtual void contextMenuEvent(QContextMenuEvent *AEvent);
	virtual void mouseDoubleClickEvent(QMouseEvent *AEvent);
	virtual void mousePressEvent(QMouseEvent *AEvent);
	virtual void mouseMoveEvent (QMouseEvent *AEvent);
	virtual void mouseReleaseEvent(QMouseEvent *AEvent);
	virtual void dropEvent(QDropEvent *AEvent);
	virtual void dragEnterEvent(QDragEnterEvent *AEvent);
	virtual void dragMoveEvent(QDragMoveEvent *AEvent);
	virtual void dragLeaveEvent(QDragLeaveEvent *AEvent);
protected slots:
	void onRosterIndexContextMenu(IRosterIndex *AIndex, Menu *AMenu);
	void onRosterLabelToolTips(IRosterIndex *AIndex, int ALabelId, QMultiMap<int,QString> &AToolTips);
	void onCopyToClipboardActionTriggered(bool);
	void onIndexInserted(IRosterIndex *AIndex);
	void onIndexDestroyed(IRosterIndex *AIndex);
	void onBlinkTimer();
	void onDragExpandTimer();
	void onShortcutActivated(const QString &AId, QWidget *AWidget);
private:
	IRostersModel *FRostersModel;
private:
	int FPressedLabel;
	QPoint FPressedPos;
	QModelIndex FPressedIndex;
private:
	bool FBlinkShow;
	int FLabelIdCounter;
	QTimer FBlinkTimer;
	QSet<int> FBlinkLabels;
	QHash<int, QVariant> FIndexLabels;
	QHash<int, int> FIndexLabelOrders;
	QHash<int, int> FIndexLabelFlags;
	QHash<int, QSet<IRosterIndex *> > FIndexLabelIndexes;
private:
	int FNotifyId;
	QHash<int, NotifyItem> FNotifyItems;
	QHash<int, QList<int> > FNotifyLabelItems;
	QHash<IRosterIndex *, QHash<int, int> > FNotifyIndexOrderLabel;
private:
	QMultiMap<int, IRostersClickHooker *> FClickHookers;
private:
	RosterIndexDelegate *FRosterIndexDelegate;
	QMultiMap<int, QAbstractProxyModel *> FProxyModels;
private:
	bool FStartDragFailed;
	QTimer FDragExpandTimer;
	QRect FDropIndicatorRect;
	QList<IRostersDragDropHandler *> FDragDropHandlers;
	QList<IRostersDragDropHandler *> FActiveDragHandlers;
};

#endif // ROSTERSVIEW_H
