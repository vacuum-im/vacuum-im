#ifndef ICONSETDELEGATE_H
#define ICONSETDELEGATE_H

#include <QPainter>
#include <QItemDelegate>
#include <QModelIndex>
#include "utilsexport.h"

#define IDR_HIDE_ICONSET_NAME         Qt::UserRole
#define IDR_MAX_ICON_COLUMNS          Qt::UserRole+1
#define IDR_MAX_ICON_ROWS             Qt::UserRole+2

#define DEFAULT_MAX_COLUMNS           15
#define DEFAULT_MAX_ROWS              1

class UTILS_EXPORT IconsetDelegate :
  public QItemDelegate
{
public:
  IconsetDelegate(QObject *AParent = NULL);
  virtual void paint(QPainter *APainter, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
  virtual QSize sizeHint(const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
protected:
  virtual void drawBackground(QPainter *APainter, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
};

#endif // ICONSETDELEGATE_H
