#include <QtDebug>
#include "rostersview.h"

#include <QHeaderView>
#include <QToolTip>
#include <QTime>
#include <QCursor>

RostersView::RostersView(QWidget *AParent)
  : QTreeView(AParent)
{
  srand(QTime::currentTime().msec());

  FRostersModel = NULL;
  FPressedLabel = DISPLAY_LABEL_ID;
  FPressedIndex = NULL;

  FContextMenu = new Menu(this);
  FIndexDataHolder = new IndexDataHolder(this);
  FRosterIndexDelegate = new RosterIndexDelegate(this);

  FBlinkShow = true;
  FBlinkTimer.setInterval(500);
  connect(&FBlinkTimer,SIGNAL(timeout()),SLOT(onBlinkTimer()));

  header()->hide();
  setIndentation(10);
  setRootIsDecorated(false);
  setSelectionMode(NoSelection);
  setSortingEnabled(true);
  setContextMenuPolicy(Qt::DefaultContextMenu);
  setItemDelegate(FRosterIndexDelegate);
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
  QModelIndex index = AProxyIndex;
  if (FProxyModels.count() > 0)
  {
    QList<QAbstractProxyModel *>::const_iterator it = FProxyModels.constEnd();
    do 
    {
      it--;
      index = (*it)->mapToSource(index);
    } while(it != FProxyModels.constBegin());
  }
  return index;
}

QModelIndex RostersView::mapFromModel(const QModelIndex &AModelIndex)
{
  QModelIndex index = AModelIndex;
  if (FProxyModels.count() > 0)
  {
    QList<QAbstractProxyModel *>::const_iterator it = FProxyModels.constBegin();
    while (it != FProxyModels.constEnd())
    {
      index = (*it)->mapFromSource(index);
      it++;
    }
  }
  return index;
}

QModelIndex RostersView::mapToProxy(QAbstractProxyModel *AProxyModel, const QModelIndex &AModelIndex)
{
  QModelIndex index = AModelIndex;
  if (FProxyModels.count() > 0)
  {
    QList<QAbstractProxyModel *>::const_iterator it = FProxyModels.constBegin();
    while (it!=FProxyModels.constEnd())
    {
      index = (*it)->mapFromSource(index);
      if ((*it) == AProxyModel)
        return index;
      it++;
    }
  }
  return index;
}

QModelIndex RostersView::mapFromProxy(QAbstractProxyModel *AProxyModel, const QModelIndex &AProxyIndex)
{
  QModelIndex index = AProxyIndex;
  if (FProxyModels.count() > 0)
  {
    bool doMap = false;
    QList<QAbstractProxyModel *>::const_iterator it = FProxyModels.constEnd();
    do 
    {
      it--;
      if ((*it) == AProxyModel)
        doMap = true;
      if (doMap)
        index = (*it)->mapToSource(index);
    } while(it != FProxyModels.constBegin());
  }
  return index;
}

int RostersView::createIndexLabel(int AOrder, const QVariant &ALabel, int AFlags)
{
  int labelId = 0;
  if (ALabel.isValid())
  {
    while (labelId <= DISPLAY_LABEL_ID || FIndexLabels.contains(labelId))
      labelId = (rand()<<16)+rand();
    FIndexLabels.insert(labelId,ALabel);
    FIndexLabelOrders.insert(labelId,AOrder);
    FIndexLabelFlags.insert(labelId,AFlags);
    if (AFlags & IRostersView::LabelBlink)
      appendBlinkLabel(labelId);
  }
  return labelId;
}

void RostersView::updateIndexLabel(int ALabelId, const QVariant &ALabel, int AFlags)
{
  if (ALabel.isValid() && FIndexLabels.contains(ALabelId) && FIndexLabels.value(ALabelId)!=ALabel)
  {
    FIndexLabels[ALabelId] = ALabel;
    FIndexLabelFlags[ALabelId] = AFlags;
    foreach (IRosterIndex *index, FIndexLabelIndexes.value(ALabelId))
    {
      QList<QVariant> ids = index->data(IRosterIndex::DR_LabelIds).toList();
      QList<QVariant> labels = index->data(IRosterIndex::DR_LabelValues).toList();
      QList<QVariant> flags = index->data(IRosterIndex::DR_LabelFlags).toList();
      int i = 0;
      while (ids.at(i).toInt()!=ALabelId) i++;
      labels[i] = ALabel;
      flags[i] = AFlags;
      if (AFlags & IRostersView::LabelBlink)
        appendBlinkLabel(ALabelId);
      else
        removeBlinkLabel(ALabelId);
      index->setData(IRosterIndex::DR_LabelValues,labels);
      index->setData(IRosterIndex::DR_LabelFlags,flags);
    }
  }
}

