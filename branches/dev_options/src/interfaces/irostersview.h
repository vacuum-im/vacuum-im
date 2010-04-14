#ifndef IROSTERSVIEW_H
#define IROSTERSVIEW_H

#include <QTreeView>
#include <QAbstractProxyModel>
#include <interfaces/irostersmodel.h>
#include <utils/menu.h>

#define ROSTERSVIEW_UUID "{BDD12B32-9C88-4e3c-9B36-2DCB5075288F}"

class IRostersClickHooker
{
public:
  virtual bool rosterIndexClicked(IRosterIndex *AIndex, int AOrder) =0;
};

class IRostersDragDropHandler
{
public:
  virtual Qt::DropActions rosterDragStart(const QMouseEvent *AEvent, const QModelIndex &AIndex, QDrag *ADrag) =0;
  virtual bool rosterDragEnter(const QDragEnterEvent *AEvent) =0;
  virtual bool rosterDragMove(const QDragMoveEvent *AEvent, const QModelIndex &AHover) =0;
  virtual void rosterDragLeave(const QDragLeaveEvent *AEvent) =0;
  virtual bool rosterDropAction(const QDropEvent *AEvent, const QModelIndex &AIndex, Menu *AMenu) =0;
};

class IRostersView
{
public:
  enum LabelFlags {
    LabelBlink                    =0x01,
    LabelVisible                  =0x02,
    LabelExpandParents            =0x04
  };
public:
  //--RostersModel
  virtual QTreeView *instance() = 0;
  virtual IRostersModel *rostersModel() const =0;
  virtual void setRostersModel(IRostersModel *AModel) =0;
  virtual bool repaintRosterIndex(IRosterIndex *AIndex) =0;
  virtual void expandIndexParents(IRosterIndex *AIndex) =0;
  virtual void expandIndexParents(const QModelIndex &AIndex) =0;
  //--ProxyModels
  virtual void insertProxyModel(QAbstractProxyModel *AProxyModel, int AOrder) =0;
  virtual QList<QAbstractProxyModel *> proxyModels() const =0;
  virtual void removeProxyModel(QAbstractProxyModel *AProxyModel) =0;
  virtual QModelIndex mapToModel(const QModelIndex &AProxyIndex) const=0;
  virtual QModelIndex mapFromModel(const QModelIndex &AModelIndex) const=0;
  virtual QModelIndex mapToProxy(QAbstractProxyModel *AProxyModel, const QModelIndex &AModelIndex) const=0;
  virtual QModelIndex mapFromProxy(QAbstractProxyModel *AProxyModel, const QModelIndex &AProxyIndex) const=0;
  //--IndexLabel
  virtual int createIndexLabel(int AOrder, const QVariant &ALabel, int AFlags = 0) =0;
  virtual void updateIndexLabel(int ALabelId, const QVariant &ALabel, int AFlags = 0) =0;
  virtual void insertIndexLabel(int ALabelId, IRosterIndex *AIndex) =0;
  virtual void removeIndexLabel(int ALabelId, IRosterIndex *AIndex) =0;
  virtual void destroyIndexLabel(int ALabelId) =0;
  virtual int labelAt(const QPoint &APoint, const QModelIndex &AIndex) const =0;
  virtual QRect labelRect(int ALabeld, const QModelIndex &AIndex) const =0;
  //--IndexNotify
  virtual int appendNotify(QList<IRosterIndex *> AIndexes, int AOrder, const QIcon &AIcon, const QString &AToolTip, int AFlags=0) =0;
  virtual QList<int> indexNotifies(IRosterIndex *Index, int AOrder) const =0;
  virtual void updateNotify(int ANotifyId, const QIcon &AIcon, const QString &AToolTip, int AFlags=0) =0;
  virtual void removeNotify(int ANotifyId) =0;
  //--ClickHookers
  virtual void insertClickHooker(int AOrder, IRostersClickHooker *AHooker) =0;
  virtual void removeClickHooker(int AOrder, IRostersClickHooker *AHooker) =0;
  //--DragDrop
  virtual void insertDragDropHandler(IRostersDragDropHandler *AHandler) =0;
  virtual void removeDragDropHandler(IRostersDragDropHandler *AHandler) =0;
  //--FooterText
  virtual void insertFooterText(int AOrderAndId, const QVariant &AValue, IRosterIndex *AIndex) =0;
  virtual void removeFooterText(int AOrderAndId, IRosterIndex *AIndex) =0;
  //--ContextMenu
  virtual void contextMenuForIndex(IRosterIndex *AIndex, int ALabelId, Menu *AMenu) =0;
  //--ClipboardMenu
  virtual void clipboardMenuForIndex(IRosterIndex *AIndex, Menu *AMenu) =0;
protected:
  virtual void modelAboutToBeSeted(IRostersModel *AIndex) =0;
  virtual void modelSeted(IRostersModel *AIndex) =0;
  virtual void proxyModelAboutToBeInserted(QAbstractProxyModel *AProxyModel, int AOrder) =0;
  virtual void proxyModelInserted(QAbstractProxyModel *AProxyModel) =0;
  virtual void proxyModelAboutToBeRemoved(QAbstractProxyModel *AProxyModel) =0;
  virtual void proxyModelRemoved(QAbstractProxyModel *AProxyModel) =0;
  virtual void viewModelAboutToBeChanged(QAbstractItemModel *AModel) =0;
  virtual void viewModelChanged(QAbstractItemModel *AModel) =0;
  virtual void indexContextMenu(IRosterIndex *AIndex, Menu *AMenu) =0;
  virtual void indexClipboardMenu(IRosterIndex *AIndex, Menu *AMenu) =0;
  virtual void labelContextMenu(IRosterIndex *AIndex, int ALabelId, Menu *AMenu) =0;
  virtual void labelToolTips(IRosterIndex *AIndex, int ALabelId, QMultiMap<int,QString> &AToolTips) =0;
  virtual void labelClicked(IRosterIndex *AIndex, int ALabelId) =0;
  virtual void labelDoubleClicked(IRosterIndex *AIndex, int ALabelId, bool &AAccepted) =0;
  virtual void notifyContextMenu(IRosterIndex *AIndex, int ANotifyId, Menu *AMenu) =0;
  virtual void notifyActivated(IRosterIndex *AIndex, int ANotifyId) =0;
  virtual void notifyRemovedByIndex(IRosterIndex *AIndex, int ANotifyId) =0;
  virtual void dragDropHandlerInserted(IRostersDragDropHandler *AHandler) =0;
  virtual void dragDropHandlerRemoved(IRostersDragDropHandler *AHandler) =0;
};

class IRostersViewPlugin
{
public:
  virtual QObject *instance() = 0;
  virtual IRostersView *rostersView() =0;
  virtual void startRestoreExpandState() =0;
  virtual void restoreExpandState(const QModelIndex &AParent = QModelIndex()) =0;
};

Q_DECLARE_INTERFACE(IRostersClickHooker,"Vacuum.Plugin.IRostersClickHooker/1.0");
Q_DECLARE_INTERFACE(IRostersDragDropHandler,"Vacuum.Plugin.IRostersDragDropHandler/1.0");
Q_DECLARE_INTERFACE(IRostersView,"Vacuum.Plugin.IRostersView/1.0");
Q_DECLARE_INTERFACE(IRostersViewPlugin,"Vacuum.Plugin.IRostersViewPlugin/1.0");

#endif
