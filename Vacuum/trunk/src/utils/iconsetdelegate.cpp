#include "iconsetdelegate.h"

#include "iconset.h"
#include "skin.h"

IconsetDelegate::IconsetDelegate(QObject *AParent)
  : QItemDelegate(AParent)
{

}

void IconsetDelegate::paint(QPainter *APainter, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const
{
  QString iconFile = AIndex.data(Qt::DisplayRole).toString();
  Iconset iconset = Skin::getIconset(iconFile);
  if (iconset.isValid())
  {
    APainter->save();
    APainter->setClipRect(AOption.rect);
    drawBackground(APainter,AOption,AIndex);

    int space = 2;
    QRect drawRect = AOption.rect.adjusted(space,space,-space,-space);

    if (!AIndex.data(IDR_HIDE_ICONSET_NAME).toBool())
    {
      QString displayText = iconset.iconsetName();
      if (displayText.isEmpty()) 
        displayText = iconFile;
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
    }

    int maxColumn = AIndex.data(IDR_MAX_ICON_COLUMNS).isValid() ? AIndex.data(IDR_MAX_ICON_COLUMNS).toInt() : DEFAULT_MAX_COLUMNS;
    int maxRows = AIndex.data(IDR_MAX_ICON_ROWS).isValid() ? AIndex.data(IDR_MAX_ICON_ROWS).toInt() : DEFAULT_MAX_ROWS;

    int row = 0;
    int column = 0;
    int iconIndex = 0;
    int left = drawRect.left();
    int top = drawRect.top();
    QList<QString> iconNames = iconset.iconNames();
    while (drawRect.bottom()>top && drawRect.right()>left && iconIndex<iconNames.count() && row < maxRows)
    {
      QIcon icon = iconset.iconByName(iconNames.at(iconIndex));
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

    QItemDelegate::drawFocus(APainter,AOption,AOption.rect);
    APainter->restore();
  }
  else
    QItemDelegate::paint(APainter,AOption,AIndex);
}

QSize IconsetDelegate::sizeHint(const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const
{
  QString iconFile = AIndex.data(Qt::DisplayRole).toString();
  Iconset iconset = Skin::getIconset(iconFile);
  if (iconset.isValid())
  {
    QSize size(0,0);

    if (!AIndex.data(IDR_HIDE_ICONSET_NAME).toBool())
      size = AOption.fontMetrics.size(Qt::TextSingleLine,iconFile);

    int space = 2;
    int iconCount = iconset.iconNames().count();
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
