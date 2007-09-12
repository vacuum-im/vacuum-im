#ifndef ROSTERINDEXDELEGATE_H
#define ROSTERINDEXDELEGATE_H

#include <QStyle>
#include <QAbstractItemDelegate>
#include "../../interfaces/irostersview.h"

typedef QMultiMap<int,QPair<int,QVariant> > LabelsMap;

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
  QRect labelRect(int ALabelId, const QStyleOptionViewItem &AOption,  
    const QModelIndex &AIndex) const;
  void appendBlinkLabel(int ALabelId) { FBlinkLabels+=ALabelId; }
  void removeBlinkLabel(int ALabelId) { FBlinkLabels-=ALabelId; }
  void setShowBlinkLabels(bool AShow) { FShowBlinkLabels = AShow; }
  bool checkOption(IRostersView::Option AOption) const;
  void setOption(IRostersView::Option AOption, bool AValue);
protected:
  QRect drawVariant(QPainter *APainter, const QStyleOptionViewItem &AOption, 
    const QRect &ARect, const QVariant &AValue) const;
  void drawBackground(QPainter *APainter, const QStyleOptionViewItem &AOption, 
    const QRect &ARect, const QModelIndex &AIndex) const;
  void drawFocus(QPainter *APainter, const QStyleOptionViewItem &AOption, 
    const QRect &ARect) const;
  LabelsMap labelsMap(const QModelIndex &AIndex) const;
  QStyleOptionViewItem setOptions(const QModelIndex &AIndex,
    const QStyleOptionViewItem &AOption) const;
  QStyleOptionViewItem setFooterOptions(const QModelIndex &AIndex,
    const QStyleOptionViewItem &AOption) const;
  QSize variantSize(const QStyleOptionViewItem &AOption, const QVariant &AValue) const;
  QRect variantRect(const QStyleOptionViewItem &AOption, const QRect &ARect, const QVariant &AValue) const;
  QSize doTextLayout(QTextLayout &ATextLayout, int ALineWidth = INT_MAX>>6) const;
private:
  static const int spacing = 2;
  QIcon::Mode getIconMode(QStyle::State AState) const;
  QIcon::State getIconState(QStyle::State AState) const;
private:
  QSet<int> FBlinkLabels;
  bool FShowBlinkLabels;
private: //Options
  int FOptions;
};

#endif // ROSTERINDEXDELEGATE_H
