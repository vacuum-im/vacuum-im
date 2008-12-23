#include "iconsetdelegate.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include "iconset.h"
#include "skin.h"

#define DEFAULT_MAX_COLUMNS           15
#define DEFAULT_MAX_ROWS              1


IconsetDelegate::IconsetDelegate(QObject *AParent)
  : QItemDelegate(AParent)
{

}

void IconsetDelegate::paint(QPainter *APainter, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const
{
  QString iconFile = AIndex.data(Qt::DisplayRole).toString();
  SkinIconset *iconset = Skin::getSkinIconset(iconFile);
  if (iconset->isValid())
  {
    APainter->save();
    if (hasClipping())
      APainter->setClipRect(AOption.rect);

    drawBackground(APainter,AOption,AIndex);

    int space = 2;
    QRect drawRect = AOption.rect.adjusted(space,space,-space,-space);

    if (!AIndex.data(IDR_HIDE_ICONSET_NAME).toBool())
    {
      QRect checkRect(drawRect.topLeft(),check(AOption,drawRect,AIndex.data(Qt::CheckStateRole)).size());
      drawCheck(APainter,AOption,checkRect,static_cast<Qt::CheckState>(AIndex.data(Qt::CheckStateRole).toInt()));
      drawRect.setLeft(checkRect.right()+space);

      QString displayText = !iconset->info().name.isEmpty() ? iconset->info().name : iconFile;
      QRect textRect(drawRect.topLeft(),AOption.fontMetrics.size(Qt::TextSingleLine,displayText));

      QPalette::ColorGroup cg = AOption.state & QStyle::State_Enabled ? QPalette::Normal : QPalette::Disabled;
      if (cg == QPalette::Normal && !(AOption.state & QStyle::State_Active))
        cg = QPalette::Inactive;
      if (AOption.state & QStyle::State_Selected)
        APainter->setPen(AOption.palette.color(cg, QPalette::HighlightedText));
      else
        APainter->setPen(AOption.palette.color(cg, QPalette::Text));
      APainter->drawText(textRect,AOption.displayAlignment,displayText);

      drawRect.setTop(textRect.bottom() + space);
      drawRect.setLeft(AOption.rect.left() + space);
    }

    int maxColumn = AIndex.data(IDR_MAX_ICON_COLUMNS).isValid() ? AIndex.data(IDR_MAX_ICON_COLUMNS).toInt() : DEFAULT_MAX_COLUMNS;
    int maxRows = AIndex.data(IDR_MAX_ICON_ROWS).isValid() ? AIndex.data(IDR_MAX_ICON_ROWS).toInt() : DEFAULT_MAX_ROWS;

    int row = 0;
    int column = 0;
    int iconIndex = 0;
    int left = drawRect.left();
    int top = drawRect.top();
    QList<QString> iconFiles = iconset->iconFiles();
    while (drawRect.bottom()>top && drawRect.right()>left && iconIndex<iconFiles.count() && row < maxRows)
    {
      QIcon icon = iconset->iconByFile(iconFiles.at(iconIndex));
      if (!icon.isNull())
      {
        QPixmap pixmap = icon.pixmap(AOption.decorationSize,QIcon::Normal,QIcon::On);
        APainter->drawPixmap(left,top,pixmap);
        left+=AOption.decorationSize.width()+space;
      }
      iconIndex++;
      column++;

      if (left >= drawRect.right() || column>=maxColumn)
      {
        column = 0;
        left = drawRect.left();
        top+=AOption.decorationSize.height()+space;
      }
    }

    drawFocus(APainter,AOption,AOption.rect);
    APainter->restore();
  }
  else
    QItemDelegate::paint(APainter,AOption,AIndex);
}

QSize IconsetDelegate::sizeHint(const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const
{
  QString iconFile = AIndex.data(Qt::DisplayRole).toString();
  SkinIconset *iconset = Skin::getSkinIconset(iconFile);
  if (iconset->isValid())
  {
    int space = 2;
    QSize size(0,0);

    if (!AIndex.data(IDR_HIDE_ICONSET_NAME).toBool())
    {
      QSize checkSize = check(AOption,AOption.rect,AIndex.data(Qt::CheckStateRole)).size();
      QString displayText = !iconset->info().name.isEmpty() ? iconset->info().name : iconFile;
      QSize textSize = AOption.fontMetrics.size(Qt::TextSingleLine,iconFile);
      size.setHeight(qMax(checkSize.height(),textSize.height()));
      size.setWidth(checkSize.width()+textSize.width()+space);
    }

    int iconCount = iconset->iconFiles().count();
    int maxColumn = AIndex.data(IDR_MAX_ICON_COLUMNS).isValid() ? AIndex.data(IDR_MAX_ICON_COLUMNS).toInt() : DEFAULT_MAX_COLUMNS;
    int maxRow = AIndex.data(IDR_MAX_ICON_ROWS).isValid() ? AIndex.data(IDR_MAX_ICON_ROWS).toInt() : DEFAULT_MAX_ROWS;
    int iconWidth = (AOption.decorationSize.width()+space)*qMin(maxColumn,iconCount);
    int iconHeight = (AOption.decorationSize.height()+space)*qMin(maxRow,(iconCount-1)/maxColumn+1);
    return QSize(qMax(size.width(),iconWidth)+space,size.height()+iconHeight+space);
  }
  else
    return QItemDelegate::sizeHint(AOption,AIndex);
}

void IconsetDelegate::drawBackground(QPainter *APainter, const QStyleOptionViewItem &AOption, const QModelIndex &/*AIndex*/) const
{
  if (AOption.state & QStyle::State_Selected)
  {
    QPalette::ColorGroup cg = AOption.state & QStyle::State_Enabled ? QPalette::Normal : QPalette::Disabled;
    if (cg == QPalette::Normal && !(AOption.state & QStyle::State_Active))
      cg = QPalette::Inactive;
    APainter->fillRect(AOption.rect, AOption.palette.brush(cg, QPalette::Highlight));
  }
}

bool IconsetDelegate::editorEvent(QEvent *AEvent, QAbstractItemModel *AModel, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex)
{
  Qt::ItemFlags flags = AModel->flags(AIndex);
  if (!(flags & Qt::ItemIsUserCheckable) || !(AOption.state & QStyle::State_Enabled) || !(flags & Qt::ItemIsEnabled))
    return false;

  QVariant value = AIndex.data(Qt::CheckStateRole);
  if (!value.isValid())
    return false;


  if ((AEvent->type() == QEvent::MouseButtonRelease) || (AEvent->type() == QEvent::MouseButtonDblClick))
  {
    int space = 2;
    QRect drawRect = AOption.rect.adjusted(space,space,-space,-space);
    QRect checkRect(drawRect.topLeft(),check(AOption, AOption.rect, Qt::Checked).size());

    if (!checkRect.contains(static_cast<QMouseEvent*>(AEvent)->pos()))
      return false;

    if (AEvent->type() == QEvent::MouseButtonDblClick)
      return true;
  }
  else if (AEvent->type() == QEvent::KeyPress)
  {
    QKeyEvent *keyEvent = static_cast<QKeyEvent*>(AEvent);
    if (keyEvent->key() != Qt::Key_Space && keyEvent->key() != Qt::Key_Select)
      return false;
  }
  else
    return false;

  Qt::CheckState state = (static_cast<Qt::CheckState>(value.toInt()) == Qt::Checked ? Qt::Unchecked : Qt::Checked);
  return AModel->setData(AIndex, state, Qt::CheckStateRole);
}