void RostersView::insertIndexLabel(int ALabelId, IRosterIndex *AIndex)
{
  if (AIndex && FIndexLabels.contains(ALabelId) && !FIndexLabelIndexes.value(ALabelId).contains(AIndex))
  {
    QList<QVariant> ids = AIndex->data(IRosterIndex::DR_LabelIds).toList();
    QList<QVariant> labels = AIndex->data(IRosterIndex::DR_LabelValues).toList();
    QList<QVariant> orders = AIndex->data(IRosterIndex::DR_LabelOrders).toList();
    QList<QVariant> flags = AIndex->data(IRosterIndex::DR_LabelFlags).toList();
    int i = 0;
    int order = FIndexLabelOrders.value(ALabelId);
    while (i<orders.count() && orders.at(i).toInt() < order) i++;
    ids.insert(i,ALabelId);
    orders.insert(i,order);
    labels.insert(i,FIndexLabels.value(ALabelId)); 
    flags.insert(i,FIndexLabelFlags.value(ALabelId));
    FIndexLabelIndexes[ALabelId] += AIndex;
    AIndex->setData(IRosterIndex::DR_LabelIds,ids);
    AIndex->setData(IRosterIndex::DR_LabelValues,labels);
    AIndex->setData(IRosterIndex::DR_LabelFlags,flags);
    AIndex->setData(IRosterIndex::DR_LabelOrders,orders);
  }
}

void RostersView::removeIndexLabel(int ALabelId, IRosterIndex *AIndex)
{
  if (AIndex && FIndexLabels.contains(ALabelId) && FIndexLabelIndexes.value(ALabelId).contains(AIndex))
  {
    QList<QVariant> ids = AIndex->data(IRosterIndex::DR_LabelIds).toList();
    QList<QVariant> labels = AIndex->data(IRosterIndex::DR_LabelValues).toList();
    QList<QVariant> orders = AIndex->data(IRosterIndex::DR_LabelOrders).toList();
    QList<QVariant> flags = AIndex->data(IRosterIndex::DR_LabelFlags).toList();
    int i = 0;
    while (i<ids.count() && ids.at(i).toInt()!=ALabelId) i++;
    ids.removeAt(i);
    orders.removeAt(i);
    labels.removeAt(i);
    flags.removeAt(i);
    FIndexLabelIndexes[ALabelId] -= AIndex;
    if (FIndexLabelIndexes[ALabelId].isEmpty())
      FIndexLabelIndexes.remove(ALabelId);
    AIndex->setData(IRosterIndex::DR_LabelOrders,orders);
    AIndex->setData(IRosterIndex::DR_LabelFlags,flags);
    AIndex->setData(IRosterIndex::DR_LabelValues,labels);
    AIndex->setData(IRosterIndex::DR_LabelIds,ids);
  }
}

void RostersView::destroyIndexLabel(int ALabelId)
{
  if (FIndexLabels.contains(ALabelId))
  {
    removeBlinkLabel(ALabelId);
    foreach (IRosterIndex *index, FIndexLabelIndexes.value(ALabelId))
      removeIndexLabel(ALabelId,index);
    FIndexLabels.remove(ALabelId);
    FIndexLabelOrders.remove(ALabelId);
    FIndexLabelFlags.remove(ALabelId);
    FIndexLabelIndexes.remove(ALabelId);
  }
}

int RostersView::labelAt(const QPoint &APoint, const QModelIndex &AIndex) const
{
  if (itemDelegate(AIndex) != FRosterIndexDelegate)
    return DISPLAY_LABEL_ID;

  return FRosterIndexDelegate->labelAt(APoint,indexOption(AIndex),AIndex);
}

QRect RostersView::labelRect(int ALabeld, const QModelIndex &AIndex) const
{
  if (itemDelegate(AIndex) != FRosterIndexDelegate)
    return QRect();

  return FRosterIndexDelegate->labelRect(ALabeld,indexOption(AIndex),AIndex);
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
  QModelIndex modelIndex = indexAt(AEvent->pos());
  if (modelIndex.isValid())
  {
    const int labelId = labelAt(AEvent->pos(),modelIndex);

    modelIndex = mapToModel(modelIndex);
    IRosterIndex *index = static_cast<IRosterIndex *>(modelIndex.internalPointer());

    FContextMenu->clear();
    if (labelId > DISPLAY_LABEL_ID)
      emit labelContextMenu(index,labelId,FContextMenu);
    if (FContextMenu->isEmpty())
      emit contextMenu(index,FContextMenu);
    if (!FContextMenu->isEmpty())
      FContextMenu->popup(AEvent->globalPos());
  }
}

QStyleOptionViewItemV2 RostersView::indexOption(const QModelIndex &AIndex) const
{
  QStyleOptionViewItemV2 option = viewOptions();
  option.rect = visualRect(AIndex);
  option.showDecorationSelected |= selectionBehavior() & SelectRows;
  option.state |= isExpanded(AIndex) ? QStyle::State_Open : QStyle::State_None;
  if (hasFocus() && currentIndex() == AIndex)
    option.state |= QStyle::State_HasFocus;
  if (selectedIndexes().contains(AIndex))
    option.state |= QStyle::State_Selected;
  if ((AIndex.flags() & Qt::ItemIsEnabled) == 0)
    option.state &= ~QStyle::State_Enabled;
  if (indexAt(viewport()->mapFromGlobal(QCursor::pos())) == AIndex)
    option.state |= QStyle::State_MouseOver;
  return option;
}

