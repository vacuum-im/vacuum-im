#include <QtDebug>
#include "rostersview.h"

#include <QHeaderView>
#include "rosterindexdelegate.h"

RostersView::RostersView(IRostersModel *AModel, QWidget *AParent)
  : QTreeView(AParent)
{
  FRostersModel = AModel;
  connect(FRostersModel,SIGNAL(streamAdded(const Jid &)),SLOT(onModelStreamAdded(const Jid &)));
  connect(FRostersModel,SIGNAL(streamRemoved(const Jid &)),SLOT(onModelStreamRemoved(const Jid &)));
  FContextMenu = new Menu(0,this);

  header()->hide();
  setIndentation(8);
  setRootIsDecorated(false);
  setSelectionMode(NoSelection);
  setSortingEnabled(true);
  setContextMenuPolicy(Qt::DefaultContextMenu);
  setItemDelegate(new RosterIndexDelegate(this));
  setModel(FRostersModel);

  FSortFilterProxyModel = new SortFilterProxyModel(this);
  FSortFilterProxyModel->setDynamicSortFilter(true);
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
    FProxyModels.removeAt(index);
    AProxyModel->setSourceModel(NULL);
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
      it--;
    }
  }
  return index;
}

bool RostersView::showOfflineContacts() const
{
  return FSortFilterProxyModel->showOffline();
}

void RostersView::setShowOfflineContacts(bool AShow)
{
  FSortFilterProxyModel->setShowOffline(AShow);
  emit showOfflineContactsChanged(AShow);
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

void RostersView::onModelStreamAdded(const Jid &)
{
  if (!FProxyModels.contains(FSortFilterProxyModel))
    addProxyModel(FSortFilterProxyModel);
  else
    FSortFilterProxyModel->clear();
}

void RostersView::onModelStreamRemoved(const Jid &)
{
  if (FRostersModel->streams().isEmpty())
    removeProxyModel(FSortFilterProxyModel);
  else
    FSortFilterProxyModel->clear();
}
