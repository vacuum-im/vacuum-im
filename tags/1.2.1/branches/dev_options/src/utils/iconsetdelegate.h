#ifndef ICONSETDELEGATE_H
#define ICONSETDELEGATE_H

#include <QHash>
#include <QPainter>
#include <QModelIndex>
#include <QItemDelegate>
#include "utilsexport.h"
#include "iconstorage.h"

enum IconsetDataRoles {
  IDR_STORAGE_NAME        = Qt::UserRole,
  IDR_STORAGE_SUBDIR,
  IDR_ICON_ROWS,
  IDR_HIDE_ICONSET_NAME
};

class UTILS_EXPORT IconsetDelegate :
  public QItemDelegate
{
public:
  IconsetDelegate(QObject *AParent = NULL);
  virtual ~IconsetDelegate();
  virtual void paint(QPainter *APainter, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
  virtual QSize sizeHint(const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
protected:
  virtual void drawBackground(QPainter *APainter, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
  virtual bool editorEvent(QEvent *AEvent, QAbstractItemModel *AModel, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex);
protected:
  void uniqueKeys(IconStorage *AStorage, QList<QString> &AKeys) const;
private:
  mutable QHash<QString, QHash<QString, IconStorage *> > FStorages;
};

#endif // ICONSETDELEGATE_H
