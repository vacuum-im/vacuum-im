#ifndef ROSTERINDEXDELEGATE_H
#define ROSTERINDEXDELEGATE_H

#include <QStyle>
#include <QItemDelegate>
#include "../../interfaces/irostersview.h"
#include "../../utils/skin.h"

class RosterIndexDelegate : 
  public QItemDelegate
{
  Q_OBJECT;

public:
  RosterIndexDelegate(QObject *AParent);
  ~RosterIndexDelegate();

	virtual void paint(QPainter *APainter, const QStyleOptionViewItem &AOption, 
    const QModelIndex &AIndex) const;
	virtual QSize sizeHint(const QStyleOptionViewItem &AOption, 
    const QModelIndex &AIndex) const;
protected:
  virtual QRect drawBackground(QPainter *APainter, const QStyleOptionViewItem &AOption, 
    const QModelIndex &AIndex, const QRect &ARect) const;
  virtual QRect drawDecoration(QPainter *APainter, const QStyleOptionViewItem &AOption, 
    const QModelIndex &AIndex, const QRect &ARect) const;
  virtual QRect drawDisplay(QPainter *APainter, const QStyleOptionViewItem &AOption, 
    const QModelIndex &AIndex, const QRect &ARect) const;
  virtual void drawFocus(QPainter *APainter, const QStyleOptionViewItem &AOption, 
    const QRect &ARect) const;
  virtual QStyleOptionViewItem setOptions(const QModelIndex &AIndex,
                                          const QStyleOptionViewItem &AOption) const;
private:
  static QIcon::Mode getIconMode(QStyle::State AState);
  static QIcon::State getIconState(QStyle::State AState);
private:
  SkinIconset FRosterIconset;
};

#endif // ROSTERINDEXDELEGATE_H
