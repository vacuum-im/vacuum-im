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
	virtual bool editRosterIndex(int ADataRole, IRosterIndex *AIndex);
	virtual bool hasMultiSelection() const;
	virtual QList<IRosterIndex *> selectedRosterIndexes() const;
	virtual void selectRosterIndex(IRosterIndex *AIndex);
	virtual QMap<int, QStringList > indexesRolesMap(const QList<IRosterIndex *> &AIndexes, const QList<int> ARoles, int AUniqueRole=-1) const;
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
	//--EditHandlers
	virtual void insertEditHandler(int AOrder, IRostersEditHandler *AHandler);
	virtual void removeEditHandler(int AOrder, IRostersEditHandler *AHandler);
	//--FooterText
	virtual void insertFooterText(int AOrderAndId, const QVariant &AValue, IRosterIndex *AIndex);
	virtual void removeFooterText(int AOrderAndId, IRosterIndex *AIndex);
	//--ContextMenu
	virtual void contextMenuForIndex(const QList<IRosterIndex *> &AIndexes, int ALabelId, Menu *AMenu);
	//--ClipboardMenu
	virtual void clipboardMenuForIndex(const QList<IRosterIndex *> &AIndexes, Menu *AMenu);
signals:
	void modelAboutToBeSeted(IRostersModel *AModel);
	void modelSeted(IRostersModel *AModel);
	void proxyModelAboutToBeInserted(QAbstractProxyModel *AProxyModel, int AOrder);
	void proxyModelInserted(QAbstractProxyModel *AProxyModel);
	void proxyModelAboutToBeRemoved(QAbstractProxyModel *AProxyModel);
	void proxyModelRemoved(QAbstractProxyModel *AProxyModel);
	void viewModelAboutToBeChanged(QAbstractItemModel *AModel);
	void viewModelChanged(QAbstractItemModel *AModel);
	void indexMultiSelection(const QList<IRosterIndex *> &ASelected, bool &AAccepted);
	void indexClipboardMenu(const QList<IRosterIndex *> &AIndexes, Menu *AMenu);
	void indexContextMenu(const QList<IRosterIndex *> &AIndexes, int ALabelId, Menu *AMenu);
	void indexToolTips(IRosterIndex *AIndex, int ALabelId, QMultiMap<int,QString> &AToolTips);
	void indexClicked(IRosterIndex *AIndex, int ALabelId);
	void indexDoubleClicked(IRosterIndex *AIndex, int ALabelId);
	void notifyInserted(int ANotifyId);
	void notifyActivated(int ANotifyId);
	void notifyRemoved(int ANotifyId);
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
	IRostersEditHandler *findEditHandler(int ADataRole, const QModelIndex &AIndex) const;
protected:
	//QTreeView
	void drawBranches(QPainter *APainter, const QRect &ARect, const QModelIndex &AIndex) const;
	//QAbstractItemView
	bool viewportEvent(QEvent *AEvent);
	void resizeEvent(QResizeEvent *AEvent);
	bool edit(const QModelIndex &AIndex, EditTrigger ATrigger, QEvent *AEvent);
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
	//QAbstractItemView
	void closeEditor(QWidget *AEditor, QAbstractItemDelegate::EndEditHint AHint);
protected slots:
	void onRosterIndexContextMenu(const QList<IRosterIndex *> &AIndexes, int ALabelId, Menu *AMenu);
	void onRosterIndexToolTips(IRosterIndex *AIndex, int ALabelId, QMultiMap<int,QString> &AToolTips);
   void onSelectionChanged(const QItemSelection &ASelected, const QItemSelection &ADeselected);
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
	QMultiMap<int, IRostersEditHandler *> FEditHandlers;
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
