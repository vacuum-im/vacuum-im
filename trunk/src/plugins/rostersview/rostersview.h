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
#include <definitions/rosterdataholderorders.h>
#include <interfaces/irostersview.h>
#include <interfaces/irostersmodel.h>
#include <utils/options.h>
#include <utils/shortcuts.h>
#include "rosterindexdelegate.h"

class RostersView :
	public QTreeView,
	public IRostersView,
	public IRosterDataHolder
{
	Q_OBJECT;
	Q_INTERFACES(IRostersView IRosterDataHolder);
public:
	RostersView(QWidget *AParent = NULL);
	~RostersView();
	virtual QTreeView *instance() { return this; }
	//IRosterDataHolder
	virtual int rosterDataOrder() const;
	virtual QList<int> rosterDataRoles() const;
	virtual QList<int> rosterDataTypes() const;
	virtual QVariant rosterData(const IRosterIndex *AIndex, int ARole) const;
	virtual bool setRosterData(IRosterIndex *AIndex, int ARole, const QVariant &AValue);
	//IRostersView
	virtual IRostersModel *rostersModel() const;
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
	virtual int registerLabel(const IRostersLabel &ALabel);
	virtual void updateLabel(int ALabelId, const IRostersLabel &ALabel);
	virtual void insertLabel(int ALabelId, IRosterIndex *AIndex);
	virtual void removeLabel(int ALabelId, IRosterIndex *AIndex);
	virtual void destroyLabel(int ALabelId);
	virtual int labelAt(const QPoint &APoint, const QModelIndex &AIndex) const;
	virtual QRect labelRect(int ALabeld, const QModelIndex &AIndex) const;
	//--IndexNotify
	virtual int activeNotify(IRosterIndex *AIndex) const;
	virtual QList<int> notifyQueue(IRosterIndex *Index) const;
	virtual IRostersNotify notifyById(int ANotifyId) const;
	virtual QList<IRosterIndex *> notifyIndexes(int ANotifyId) const;
	virtual int insertNotify(const IRostersNotify &ANotify, const QList<IRosterIndex *> &AIndexes);
	virtual void activateNotify(int ANotifyId);
	virtual void removeNotify(int ANotifyId);
	//--ClickHookers
	virtual void insertClickHooker(int AOrder, IRostersClickHooker *AHooker);
	virtual void removeClickHooker(int AOrder, IRostersClickHooker *AHooker);
	//--KeyHookers
	virtual void insertKeyHooker(int AOrder, IRostersKeyHooker *AHooker);
	virtual void removeKeyHooker(int AOrder, IRostersKeyHooker *AHooker);
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
	void notifyInserted(int ANotifyId);
	void notifyActivated(int ANotifyId);
	void notifyRemoved(int ANotifyId);
	void dragDropHandlerInserted(IRostersDragDropHandler *AHandler);
	void dragDropHandlerRemoved(IRostersDragDropHandler *AHandler);
	//IRosterDataHolder
	void rosterDataChanged(IRosterIndex *AIndex = NULL, int ARole = 0);
public:
	void updateStatusText(IRosterIndex *AIndex = NULL);
protected:
	QStyleOptionViewItemV4 indexOption(const QModelIndex &AIndex) const;
	void appendBlinkItem(int ALabelId, int ANotifyId);
	void removeBlinkItem(int ALabelId, int ANotifyId);
	QString intId2StringId(int AIntId) const;
	void removeLabels();
	void setDropIndicatorRect(const QRect &ARect);
protected:
	//QTreeView
	void drawBranches(QPainter *APainter, const QRect &ARect, const QModelIndex &AIndex) const;
	//QAbstractItemView
	bool viewportEvent(QEvent *AEvent);
	void resizeEvent(QResizeEvent *AEvent);
	//QWidget
	void paintEvent(QPaintEvent *AEvent);
	void contextMenuEvent(QContextMenuEvent *AEvent);
	void mouseDoubleClickEvent(QMouseEvent *AEvent);
	void mousePressEvent(QMouseEvent *AEvent);
	void mouseMoveEvent(QMouseEvent *AEvent);
	void mouseReleaseEvent(QMouseEvent *AEvent);
	void keyPressEvent(QKeyEvent *AEvent);
	void keyReleaseEvent(QKeyEvent *AEvent);
	void dropEvent(QDropEvent *AEvent);
	void dragEnterEvent(QDragEnterEvent *AEvent);
	void dragMoveEvent(QDragMoveEvent *AEvent);
	void dragLeaveEvent(QDragLeaveEvent *AEvent);
protected slots:
	void onRosterIndexContextMenu(IRosterIndex *AIndex, Menu *AMenu);
	void onRosterLabelToolTips(IRosterIndex *AIndex, int ALabelId, QMultiMap<int,QString> &AToolTips);
	void onCopyToClipboardActionTriggered(bool);
	void onIndexInserted(IRosterIndex *AIndex);
	void onIndexDestroyed(IRosterIndex *AIndex);
	void onRemoveIndexNotifyTimeout();
	void onUpdateIndexNotifyTimeout();
	void onBlinkTimerTimeout();
	void onDragExpandTimer();
	void onShortcutActivated(const QString &AId, QWidget *AWidget);
private:
	IRostersModel *FRostersModel;
private:
	int FPressedLabel;
	QPoint FPressedPos;
	QModelIndex FPressedIndex;
private:
	bool FBlinkVisible;
	QTimer FBlinkTimer;
	QSet<int> FBlinkLabels;
	QSet<int> FBlinkNotifies;
private:
	QMap<int, IRostersLabel> FLabelItems;
	QMultiMap<IRosterIndex *, int> FIndexLabels;
private:
	QMap<QTimer *, int> FNotifyTimer;
	QSet<IRosterIndex *> FNotifyUpdates;
	QMap<int, IRostersNotify> FNotifyItems;
	QMap<IRosterIndex *, int> FActiveNotifies;
	QMultiMap<IRosterIndex *, int> FIndexNotifies;
private:
	QMultiMap<int, IRostersKeyHooker *> FKeyHookers;
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
