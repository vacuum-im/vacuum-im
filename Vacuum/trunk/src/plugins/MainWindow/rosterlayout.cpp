#include "rosterlayout.h"

RosterLayout::RosterLayout(QWidget *AParent)
  : QLayout(AParent)
{
  setSpacing(0);
}

RosterLayout::~RosterLayout()
{
  qDeleteAll(FItemList);
}

void RosterLayout::addItem( QLayoutItem *AItem )
{
  FItemList.append(AItem);
}

QSize RosterLayout::sizeHint() const
{
  QSize size(0,0);
  for (int i =0; i < count(); ++i)
  {
    size.rwidth() = qMax(size.width(),itemAt(i)->sizeHint().width());
    size.rheight() += itemAt(i)->sizeHint().height();
  }

  if (count() > 0)
    size.rheight() += (count()-1)*spacing(); 

  return size;
}

QSize RosterLayout::minimumSize() const
{
  return sizeHint();
}

QLayoutItem * RosterLayout::itemAt( int AIndex ) const
{
  return FItemList.value(AIndex);
}

QLayoutItem * RosterLayout::takeAt( int AIndex )
{
  if (AIndex>=0 && AIndex<FItemList.count())
    return FItemList.takeAt(AIndex);
  return 0;
}

void RosterLayout::setGeometry( const QRect &ARect )
{
  if (FItemList.count() == 0)
  {
    QLayout::setGeometry(ARect);
    return;
  }
  
  int height = 0;
  int width = ARect.width();
  for (int i =0; i < count(); ++i)
    width = qMax(width,itemAt(i)->sizeHint().width());

  for (int i =0; i < count(); ++i)
  {
    QLayoutItem *item = itemAt(i);
    item->setGeometry(QRect(ARect.x(),ARect.y()+height,width,item->sizeHint().height()));
    height += item->sizeHint().height() + spacing();
  }

  QLayout::setGeometry(QRect(ARect.x(),ARect.y(),width,height-spacing()));
}