void RostersView::appendBlinkLabel(int ALabelId)
{
  FBlinkLabels+=ALabelId;
  FRosterIndexDelegate->appendBlinkLabel(ALabelId);
  if (!FBlinkTimer.isActive())
    FBlinkTimer.start();
}

void RostersView::removeBlinkLabel(int ALabelId)
{
  FBlinkLabels-=ALabelId;
  FRosterIndexDelegate->removeBlinkLabel(ALabelId);
  if (FBlinkLabels.isEmpty() && FBlinkTimer.isActive())
    FBlinkTimer.stop();
}

bool RostersView::viewportEvent(QEvent *AEvent)
{
  switch(AEvent->type())
  {
  case QEvent::ToolTip:
    {
      QHelpEvent *helpEvent = static_cast<QHelpEvent *>(AEvent); 
      QModelIndex modelIndex = indexAt(helpEvent->pos());
      if (modelIndex.isValid())
      {
        QMultiMap<int,QString> toolTipsMap;
        const int labelId = labelAt(helpEvent->pos(),modelIndex);
        
        modelIndex = mapToModel(modelIndex);
        IRosterIndex *index = static_cast<IRosterIndex *>(modelIndex.internalPointer());

        if (labelId > DISPLAY_LABEL_ID)
          emit labelToolTips(index,labelId,toolTipsMap);

        if (toolTipsMap.isEmpty())
        {
          toolTipsMap.insert(ROSTERSVIEW_TOOLTIPORDER,index->data(Qt::ToolTipRole).toString());
          emit toolTips(index,toolTipsMap);
        }

        QString toolTipStr;
        QMultiMap<int,QString>::const_iterator it = toolTipsMap.constBegin();
        while (it != toolTipsMap.constEnd())
        {
          if (!it.value().isEmpty())
            toolTipStr += it.value() +"<br>";
          it++;
        }
        toolTipStr.chop(4);
        QToolTip::showText(helpEvent->globalPos(),toolTipStr,this);
        return true;
      }
    }
  default:
    return QTreeView::viewportEvent(AEvent);
  }
}

void RostersView::mouseDoubleClickEvent(QMouseEvent *AEvent)
{
  if (viewport()->rect().contains(AEvent->pos()))
  {
    QModelIndex modelIndex = indexAt(AEvent->pos());
    const int labelId = labelAt(AEvent->pos(),modelIndex);
    if (modelIndex.isValid() && labelId > DISPLAY_LABEL_ID)
    {
      modelIndex = mapToModel(modelIndex);
      IRosterIndex *index = static_cast<IRosterIndex *>(modelIndex.internalPointer());
     
      bool accepted = false;
      emit labelDoubleClicked(index,labelId,accepted);
      if (accepted) 
        return;
    }
  }
  QTreeView::mouseDoubleClickEvent(AEvent);
}

void RostersView::mousePressEvent(QMouseEvent *AEvent)
{
  if (viewport()->rect().contains(AEvent->pos()))
  {
    QModelIndex modelIndex = indexAt(AEvent->pos());
    const int labelId = labelAt(AEvent->pos(),modelIndex);
    if (modelIndex.isValid() && labelId > DISPLAY_LABEL_ID)
    {
      modelIndex = mapToModel(modelIndex);
      FPressedIndex = static_cast<IRosterIndex *>(modelIndex.internalPointer());
      FPressedLabel = labelId;
    }
  }
  QTreeView::mousePressEvent(AEvent);
}

void RostersView::mouseReleaseEvent(QMouseEvent *AEvent)
{
  if (viewport()->rect().contains(AEvent->pos()))
  {
    QModelIndex modelIndex = indexAt(AEvent->pos());
    const int labelId = labelAt(AEvent->pos(),modelIndex);
    if (modelIndex.isValid() && labelId > DISPLAY_LABEL_ID)
    {
      modelIndex = mapToModel(modelIndex);
      IRosterIndex *index = static_cast<IRosterIndex *>(modelIndex.internalPointer());
      if (FPressedIndex == index && FPressedLabel == labelId)
      {
        emit labelClicked(index,labelId);
      }
    }
  }
  FPressedLabel = DISPLAY_LABEL_ID;
  FPressedIndex = NULL;
  QTreeView::mouseReleaseEvent(AEvent);
}

void RostersView::onIndexCreated(IRosterIndex *AIndex, IRosterIndex *)
{
  if (AIndex->type() < IRosterIndex::IT_UserDefined)
    AIndex->setDataHolder(FIndexDataHolder);
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

void RostersView::onBlinkTimer()
{
  FBlinkShow = !FBlinkShow;
  FRosterIndexDelegate->setShowBlinkLabels(FBlinkShow);
  foreach(int labelId,FBlinkLabels)
  {
    foreach(IRosterIndex *index, FIndexLabelIndexes.value(labelId))
    {
      QModelIndex modelIndex = mapFromModel(FRostersModel->modelIndexByRosterIndex(index));
      if (modelIndex.isValid())
      {
        QRect rect = visualRect(modelIndex).adjusted(1,1,-1,-1);
        if (!rect.isEmpty())
          viewport()->repaint(rect);
      }
    }
  }
}

