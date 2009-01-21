#ifndef ROSTERINDEXDELEGATE_H
#define ROSTERINDEXDELEGATE_H

#include <QStyle>
#include <QAbstractItemDelegate>
#include "../../definations/rosterlabelorders.h"
#include "../../definations/rosterindextyperole.h"
#include "../../interfaces/irostersview.h"

struct LabelItem
{
  LabelItem() { id =-1; order = 0; flags = 0; }
  int id;
  int order;
  int flags;
  QSize size;
  QRect rect;
  QVariant value;
  bool operator <(const LabelItem &AItem) const {
    return order < AItem.order;
  }
};

class RosterIndexDelegate : 
  public QAbstractItemDelegate
{
  Q_OBJECT;
public:
  RosterIndexDelegate(QObject *AParent);
  ~RosterIndexDelegate();
  //QAbstractItemDelegate
	virtual void paint(QPainter *APainter, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
	virtual QSize sizeHint(const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
  //RosterIndexDelegate
  int labelAt(const QPoint &APoint, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
  QRect labelRect(int ALabelId, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
  void setShowBlinkLabels(bool AShow) { FShowBlinkLabels = AShow; }
protected:
  QHash<int,QRect> drawIndex(QPainter *APainter, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
  void drawLabelItem(QPainter *APainter, const QStyleOptionViewItem &AOption, const LabelItem &ALabel) const;
  void drawBackground(QPainter *APainter, const QStyleOptionViewItem &AOption, const QRect &ARect, const QModelIndex &AIndex) const;
  void drawFocus(QPainter *APainter, const QStyleOptionViewItem &AOption, const QRect &ARect) const;
  QStyleOptionViewItem indexOptions(const QModelIndex &AIndex, const QStyleOptionViewItem &AOption) const;
  QStyleOptionViewItem indexFooterOptions(const QStyleOptionViewItem &AOption) const;
  QList<LabelItem> itemLabels(const QModelIndex &AIndex) const;
  QList<LabelItem> itemFooters(const QModelIndex &AIndex) const;
  QSize variantSize(const QStyleOptionViewItem &AOption, const QVariant &AValue) const;
  void getLabelsSize(const QStyleOptionViewItem &AOption, QList<LabelItem> &ALabels) const;
  void removeWidth(QRect &ARect, int AWidth, bool AIsLeftToRight) const;
private:
  static const int spacing = 2;
  QIcon::Mode getIconMode(QStyle::State AState) const;
  QIcon::State getIconState(QStyle::State AState) const;
private:
  bool FShowBlinkLabels;
};

#endif // ROSTERINDEXDELEGATE_H
