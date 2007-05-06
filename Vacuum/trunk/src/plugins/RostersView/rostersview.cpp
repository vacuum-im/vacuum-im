#include <QtDebug>
#include "rostersview.h"

#include <QHeaderView>
#include <QToolTip>
#include <QTime>

RostersView::RostersView(QWidget *AParent)
  : QTreeView(AParent)
{
  srand(QTime::currentTime().msec());

  FRostersModel = NULL;
  FContextMenu = new Menu(this);
  FIndexDataHolder = new IndexDataHolder(this);
  FRosterIndexDelegate = new RosterIndexDelegate(this);

  header()->hide();
  setIndentation(10);
  setRootIsDecorated(false);
  setSelectionMode(NoSelection);
  setSortingEnabled(true);
  setContextMenuPolicy(Qt::DefaultContextMenu);
  setItemDelegate(FRosterIndexDelegate);

  SkinIconset iconset;
  iconset.openFile(STATUS_ICONSETFILE);
  labelId1 = createIndexLabel(10001,QString("<>"));
  labelId2 = createIndexLabel(10002,iconset.iconByName("online"));
}

RostersView::~RostersView()
{

}

void RostersView::setModel(IRostersModel *AModel)
{
  if (FRostersModel != AModel)
  {
    emit modelAboutToBeSeted(AModel);
    
    if (FRostersModel)
    {
      disconnect(FRostersModel,SIGNAL(indexCreated(IRosterIndex *,IRosterIndex *)),
        this,SLOT(onIndexCreated(IRosterIndex *, IRosterIndex *)));
      disconnect(AModel,SIGNAL(indexRemoved(IRosterIndex *)),
        this,SLOT(onIndexRemoved(IRosterIndex *)));
      FIndexDataHolder->clear();
    }
    if (AModel)
    {
      connect(AModel,SIGNAL(indexCreated(IRosterIndex *,IRosterIndex *)),
        SLOT(onIndexCreated(IRosterIndex *, IRosterIndex *)));
      connect(AModel,SIGNAL(indexRemoved(IRosterIndex *)),
        SLOT(onIndexRemoved(IRosterIndex *)));
    }

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

int RostersView::createIndexLabel(int AOrder, const QVariant &ALabel)
{
  int labelId = 0;
  if (!ALabel.isNull())
  {
    while (labelId == NULL_LABEL_ID || labelId == DISPLAY_LABEL_ID || FIndexLabels.contains(labelId))
      labelId = (rand()<<16)+rand();
    FIndexLabels.insert(labelId,ALabel);
    FIndexLabelOrders.insert(labelId,AOrder);
  }
  return labelId;
}

void RostersView::updateIndexLabel(int ALabelId, const QVariant &ALabel)
{
  if (FIndexLabels.contains(ALabelId) && FIndexLabels.value(ALabelId)!=ALabel)
  {
    if (!ALabel.isValid())
    {
      FIndexLabels[ALabelId] = ALabel;
      foreach (IRosterIndex *index, FIndexLabelIndexes.value(ALabelId))
      {
        insertIndexLabel(ALabelId,index);
      }
    }
    else
    {
      foreach (IRosterIndex *index, FIndexLabelIndexes.value(ALabelId))
      {
        removeIndexLabel(ALabelId,index);
      }
      FIndexLabels.remove(ALabelId);
      FIndexLabelOrders.remove(ALabelId);
      FIndexLabelIndexes.remove(ALabelId);
    }
  }
}

void RostersView::insertIndexLabel(int ALabelId, IRosterIndex *AIndex)
{
  if (AIndex && FIndexLabels.contains(ALabelId))
  {
    int order = FIndexLabelOrders.value(ALabelId);
    QVariant label = FIndexLabels.value(ALabelId);
    QList<QVariant> ids = AIndex->data(IRosterIndex::DR_LabelIds).toList();
    QList<QVariant> orders = AIndex->data(IRosterIndex::DR_LabelOrders).toList();
    QList<QVariant> labels = AIndex->data(IRosterIndex::DR_LabelValues).toList();
    int i = 0;
    while (i<orders.count() && orders.at(i).toInt() < order)
      i++;
    ids.insert(i,ALabelId);
    orders.insert(i,order);
    labels.insert(i,label);
    FIndexLabelIndexes[ALabelId] += AIndex;
    AIndex->setData(IRosterIndex::DR_LabelIds,ids);
    AIndex->setData(IRosterIndex::DR_LabelOrders,orders);
    AIndex->setData(IRosterIndex::DR_LabelValues,labels);
  }
}

void RostersView::removeIndexLabel(int ALabelId, IRosterIndex *AIndex)
{
  if (AIndex && FIndexLabels.contains(ALabelId) && FIndexLabelIndexes.value(ALabelId).contains(AIndex))
  {
    int order = FIndexLabelOrders.value(ALabelId);
    QVariant label = FIndexLabels.value(ALabelId);
    QList<QVariant> ids = AIndex->data(IRosterIndex::DR_LabelIds).toList();
    QList<QVariant> orders = AIndex->data(IRosterIndex::DR_LabelOrders).toList();
    QList<QVariant> labels = AIndex->data(IRosterIndex::DR_LabelValues).toList();
    int i = 0;
    while (i<orders.count() && (orders.at(i).toInt()!=order || labels.at(i)!=label))
      i++;
    ids.removeAt(i);
    orders.removeAt(i);
    labels.removeAt(i);
    FIndexLabelIndexes[ALabelId] -= AIndex;
    if (FIndexLabelIndexes[ALabelId].isEmpty())
      FIndexLabelIndexes.remove(ALabelId);
    AIndex->setData(IRosterIndex::DR_LabelIds,ids);
    AIndex->setData(IRosterIndex::DR_LabelOrders,orders);
    AIndex->setData(IRosterIndex::DR_LabelValues,labels);
  }
}

int RostersView::labelAt(const QPoint &APoint) const
{
  return NULL_LABEL_ID;
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

bool RostersView::viewportEvent(QEvent *AEvent)
{
  switch(AEvent->type())
  {
  case QEvent::ToolTip:
    {
      QHelpEvent *helpEvent = static_cast<QHelpEvent *>(AEvent); 
      QModelIndex index = indexAt(helpEvent->pos());
      if (index.isValid())
      {
        QMultiMap<int,QString> toolTips;
        toolTips.insert(ROSTERSVIEW_TOOLTIPORDER,index.data(Qt::ToolTipRole).toString());
        emit toolTipMap(index,toolTips);

        QString allToolTips;
        QMultiMap<int,QString>::const_iterator it = toolTips.constBegin();
        while (it != toolTips.constEnd())
        {
          if (!it.value().isEmpty())
            allToolTips += it.value() +"<br>";
          it++;
        }
        allToolTips.chop(4);
        QToolTip::showText(helpEvent->globalPos(),allToolTips,this);
        return true;
      }
    }
  default:
    return QTreeView::viewportEvent(AEvent);
  }
}

void RostersView::onIndexCreated(IRosterIndex *AIndex, IRosterIndex *)
{
  if (AIndex->type() < IRosterIndex::IT_UserDefined)
    AIndex->setDataHolder(FIndexDataHolder);
  //insertIndexLabel(labelId1,AIndex);
  //insertIndexLabel(labelId2,AIndex);
}
                                               
void RostersView::onIndexRemoved(IRosterIndex *AIndex)
{
  QHash<int, QSet<IRosterIndex *> >::iterator it = FIndexLabelIndexes.begin();
  while (it!=FIndexLabelIndexes.end())
  {
    if (it.value().contains(AIndex))
      it.value() -= AIndex;

    if (it.value().isEmpty())
      it = FIndexLabelIndexes.erase(it);
    else
      it++;
  }
}

