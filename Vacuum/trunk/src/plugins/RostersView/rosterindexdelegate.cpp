#include "rosterindexdelegate.h"

#include <QApplication>
#include <QPainter>
#include <QTextOption>
#include <QTextLayout>

#define BRANCH_WIDTH  10

RosterIndexDelegate::RosterIndexDelegate(QObject *AParent) : QAbstractItemDelegate(AParent)
{
  FShowBlinkLabels = true;
}

RosterIndexDelegate::~RosterIndexDelegate()
{

}

void RosterIndexDelegate::paint(QPainter *APainter, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const
{
  drawIndex(APainter,AOption,AIndex);
}

QSize RosterIndexDelegate::sizeHint(const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const
{
  QStyleOptionViewItem options = indexOptions(AIndex,AOption);
  const int hMargin = QApplication::style()->pixelMetric(QStyle::PM_FocusFrameHMargin);
  const int vMargin = QApplication::style()->pixelMetric(QStyle::PM_FocusFrameVMargin);

  QList<LabelItem> labels = itemLabels(AIndex);
  QSize labelsSize = setLabelsSize(options,labels);

  if (AIndex.parent().isValid() && AIndex.model()->hasChildren(AIndex))
  {
    labelsSize.rwidth() += labelsSize.width() + BRANCH_WIDTH + spacing;
    labelsSize.rheight() = qMax(labelsSize.height(), BRANCH_WIDTH);
  }

  QList<LabelItem> lines = itemTextLines(AIndex);
  QSize linesSize = setTextLinesSize(options,lines);

  return QSize(labelsSize.width()+linesSize.width()+hMargin, qMax(labelsSize.height(),linesSize.height())+vMargin);
}

int RosterIndexDelegate::labelAt(const QPoint &APoint, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const
{
  if (!AOption.rect.contains(APoint))
    return RLID_NULL;

  QHash<int,QRect> rectHash = drawIndex(NULL,AOption,AIndex);
  QHash<int,QRect>::const_iterator it = rectHash.constBegin();
  for(; it != rectHash.constEnd(); it++)
    if (it.value().contains(APoint))
      return it.key();

  return RLID_DISPLAY;
}

QRect RosterIndexDelegate::labelRect(int ALabelId, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const
{
  return drawIndex(NULL,AOption,AIndex).value(ALabelId);
}

QHash<int,QRect> RosterIndexDelegate::drawIndex(QPainter *APainter, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const
{
  QHash<int,QRect> rectHash;

  QStyleOptionViewItem option = indexOptions(AIndex,AOption);
  const int hMargin = QApplication::style()->pixelMetric(QStyle::PM_FocusFrameHMargin) >> 1;
  const int vMargin = QApplication::style()->pixelMetric(QStyle::PM_FocusFrameVMargin) >> 1;

  QRect paintRect(option.rect.adjusted(hMargin,vMargin,-hMargin,-vMargin));

  if (APainter)
  {
    APainter->save();
    drawBackground(APainter,option,option.rect,AIndex);
  }

  if (AIndex.parent().isValid() && AIndex.model()->hasChildren(AIndex))
  {
    QStyleOption brachOption(option);
    brachOption.state |= QStyle::State_Children;
    brachOption.rect = QStyle::alignedRect(option.direction,Qt::AlignVCenter|Qt::AlignLeft,QSize(BRANCH_WIDTH,BRANCH_WIDTH),paintRect);
    if (APainter)
      qApp->style()->drawPrimitive(QStyle::PE_IndicatorBranch, &brachOption, APainter);
    removeWidth(paintRect,BRANCH_WIDTH,AOption.direction==Qt::LeftToRight);
    rectHash.insert(RLID_INDICATORBRANCH,brachOption.rect);
  }

  QList<LabelItem> labels = itemLabels(AIndex);
  setLabelsSize(option,labels);
  paintRect = setLabelsRect(option,paintRect,labels);

  QList<LabelItem> lines = itemTextLines(AIndex);
  setTextLinesSize(option,lines);
  paintRect = setTextLinesRect(option,paintRect,lines);

  foreach(LabelItem label, labels)
  {
    if (APainter && ((label.flags&IRostersView::LabelBlink)==0 || FShowBlinkLabels))
      drawLabelItem(APainter,option,label);
    rectHash.insert(label.id,label.rect);
  }

  foreach(LabelItem line, lines)
  {
    if (APainter && ((line.flags&IRostersView::LabelBlink)==0 || FShowBlinkLabels))
      drawLabelItem(APainter,line.id==RLID_FOOTER_TEXT ? indexFooterOptions(option) : option,line);
    rectHash.insert(line.id,line.rect);
  }

  if (APainter)
  {
    drawFocus(APainter,option,option.rect);
    APainter->restore();
  }

  return rectHash;
}

void RosterIndexDelegate::drawLabelItem(QPainter *APainter, const QStyleOptionViewItem &AOption, const LabelItem &ALabel) const
{
  if (ALabel.rect.isEmpty())
    return;

  switch(ALabel.value.type())
  {
  case QVariant::Pixmap:
    {
      QPixmap pixmap = qvariant_cast<QPixmap>(ALabel.value);
      APainter->drawPixmap(ALabel.rect.topLeft(),pixmap);
    }
  case QVariant::Image:
    {
      QImage image = qvariant_cast<QImage>(ALabel.value);
      APainter->drawImage(ALabel.rect.topLeft(),image);
    }
  case QVariant::Icon:
    {
      QIcon icon = qvariant_cast<QIcon>(ALabel.value);
      QPixmap pixmap = icon.pixmap(AOption.decorationSize,getIconMode(AOption.state),getIconState(AOption.state));
      APainter->drawPixmap(ALabel.rect.topLeft(),pixmap);
    }
  case QVariant::String:
    {
      if (APainter)
      {
        QPalette::ColorGroup cg = AOption.state & QStyle::State_Enabled ? QPalette::Normal : QPalette::Disabled;
        if (cg == QPalette::Normal && !(AOption.state & QStyle::State_Active))
          cg = QPalette::Inactive;

        if (AOption.state & QStyle::State_Selected)
        {
          APainter->fillRect(ALabel.rect, AOption.palette.brush(cg, QPalette::Highlight));
          APainter->setPen(AOption.palette.color(cg, QPalette::HighlightedText));
        } 
        else 
        {
          APainter->setPen(AOption.palette.color(cg, QPalette::Text));
        }
        APainter->setFont(AOption.font);
      }
      QString text = ALabel.value.toString();
      int flags = AOption.direction | Qt::TextSingleLine;
      APainter->drawText(ALabel.rect,flags,text);
    }
  }
}

void RosterIndexDelegate::drawBackground(QPainter *APainter, const QStyleOptionViewItem &AOption,  
                                         const QRect &ARect, const QModelIndex &AIndex) const
{
  if (AOption.showDecorationSelected && (AOption.state & QStyle::State_Selected)) {
    QPalette::ColorGroup cg = AOption.state & QStyle::State_Enabled ? QPalette::Normal : QPalette::Disabled;
    if (cg == QPalette::Normal && !(AOption.state & QStyle::State_Active))
      cg = QPalette::Inactive;
    APainter->fillRect(ARect, AOption.palette.brush(cg, QPalette::Highlight));
  } 
  else 
  {
    QVariant value = AIndex.data(Qt::BackgroundRole);
    if (qVariantCanConvert<QBrush>(value)) 
    {
      QPointF oldBO = APainter->brushOrigin();
      APainter->setBrushOrigin(ARect.topLeft());
      APainter->fillRect(ARect, qvariant_cast<QBrush>(value));
      APainter->setBrushOrigin(oldBO);
    }
  }
}

void RosterIndexDelegate::drawFocus(QPainter *APainter, const QStyleOptionViewItem &AOption, const QRect &ARect) const 
{
  if ((AOption.state & QStyle::State_HasFocus) && ARect.isValid())
  {
    QStyleOptionFocusRect focusOption;
    focusOption.QStyleOption::operator=(AOption);
    focusOption.rect = ARect;
    focusOption.state |= QStyle::State_KeyboardFocusChange;
    QPalette::ColorGroup cg = (AOption.state & QStyle::State_Enabled) ? QPalette::Normal : QPalette::Disabled;
    QPalette::ColorRole cr = (AOption.state & QStyle::State_Selected) ? QPalette::Highlight : QPalette::Window;
    focusOption.backgroundColor = AOption.palette.color(cg,cr);
    qApp->style()->drawPrimitive(QStyle::PE_FrameFocusRect, &focusOption, APainter);
  }
}

QStyleOptionViewItem RosterIndexDelegate::indexOptions(const QModelIndex &AIndex, const QStyleOptionViewItem &AOption) const
{
  QStyleOptionViewItem option = AOption;

  QVariant data = AIndex.data(Qt::FontRole);
  if (data.isValid())
  {
    option.font = qvariant_cast<QFont>(data).resolve(option.font);
    option.fontMetrics = QFontMetrics(option.font);
  }

  data = AIndex.data(Qt::ForegroundRole);
  if (qVariantCanConvert<QBrush>(data))
    option.palette.setBrush(QPalette::Text, qvariant_cast<QBrush>(data));

  data = AIndex.data(RDR_FontHint);
  if (data.isValid())
    option.font.setStyleHint((QFont::StyleHint)data.toInt());
  
  data = AIndex.data(RDR_FontSize);
  if (data.isValid())
    option.font.setPointSize(data.toInt());

  data = AIndex.data(RDR_FontWeight);
  if (data.isValid())
    option.font.setWeight(data.toInt());

  data = AIndex.data(RDR_FontStyle);
  if (data.isValid())
    option.font.setStyle((QFont::Style)data.toInt());

  data = AIndex.data(RDR_FontUnderline);
  if (data.isValid())
    option.font.setUnderline(data.toBool());

  return option;
}

QStyleOptionViewItem RosterIndexDelegate::indexFooterOptions(const QStyleOptionViewItem &AOption) const
{
  QStyleOptionViewItem option = AOption;

  option.font.setPointSize(option.font.pointSize()-1);
  option.font.setBold(false);
  option.font.setItalic(true);

  return option;
}

QList<LabelItem> RosterIndexDelegate::itemLabels(const QModelIndex &AIndex) const
{
  QList<LabelItem> labels;

  QList<QVariant> labelIds = AIndex.data(RDR_LabelIds).toList();
  QList<QVariant> labelOrders = AIndex.data(RDR_LabelOrders).toList();
  QList<QVariant> labelFlags = AIndex.data(RDR_LabelFlags).toList();
  QList<QVariant> labelValues = AIndex.data(RDR_LabelValues).toList();
  
  for (int i = 0; i < labelOrders.count(); i++)
  {
    LabelItem label;
    label.id = labelIds.at(i).toInt();
    label.order = labelOrders.at(i).toInt();
    label.flags = labelFlags.at(i).toInt();
    label.value = labelValues.at(i).type()==QVariant::Int ? AIndex.data(labelValues.at(i).toInt()) : labelValues.at(i);
    labels.append(label);
  }

  LabelItem decorLabel;
  decorLabel.id = RLID_DISPLAY;
  decorLabel.order = RLO_DECORATION;
  decorLabel.flags = 0;
  decorLabel.value = AIndex.data(Qt::DecorationRole);
  labels.append(decorLabel);

  return labels;
}

QList<LabelItem> RosterIndexDelegate::itemTextLines(const QModelIndex &AIndex) const
{
  QList<LabelItem> lines;
  QMap<QString,QVariant> footerMap = AIndex.data(RDR_FooterText).toMap();
  QMap<QString,QVariant>::const_iterator fit = footerMap.constBegin();
  while (fit != footerMap.constEnd())
  {
    LabelItem footer;
    footer.id = RLID_FOOTER_TEXT;
    footer.order = fit.key().toInt();
    footer.flags = 0;
    footer.value = fit.value().type()==QVariant::Int ? AIndex.data(fit.value().toInt()) : fit.value();
    lines.append(footer);
    fit++;
  }

  LabelItem display;
  display.id = RLID_DISPLAY;
  display.order = RLO_DISPLAY;
  display.flags = 0;
  display.value = AIndex.data(Qt::DisplayRole);
  lines.append(display);

  return lines;
}

QSize RosterIndexDelegate::variantSize(const QStyleOptionViewItem &AOption, const QVariant &AValue) const
{
  switch(AValue.type())
  {
  case QVariant::Pixmap:
    {
      QPixmap pixmap = qvariant_cast<QPixmap>(AValue);
      if (!pixmap.isNull())
        return pixmap.size();
    }
  case QVariant::Image:
    {
      QImage image = qvariant_cast<QImage>(AValue);
      if (!image.isNull())
        return image.size();
    }
  case QVariant::Icon:
    {
      QIcon icon = qvariant_cast<QIcon>(AValue);
      if (!icon.isNull())
        return AOption.decorationSize;
    }
  case QVariant::String:
    {
      QString text = AValue.toString();
      if (!text.isEmpty())
      {
        QFontMetrics fontMetrics(AOption.font, NULL);
        return fontMetrics.size(AOption.direction | Qt::TextSingleLine,text); 
      }
    }
  }
  return QSize(0,0);
}

QSize RosterIndexDelegate::setLabelsSize(const QStyleOptionViewItem &AOption, QList<LabelItem> &ALabels) const
{
  QSize size(0,0);
  for (QList<LabelItem>::iterator it = ALabels.begin(); it != ALabels.end(); it++)
  {
    it->size = variantSize(AOption,it->value);
    size.rwidth() += it->size.width() + spacing;
    size.rheight() = qMax(size.height(),it->size.height());
  }
  return size;
}

QSize RosterIndexDelegate::setTextLinesSize(const QStyleOptionViewItem &AOption, QList<LabelItem> &ALines) const
{
  QSize size(0,0);
  for (QList<LabelItem>::iterator it = ALines.begin(); it != ALines.end(); it++)
  {
    it->size = variantSize(it->id==RLID_FOOTER_TEXT ? indexFooterOptions(AOption) : AOption, it->value);
    size.rwidth() = qMax(size.width(),it->size.width());
    size.rheight() += it->size.height();
  }
  return size;
}

QRect RosterIndexDelegate::setLabelsRect(const QStyleOptionViewItem &AOption, const QRect &ARect, QList<LabelItem> &ALabels) const
{
  qSort(ALabels);
  QRect paintRect = ARect;
  
  for (int i =0; i<ALabels.count() && ALabels.at(i).order<RLO_RIGHTALIGN; i++)
  {
    LabelItem &label = ALabels[i];
    label.rect = QStyle::alignedRect(AOption.direction,Qt::AlignVCenter|Qt::AlignLeft,label.size,paintRect).intersected(paintRect);
    removeWidth(paintRect,label.rect.width(),AOption.direction==Qt::LeftToRight);
  }
  
  for (int i=ALabels.count()-1; i>=0 && ALabels.at(i).order>=RLO_RIGHTALIGN; i--)
  {
    LabelItem &label = ALabels[i];
    label.rect = QStyle::alignedRect(AOption.direction,Qt::AlignVCenter|Qt::AlignRight,label.size,paintRect).intersected(paintRect);
    removeWidth(paintRect,label.rect.width(),AOption.direction!=Qt::LeftToRight);
  }

  return paintRect;
}

QRect RosterIndexDelegate::setTextLinesRect(const QStyleOptionViewItem &AOption, const QRect &ARect, QList<LabelItem> &ALines) const
{
  qSort(ALines);
  QSize textSize(0,0);
  for (QList<LabelItem>::const_iterator it = ALines.constBegin(); it != ALines.constEnd(); it++)
  {
    textSize.rheight() += it->size.height();
    textSize.rwidth() = qMax(textSize.width(),it->size.width());
  }

  QRect paintRect = QStyle::alignedRect(AOption.direction,Qt::AlignVCenter|Qt::AlignLeft,textSize,ARect).intersected(ARect);
  for (QList<LabelItem>::iterator it = ALines.begin(); it != ALines.end() && !paintRect.isEmpty(); it++)
  {
    it->rect = QStyle::alignedRect(AOption.direction,Qt::AlignTop | Qt::AlignLeft,it->size,paintRect).intersected(paintRect);
    paintRect.setTop(it->rect.bottom());
  }

  return paintRect;
}

void RosterIndexDelegate::removeWidth(QRect &ARect,int AWidth, bool AIsLeftToRight) const
{
  if (AIsLeftToRight)
    ARect.setLeft(ARect.left()+AWidth+spacing);
  else
    ARect.setRight(ARect.right()-AWidth-spacing);
}

QIcon::Mode RosterIndexDelegate::getIconMode(QStyle::State AState) const
{
  if (!(AState & QStyle::State_Enabled)) return QIcon::Disabled;
  if (AState & QStyle::State_Selected) return QIcon::Selected;
  return QIcon::Normal;
}

QIcon::State RosterIndexDelegate::getIconState(QStyle::State AState) const
{
  return AState & QStyle::State_Open ? QIcon::On : QIcon::Off;
}

