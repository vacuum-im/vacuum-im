#include "rosterindexdelegate.h"
#include <QPainter>

RosterIndexDelegate::RosterIndexDelegate(QObject *AParent)
  : QItemDelegate(AParent)
{

}

RosterIndexDelegate::~RosterIndexDelegate()
{

}

void RosterIndexDelegate::paint(QPainter *APainter, 
                                const QStyleOptionViewItem &AOption,  
                                const QModelIndex &AIndex) const
{
  QItemDelegate::paint(APainter,AOption,AIndex);
  return;

  IRosterIndex *rosterIndex = static_cast<IRosterIndex *>(AIndex.internalPointer());
  if (rosterIndex && rosterIndex->itemDelegate())
  {
    rosterIndex->itemDelegate()->paint(APainter,AOption,AIndex);
    return;
  }

  APainter->save();

  QRect freeRect = drawBackground(APainter,AOption,AIndex,AOption.rect);
  
  QRect usedRect = drawDecoration(APainter,AOption,AIndex,freeRect); 
  if (!usedRect.isNull())
    freeRect.setLeft(usedRect.left()+2);
  
  usedRect = drawDisplay(APainter,AOption,AIndex,freeRect);
  if (!usedRect.isNull())
    freeRect.setLeft(usedRect.left()+2);

  APainter->restore();
}

QSize RosterIndexDelegate::sizeHint(const QStyleOptionViewItem &AOption,  
                                    const QModelIndex &AIndex) const
{
  IRosterIndex *rosterIndex = static_cast<IRosterIndex *>(AIndex.internalPointer());
  if (rosterIndex && rosterIndex->itemDelegate())
    return rosterIndex->itemDelegate()->sizeHint(AOption,AIndex);

  return QItemDelegate::sizeHint(AOption,AIndex);
}

QRect RosterIndexDelegate::drawBackground(QPainter *APainter, const QStyleOptionViewItem &AOption,  
                                          const QModelIndex &AIndex, const QRect &/*ARect*/) const
{
  QItemDelegate::drawBackground(APainter,AOption,AIndex);
  return AOption.rect;
}

QRect RosterIndexDelegate::drawDecoration(QPainter *APainter, const QStyleOptionViewItem &AOption,  
                                          const QModelIndex &AIndex, const QRect &ARect ) const
{
  QVariant data = AIndex.data(Qt::DecorationRole);
  if (data.isValid() && data.canConvert(QVariant::Icon))
  {
    QRect iconRect = ARect.intersected(QRect(ARect.topLeft(),rect(AOption,AIndex,Qt::DecorationRole).size()));
    QItemDelegate::drawDecoration(APainter,AOption,iconRect,decoration(AOption,data));
    return iconRect;
  }
  return QRect();
}

QRect RosterIndexDelegate::drawDisplay(QPainter *APainter, const QStyleOptionViewItem &AOption,  
                                       const QModelIndex &AIndex, const QRect &ARect) const
{
  QVariant data = AIndex.data(Qt::DisplayRole);
  if (data.isValid() && data.canConvert(QVariant::String))
  {
    QString text = data.toString();
    if (!text.isEmpty())
    {
      QRect textRect = ARect.intersected(QRect(ARect.topLeft(),rect(AOption,AIndex,Qt::DisplayRole).size()));
      QItemDelegate::drawDisplay(APainter,AOption,textRect,text);
      return textRect;
    }
  }
  return QRect();
}

QIcon::Mode RosterIndexDelegate::getIconMode( QStyle::State AState )
{
  if (!(AState & QStyle::State_Enabled)) return QIcon::Disabled;
  if (AState & QStyle::State_Selected) return QIcon::Selected;
  return QIcon::Normal;
}

QIcon::State RosterIndexDelegate::getIconState( QStyle::State AState )
{
  return AState & QStyle::State_Open ? QIcon::On : QIcon::Off;
}
