#include <QtDebug>
#include "rostersview.h"

#include <QApplication>
#include <QHeaderView>
#include <QToolTip>
#include <QCursor>
#include <QHelpEvent>

RostersView::RostersView(QWidget *AParent)
  : QTreeView(AParent)
{
  FLabelId = 1;
  FHookerId = 1;
  FOptions = 0;

  FRostersModel = NULL;
  FPressedLabel = RLID_DISPLAY;
  FPressedIndex = NULL;

  FContextMenu = new Menu(this);
  FRosterIndexDelegate = new RosterIndexDelegate(this);

  FBlinkShow = true;
  FBlinkTimer.setInterval(500);
  connect(&FBlinkTimer,SIGNAL(timeout()),SLOT(onBlinkTimer()));

  header()->hide();
  setIndentation(4);
  setRootIsDecorated(false);
  setSelectionMode(NoSelection);
  setSortingEnabled(true);
  setContextMenuPolicy(Qt::DefaultContextMenu);
  setItemDelegate(FRosterIndexDelegate);
}

RostersView::~RostersView()
{
  removeLabels();
  while (FClickHookerItems.count()>0)
    destroyClickHooker(FClickHookerItems.at(0)->hookerId);
}

void RostersView::setModel(IRostersModel *AModel)
{
  if (FRostersModel != AModel)
  {
    emit modelAboutToBeSeted(AModel);
    
    if (FRostersModel)
    {
      disconnect(AModel,SIGNAL(indexRemoved(IRosterIndex *)),
        this,SLOT(onIndexRemoved(IRosterIndex *)));
      removeLabels();
      removeClickHookers();
    }

    if (AModel)
    {
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

bool RostersView::repaintRosterIndex(IRosterIndex *AIndex)
{
  if (FRostersModel)
  {
    QModelIndex modelIndex = mapFromModel(FRostersModel->modelIndexByRosterIndex(AIndex));
    if (modelIndex.isValid())
    {
      QRect rect = visualRect(modelIndex).adjusted(1,1,-1,-1);
      if (!rect.isEmpty())
      {
        viewport()->repaint(rect);
        return true;
      }
    }
  }
  return false;
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
    labelId = FLabelId++;
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
      QList<QVariant> ids = index->data(RDR_LabelIds).toList();
      QList<QVariant> labels = index->data(RDR_LabelValues).toList();
      QList<QVariant> flags = index->data(RDR_LabelFlags).toList();
      int i = 0;
      while (ids.at(i).toInt()!=ALabelId) i++;
      labels[i] = ALabel;
      flags[i] = AFlags;
      if (AFlags & IRostersView::LabelBlink)
        appendBlinkLabel(ALabelId);
      else
        removeBlinkLabel(ALabelId);
      index->setData(RDR_LabelValues,labels);
      index->setData(RDR_LabelFlags,flags);
    }
  }
}

void RostersView::insertIndexLabel(int ALabelId, IRosterIndex *AIndex)
{
  if (AIndex && FIndexLabels.contains(ALabelId) && !FIndexLabelIndexes.value(ALabelId).contains(AIndex))
  {
    QList<QVariant> ids = AIndex->data(RDR_LabelIds).toList();
    QList<QVariant> labels = AIndex->data(RDR_LabelValues).toList();
    QList<QVariant> orders = AIndex->data(RDR_LabelOrders).toList();
    QList<QVariant> flags = AIndex->data(RDR_LabelFlags).toList();
    int i = 0;
    int order = FIndexLabelOrders.value(ALabelId);
    while (i<orders.count() && orders.at(i).toInt() < order) i++;
    ids.insert(i,ALabelId);
    orders.insert(i,order);
    labels.insert(i,FIndexLabels.value(ALabelId)); 
    flags.insert(i,FIndexLabelFlags.value(ALabelId));
    FIndexLabelIndexes[ALabelId] += AIndex;
    AIndex->setData(RDR_LabelIds,ids);
    AIndex->setData(RDR_LabelValues,labels);
    AIndex->setData(RDR_LabelFlags,flags);
    AIndex->setData(RDR_LabelOrders,orders);
  }
}

void RostersView::removeIndexLabel(int ALabelId, IRosterIndex *AIndex)
{
  if (AIndex && FIndexLabels.contains(ALabelId) && FIndexLabelIndexes.value(ALabelId).contains(AIndex))
  {
    QList<QVariant> ids = AIndex->data(RDR_LabelIds).toList();
    QList<QVariant> labels = AIndex->data(RDR_LabelValues).toList();
    QList<QVariant> orders = AIndex->data(RDR_LabelOrders).toList();
    QList<QVariant> flags = AIndex->data(RDR_LabelFlags).toList();
    int i = 0;
    while (i<ids.count() && ids.at(i).toInt()!=ALabelId) i++;
    ids.removeAt(i);
    orders.removeAt(i);
    labels.removeAt(i);
    flags.removeAt(i);
    FIndexLabelIndexes[ALabelId] -= AIndex;
    if (FIndexLabelIndexes[ALabelId].isEmpty())
      FIndexLabelIndexes.remove(ALabelId);
    AIndex->setData(RDR_LabelOrders,orders);
    AIndex->setData(RDR_LabelFlags,flags);
    AIndex->setData(RDR_LabelValues,labels);
    AIndex->setData(RDR_LabelIds,ids);
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
    return RLID_DISPLAY;

  return FRosterIndexDelegate->labelAt(APoint,indexOption(AIndex),AIndex);
}

QRect RostersView::labelRect(int ALabeld, const QModelIndex &AIndex) const
{
  if (itemDelegate(AIndex) != FRosterIndexDelegate)
    return QRect();

  return FRosterIndexDelegate->labelRect(ALabeld,indexOption(AIndex),AIndex);
}

int RostersView::createClickHooker(IRostersClickHooker *AHooker, int APriority, bool AAutoRemove)
{
  int i = 0;
  while (i < FClickHookerItems.count() && FClickHookerItems.at(i)->priority >= APriority)
    i++;
  ClickHookerItem *item = new ClickHookerItem;
  item->hookerId = FHookerId++;
  item->hooker = AHooker;
  item->priority = APriority;
  item->autoRemove = AAutoRemove;
  FClickHookerItems.insert(i,item);
  return item->hookerId;
}

void RostersView::insertClickHooker(int AHookerId, IRosterIndex *AIndex)
{
  int i = 0;
  while (i < FClickHookerItems.count())
  {
    ClickHookerItem *item = FClickHookerItems.at(i);
    if (item->hookerId == AHookerId)
    {
      item->indexes += AIndex;
      break;
    }
    i++;
  }
}

void RostersView::removeClickHooker(int AHookerId, IRosterIndex *AIndex)
{
  int i = 0;
  while (i < FClickHookerItems.count())
  {
    ClickHookerItem *item = FClickHookerItems.at(i);
    if (item->hookerId == AHookerId)
    {
      item->indexes -= AIndex;
      break;
    }
    i++;
  }
}

void RostersView::destroyClickHooker(int AHookerId)
{
  int i = 0;
  while (i < FClickHookerItems.count())
  {
    ClickHookerItem *item = FClickHookerItems.at(i);
    if (item->hookerId == AHookerId)
    {
      FClickHookerItems.removeAt(i);
      delete item;
      break;
    }
    i++;
  }
}

void RostersView::insertFooterText(int AOrderAndId, const QString &AText, IRosterIndex *AIndex)
{
  if (!AText.isEmpty())
  {
    QString footerId = intId2StringId(AOrderAndId);
    QMap<QString,QVariant> footerMap = AIndex->data(RDR_FooterText).toMap();
    footerMap.insert(footerId, AText);
    AIndex->setData(RDR_FooterText,footerMap);
  } 
  else 
    removeFooterText(AOrderAndId,AIndex);
}

void RostersView::removeFooterText(int AOrderAndId, IRosterIndex *AIndex)
{
  QString footerId = intId2StringId(AOrderAndId);
  QMap<QString,QVariant> footerMap = AIndex->data(RDR_FooterText).toMap();
  if (footerMap.contains(footerId))
  {
    footerMap.remove(footerId);
    AIndex->setData(RDR_FooterText,footerMap);
  }
}

bool RostersView::checkOption(IRostersView::Option AOption) const
{
  return (FOptions & AOption) > 0;
}

void RostersView::setOption(IRostersView::Option AOption, bool AValue)
{
  AValue ? FOptions |= AOption : FOptions &= ~AOption;
  FRosterIndexDelegate->setOption(AOption,AValue);
}

QStyleOptionViewItemV2 RostersView::indexOption(const QModelIndex &AIndex) const
{
  QStyleOptionViewItemV2 option = viewOptions();
  option.initFrom(this);
  option.rect = visualRect(AIndex);
  
  //Костыль
  if (isRightToLeft())
  {
    QModelIndex pIndex = AIndex.parent();
    while (pIndex.isValid())
    {
      option.rect.setLeft(option.rect.left()-indentation());
      option.rect.setRight(option.rect.right()-indentation());
      pIndex = pIndex.parent();
    }
  }

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

QString RostersView::intId2StringId(int AIntId)
{
  return QString("%1").arg(AIntId,10,10,QLatin1Char('0'));
}

void RostersView::removeLabels()
{
  QList<int> labels = FIndexLabels.keys();
  foreach (int label, labels)
  {
    QSet<IRosterIndex *> indexes = FIndexLabelIndexes.value(label);
    foreach(IRosterIndex *index, indexes)
      removeIndexLabel(label,index);
  }
}

void RostersView::removeClickHookers()
{
  foreach(ClickHookerItem *hookerItem, FClickHookerItems)
  {
    QSet<IRosterIndex *> indexes = hookerItem->indexes;
    foreach(IRosterIndex *index, indexes)
      removeClickHooker(hookerItem->hookerId,index);
  }
}

void RostersView::drawBranches(QPainter * /*APainter*/, const QRect &/*ARect*/, const QModelIndex &/*AIndex*/) const
{

}

void RostersView::rowsAboutToBeRemoved(const QModelIndex &AParent, int AStart, int AEnd)
{
  emit indexAboutToBeRemoved(AParent,AStart,AEnd);
  QTreeView::rowsAboutToBeRemoved(AParent,AStart,AEnd);
}

void RostersView::rowsInserted(const QModelIndex &AParent, int AStart, int AEnd)
{
  QTreeView::rowsInserted(AParent,AStart,AEnd);
  emit indexInserted(AParent,AStart,AEnd);
}

bool RostersView::viewportEvent(QEvent *AEvent)
{
  switch(AEvent->type())
  {
  case QEvent::ToolTip:
    {
      QHelpEvent *helpEvent = static_cast<QHelpEvent *>(AEvent); 
      QModelIndex viewIndex = indexAt(helpEvent->pos());
      if (viewIndex.isValid())
      {
        QMultiMap<int,QString> toolTipsMap;
        const int labelId = labelAt(helpEvent->pos(),viewIndex);
        
        QModelIndex modelIndex = mapToModel(viewIndex);
        IRosterIndex *index = static_cast<IRosterIndex *>(modelIndex.internalPointer());

        emit labelToolTips(index,labelId,toolTipsMap);

        if (labelId == RLID_DISPLAY || labelId == RLID_FOOTER_TEXT || toolTipsMap.isEmpty())
          toolTipsMap.insert(TTO_ROSTERSVIEW,index->data(Qt::ToolTipRole).toString());

        QString toolTipStr;
        QMultiMap<int,QString>::const_iterator it = toolTipsMap.constBegin();
        while (it != toolTipsMap.constEnd())
        {
          if (!it.value().isEmpty())
            toolTipStr += it.value()+"<br>";
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

void RostersView::contextMenuEvent(QContextMenuEvent *AEvent)
{
  QModelIndex modelIndex = indexAt(AEvent->pos());
  if (modelIndex.isValid())
  {
    const int labelId = labelAt(AEvent->pos(),modelIndex);

    modelIndex = mapToModel(modelIndex);
    IRosterIndex *index = static_cast<IRosterIndex *>(modelIndex.internalPointer());

    FContextMenu->clear();
    if (labelId > RLID_DISPLAY)
      emit labelContextMenu(index,labelId,FContextMenu);
    if (FContextMenu->isEmpty())
      emit contextMenu(index,FContextMenu);
    if (!FContextMenu->isEmpty())
      FContextMenu->popup(AEvent->globalPos());
  }
}

void RostersView::mouseDoubleClickEvent(QMouseEvent *AEvent)
{
  bool accepted = false;
  if (viewport()->rect().contains(AEvent->pos()))
  {
    QModelIndex viewIndex = indexAt(AEvent->pos());
    const int labelId = labelAt(AEvent->pos(),viewIndex);
    if (viewIndex.isValid())
    {
      QModelIndex modelIndex = mapToModel(viewIndex);
      IRosterIndex *index = static_cast<IRosterIndex *>(modelIndex.internalPointer());
      if (labelId == RLID_DISPLAY)
      {
        int i = 0;
        while (!accepted && i<FClickHookerItems.count())
        {
          ClickHookerItem *item = FClickHookerItems.at(i);
          if (item->indexes.contains(index) || item->indexes.contains(NULL))
          {
            accepted = item->hooker->rosterIndexClicked(index,item->hookerId);
            if (accepted && item->autoRemove)
              destroyClickHooker(item->hookerId);
            else
              i++;
          }
          else
            i++;
        }
      }
      if (!accepted)
        emit labelDoubleClicked(index,labelId,accepted);
    }
  }
  if (!accepted)
    QTreeView::mouseDoubleClickEvent(AEvent);
}

void RostersView::mousePressEvent(QMouseEvent *AEvent)
{
  if (AEvent->button()==Qt::LeftButton && viewport()->rect().contains(AEvent->pos()))
  {
    QModelIndex viewIndex = indexAt(AEvent->pos());
    if (viewIndex.isValid())
    {
      const int labelId = labelAt(AEvent->pos(),viewIndex);
      QModelIndex modelIndex = mapToModel(viewIndex);
      FPressedIndex = static_cast<IRosterIndex *>(modelIndex.internalPointer());
      FPressedLabel = labelId;
      if (labelId == RLID_INDICATORBRANCH)
        setExpanded(viewIndex,!isExpanded(viewIndex));
    }
  }
  QTreeView::mousePressEvent(AEvent);
}

void RostersView::mouseReleaseEvent(QMouseEvent *AEvent)
{
  if (AEvent->button()==Qt::LeftButton && viewport()->rect().contains(AEvent->pos()))
  {
    QModelIndex viewIndex = indexAt(AEvent->pos());
    if (viewIndex.isValid())
    {
      const int labelId = labelAt(AEvent->pos(),viewIndex);
      QModelIndex modelIndex = mapToModel(viewIndex);
      IRosterIndex *index = static_cast<IRosterIndex *>(modelIndex.internalPointer());
      if (FPressedIndex == index && FPressedLabel == labelId)
        emit labelClicked(index,labelId);
    }
  }
  FPressedLabel = RLID_DISPLAY;
  FPressedIndex = NULL;
  QTreeView::mouseReleaseEvent(AEvent);
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

  int i = 0;
  while (i<FClickHookerItems.count())
  {
    ClickHookerItem *item = FClickHookerItems.at(i);
    if (item->indexes.contains(AIndex))
      item->indexes -= AIndex;
    i++;
  }
}

void RostersView::onBlinkTimer()
{
  FBlinkShow = !FBlinkShow;
  FRosterIndexDelegate->setShowBlinkLabels(FBlinkShow);
  foreach(int labelId,FBlinkLabels)
    foreach(IRosterIndex *index, FIndexLabelIndexes.value(labelId))
      repaintRosterIndex(index);
}

