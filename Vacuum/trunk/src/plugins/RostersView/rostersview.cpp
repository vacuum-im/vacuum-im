#include <QtDebug>
#include "rostersview.h"
#include <QHeaderView>
#include "rosterindexdelegate.h"
//#include "sortfilterproxymodel.h"

RostersView::RostersView(IRostersModel *AModel, QWidget *AParent)
  : QTreeView(AParent)
{
  FRostersModel = AModel;
  FContextMenu = new Menu(0,this);

  header()->hide();
  setIndentation(3);
  setRootIsDecorated(false);
  setSelectionMode(NoSelection);
  setSortingEnabled(true);
  setContextMenuPolicy(Qt::DefaultContextMenu);

  setItemDelegate(new RosterIndexDelegate(this));
  
  setModel(FRostersModel);

  //SortFilterProxyModel *proxyModel = new SortFilterProxyModel(this);
  //proxyModel->setDynamicSortFilter(true);
  //addProxyModel(proxyModel);
}

RostersView::~RostersView()
{

}

void RostersView::addProxyModel(QAbstractProxyModel *AProxyModel)
{
  if (AProxyModel && !FProxyModels.contains(AProxyModel))
  {
    if (!FProxyModels.isEmpty())
      AProxyModel->setSourceModel(FProxyModels.last());
    else
      AProxyModel->setSourceModel(FRostersModel);
    FProxyModels.append(AProxyModel);
    setModel(AProxyModel);
    emit proxyModelAdded(AProxyModel); 
  }
}

void RostersView::removeProxyModel(QAbstractProxyModel *AProxyModel)
{
  int index = FProxyModels.indexOf(AProxyModel);
  if (index != -1)
  {
    QAbstractProxyModel *befour = FProxyModels.value(index-1,NULL);
    QAbstractProxyModel *after = FProxyModels.value(index+1,NULL);
    if (after == NULL && befour == NULL)
      setModel(FRostersModel);
    else if (after == NULL)
      setModel(befour);
    else if (befour = NULL)
      after->setSourceModel(FRostersModel);
    else
      after->setSourceModel(befour);
    FProxyModels.remove(index);
    emit proxyModelRemoved(AProxyModel);
  } 
}

void RostersView::contextMenuEvent(QContextMenuEvent *AEvent)
{
  QModelIndex index = indexAt(AEvent->pos());
  FContextMenu->clear();
  emit contextMenu(index,FContextMenu);
  if (!FContextMenu->isEmpty())
    FContextMenu->popup(AEvent->globalPos());
}
