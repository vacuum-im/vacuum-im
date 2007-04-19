#include <QtDebug>
#include "rostersview.h"

#include <QHeaderView>
#include "rosterindexdelegate.h"

RostersView::RostersView(QWidget *AParent)
  : QTreeView(AParent)
{
  FRostersModel = NULL;
  FContextMenu = new Menu(this);

  header()->hide();
  setIndentation(8);
  setRootIsDecorated(false);
  setSelectionMode(NoSelection);
  setSortingEnabled(true);
  setContextMenuPolicy(Qt::DefaultContextMenu);
  setItemDelegate(new RosterIndexDelegate(this));
}

RostersView::~RostersView()
{

}

void RostersView::setModel(IRostersModel *AModel)
{
  if (FRostersModel != AModel)
  {
    emit modelAboutToBeSeted(AModel);
    FRostersModel = AModel;
    if (!FProxyModels.isEmpty())
      FProxyModels.first()->setSourceModel(AModel);
    QTreeView::setModel(AModel);
    emit modelSeted(AModel);
  }
}

void RostersView::addProxyModel(QAbstractProxyModel *AProxyModel)
{
  if (AProxyModel && !FProxyModels.contains(AProxyModel))
  {
    emit proxyModelAboutToBeAdded(AProxyModel);
    QAbstractProxyModel *lastProxy = lastProxyModel();
    FProxyModels.append(AProxyModel);
    QTreeView::setModel(AProxyModel);
    if (lastProxy)
      AProxyModel->setSourceModel(lastProxy);
    else
      AProxyModel->setSourceModel(FRostersModel);
    emit proxyModelAdded(AProxyModel); 
  }
}

void RostersView::removeProxyModel(QAbstractProxyModel *AProxyModel)
{
  int index = FProxyModels.indexOf(AProxyModel);
  if (index != -1)
  {
    emit proxyModelAboutToBeRemoved(AProxyModel);
    QAbstractProxyModel *befour = FProxyModels.value(index-1,NULL);
    QAbstractProxyModel *after = FProxyModels.value(index+1,NULL);
    if (after == NULL && befour == NULL)
      QTreeView::setModel(FRostersModel);
    else if (after == NULL)
      QTreeView::setModel(befour);
    else if (befour = NULL)
      after->setSourceModel(FRostersModel);
    else
      after->setSourceModel(befour);
    AProxyModel->setSourceModel(NULL);
    FProxyModels.removeAt(index);
    emit proxyModelRemoved(AProxyModel);
  } 
}

QModelIndex RostersView::mapToModel(const QModelIndex &AProxyIndex)
{
  if (FProxyModels.count() > 0)
  {
    QModelIndex index = AProxyIndex;
    QList<QAbstractProxyModel *>::const_iterator it = FProxyModels.constEnd();
    do 
    {
      it--;
      index = (*it)->mapToSource(index);
    } while(it != FProxyModels.constBegin());
    return index;
  }
  else
    return AProxyIndex;
}

QModelIndex RostersView::mapFromModel(const QModelIndex &AModelIndex)
{
  if (FProxyModels.count() > 0)
  {
    QModelIndex index = AModelIndex;
    QList<QAbstractProxyModel *>::const_iterator it = FProxyModels.constBegin();
    while (it != FProxyModels.constEnd())
    {
      index = (*it)->mapFromSource(index);
      it++;
    }
    return index;
  }
  else
    return AModelIndex;
}

void RostersView::drawBranches(QPainter *APainter, const QRect &ARect, const QModelIndex &AIndex) const
{
  QVariant data = AIndex.data(Qt::BackgroundRole);
  if (qVariantCanConvert<QBrush>(data))
  {
    QRect rect(ARect.right()-indentation(),ARect.top(),indentation()+1,ARect.height());
    APainter->fillRect(rect, qvariant_cast<QBrush>(data));
  }

  QTreeView::drawBranches(APainter,ARect,AIndex);
}

void RostersView::contextMenuEvent(QContextMenuEvent *AEvent)
{
  QModelIndex index = indexAt(AEvent->pos());
  FContextMenu->clear();
  emit contextMenu(index,FContextMenu);
  if (!FContextMenu->isEmpty())
    FContextMenu->popup(AEvent->globalPos());
}

