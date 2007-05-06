#ifndef ROSTERINDEXDELEGATE_H
#define ROSTERINDEXDELEGATE_H

#include <QStyle>
#include <QAbstractItemDelegate>
#include "../../interfaces/irostersview.h"

class RosterIndexDelegate : 
  public QAbstractItemDelegate
{
  Q_OBJECT;

public:
  RosterIndexDelegate(QObject *AParent);
  ~RosterIndexDelegate();

	virtual void paint(QPainter *APainter, const QStyleOptionViewItem &AOption, 
    const QModelIndex &AIndex) const;
	virtual QSize sizeHint(const QStyleOptionViewItem &AOption, 
    const QModelIndex &AIndex) const;
  
  int labelAt(const QPoint &APoint, const QStyleOptionViewItem &AOption,  
    const QModelIndex &AIndex) const;
protected:
  QRect drawVariant(QPainter *APainter, const QStyleOptionViewItem &AOption, 
    const QRect &ARect, const QVariant &AValue) const;
  void drawBackground(QPainter *APainter, const QStyleOptionViewItem &AOption, 
    const QModelIndex &AIndex) const;
  void drawFocus(QPainter *APainter, const QStyleOptionViewItem &AOption, 
    const QRect &ARect) const;
  QMultiMap<int,QVariant> labelsMap(const QModelIndex &AIndex) const;
  QStyleOptionViewItem setOptions(const QModelIndex &AIndex,
    const QStyleOptionViewItem &AOption) const;
  QSize variantSize(const QStyleOptionViewItem &AOption, const QVariant &AValue) const;
  QRect variantRect(const QStyleOptionViewItem &AOption, const QRect &ARect, const QVariant &AValue) const;
  QSize doTextLayout(QTextLayout &ATextLayout, int ALineWidth = INT_MAX>>6) const;
private:
  static const int spacing = 2;
  QIcon::Mode getIconMode(QStyle::State AState) const;
  QIcon::State getIconState(QStyle::State AState) const;
};

#endif // ROSTERINDEXDELEGATE_H
