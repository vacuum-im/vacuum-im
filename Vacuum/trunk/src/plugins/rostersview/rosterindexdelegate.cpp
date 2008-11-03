#include "rosterindexdelegate.h"

#include <QApplication>
#include <QPainter>

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
  QStyleOptionViewItem option = indexOptions(AIndex,AOption);
  const int hMargin = QApplication::style()->pixelMetric(QStyle::PM_FocusFrameHMargin);
  const int vMargin = QApplication::style()->pixelMetric(QStyle::PM_FocusFrameVMargin);

  QSize leftCenter(0,0);
  QSize middleTop(0,0);
  QSize middleBottom(0,0);
  QSize rightCenter(0,0);

  if (AIndex.parent().isValid() && AIndex.model()->hasChildren(AIndex))
  {
    leftCenter.rwidth() += BRANCH_WIDTH;
    leftCenter.rheight() += BRANCH_WIDTH;
  }

  QList<LabelItem> labels = itemLabels(AIndex);
  getLabelsSize(option,labels);
  for (QList<LabelItem>::const_iterator it = labels.constBegin(); it!=labels.constEnd(); it++)
  {
    if (it->order >= RLAP_LEFT_CENTER && it->order < RLAP_LEFT_TOP)
    {
      leftCenter.rwidth() += it->size.width() + spacing;
      leftCenter.rheight() = qMax(leftCenter.height(),it->size.height());
    }
    else if (it->order >= RLAP_LEFT_TOP && it->order < RLAP_RIGHT_CENTER)
    {
      middleTop.rwidth() += it->size.width() + spacing;
      middleTop.rheight() = qMax(leftCenter.height(),it->size.height());
    }
    else if (it->order >= RLAP_RIGHT_CENTER)
    {
      rightCenter.rwidth() += it->size.width() + spacing;
      rightCenter.rheight() = qMax(leftCenter.height(),it->size.height());
    }
  }

  QList<LabelItem> footers = itemFooters(AIndex);
  getLabelsSize(option,footers);
  for (QList<LabelItem>::const_iterator it = footers.constBegin(); it!=footers.constEnd(); it++)
  {
    middleBottom.rwidth() = qMax(middleBottom.width(),it->size.width());
    middleBottom.rheight() += it->size.height();
  }

  QSize hint(0,0);
  hint.rwidth() += qMax(middleTop.width(),middleBottom.width());
  hint.rheight() = qMax(hint.height(),middleTop.height()+middleBottom.height());
  hint.rwidth() += leftCenter.width();
  hint.rheight() = qMax(hint.height(),leftCenter.height());
  hint.rwidth() += rightCenter.width();
  hint.rheight() = qMax(hint.height(),rightCenter.height());
  hint += QSize(hMargin,vMargin);

  return hint;
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
    APainter->setClipping(true);
    APainter->setClipRect(option.rect);
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
  QList<LabelItem> footers = itemFooters(AIndex);
  getLabelsSize(option,labels);
  getLabelsSize(option,footers);
  qSort(labels);
  qSort(footers);
  
  int leftIndex =0;
  for (; leftIndex<labels.count() && labels.at(leftIndex).order<RLAP_LEFT_TOP; leftIndex++)
  {
    LabelItem &label = labels[leftIndex];
    Qt::Alignment align = Qt::AlignLeft | Qt::AlignVCenter;
    label.rect = QStyle::alignedRect(option.direction,align,label.size,paintRect).intersected(paintRect);
    removeWidth(paintRect,label.rect.width(),AOption.direction==Qt::LeftToRight);
    if (APainter)
      drawLabelItem(APainter,option,label);
    rectHash.insert(label.id,label.rect);
  }

  int rightIndex=labels.count()-1;
  for (; rightIndex>=0 && labels.at(rightIndex).order>=RLAP_RIGHT_CENTER; rightIndex--)
  {
    LabelItem &label = labels[rightIndex];
    Qt::Alignment align = Qt::AlignRight | Qt::AlignVCenter;
    label.rect = QStyle::alignedRect(option.direction,align,label.size,paintRect).intersected(paintRect);
    removeWidth(paintRect,label.rect.width(),AOption.direction!=Qt::LeftToRight);
    if (APainter)
      drawLabelItem(APainter,option,label);
    rectHash.insert(label.id,label.rect);
  }

  int topLabelsWidth = 0;
  QSize middleTop(paintRect.width(),0);
  for (int i=leftIndex; i<=rightIndex; i++)
  {
    const LabelItem &label = labels.at(i);
    middleTop.rheight() = qMax(middleTop.height(),label.size.height());
    if (label.id !=RLID_DISPLAY)
      topLabelsWidth += label.size.width()+spacing;
  }

  QSize middleBottom(paintRect.width(),0);
  for (int i=0; i<footers.count(); i++)
  {
    const LabelItem &label = footers.at(i);
    middleBottom.rheight() += label.size.height();
  }

  QSize middle(paintRect.width(),middleTop.height()+middleBottom.height());
  paintRect = QStyle::alignedRect(option.direction,Qt::AlignLeft|Qt::AlignVCenter,middle,paintRect).intersected(paintRect);
  QRect topRect = QStyle::alignedRect(option.direction,Qt::AlignLeft|Qt::AlignTop,middleTop,paintRect).intersected(paintRect);
  QRect bottomRect = QStyle::alignedRect(option.direction,Qt::AlignLeft|Qt::AlignBottom,middleBottom,paintRect).intersected(paintRect);

  for (; leftIndex<=rightIndex && labels.at(leftIndex).order<RLAP_RIGHT_TOP; leftIndex++)
  {
    LabelItem &label = labels[leftIndex];
    if (label.id == RLID_DISPLAY)
      label.size.rwidth() = qMin(label.size.width(),middleTop.width()-topLabelsWidth);
    Qt::Alignment align = Qt::AlignVCenter | Qt::AlignLeft;
    label.rect = QStyle::alignedRect(option.direction,align,label.size,topRect).intersected(topRect);
    removeWidth(topRect,label.rect.width(),option.direction==Qt::LeftToRight);
    if (APainter)
      drawLabelItem(APainter,option,label);
    rectHash.insert(label.id,label.rect);
  }

  for (; leftIndex<=rightIndex && labels.at(rightIndex).order>=RLAP_RIGHT_TOP;rightIndex--)
  {
    LabelItem &label = labels[rightIndex];
    if (label.id == RLID_DISPLAY)
      label.size.rwidth() = qMin(label.size.width(),middleTop.width()-topLabelsWidth);
    Qt::Alignment align = Qt::AlignVCenter | Qt::AlignRight;
    label.rect = QStyle::alignedRect(option.direction,align,label.size,topRect).intersected(topRect);
    removeWidth(topRect,label.rect.width(),option.direction!=Qt::LeftToRight);
    if (APainter)
      drawLabelItem(APainter,option,label);
    rectHash.insert(label.id,label.rect);
  }

  for (int i=0; i<footers.count(); i++)
  {
    LabelItem &label = footers[i];
    label.rect = QStyle::alignedRect(option.direction,Qt::AlignTop|Qt::AlignLeft,label.size,bottomRect).intersected(bottomRect);
    bottomRect.setTop(label.rect.bottom());
    if (APainter)
      drawLabelItem(APainter,indexFooterOptions(option),label);
    rectHash.insert(label.id,label.rect);
  }

  if (APainter)
  {
    APainter->setClipRect(option.rect);
    drawFocus(APainter,option,option.rect);
    APainter->restore();
  }

  return rectHash;
}

void RosterIndexDelegate::drawLabelItem(QPainter *APainter, const QStyleOptionViewItem &AOption, const LabelItem &ALabel) const
{
  if (ALabel.rect.isEmpty() || ALabel.value.isNull() || ((ALabel.flags&IRostersView::LabelBlink)>0 && !FShowBlinkLabels))
    return;

  APainter->setClipRect(ALabel.rect);

  switch(ALabel.value.type())
  {
  case QVariant::Pixmap:
    {
      QPixmap pixmap = qvariant_cast<QPixmap>(ALabel.value);
      APainter->drawPixmap(ALabel.rect.topLeft(),pixmap);
      break;
    }
  case QVariant::Image:
    {
      QImage image = qvariant_cast<QImage>(ALabel.value);
      APainter->drawImage(ALabel.rect.topLeft(),image);
      break;
    }
  case QVariant::Icon:
    {
      QIcon icon = qvariant_cast<QIcon>(ALabel.value);
      QPixmap pixmap = icon.pixmap(AOption.decorationSize,getIconMode(AOption.state),getIconState(AOption.state));
      APainter->drawPixmap(ALabel.rect.topLeft(),pixmap);
      break;
    }
  case QVariant::String:
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
      int flags = AOption.direction | Qt::TextSingleLine;
      QFontMetrics fontMetrics(AOption.font,APainter->device());
      QString text = fontMetrics.elidedText(ALabel.value.toString(),Qt::ElideRight,ALabel.rect.width(),flags);
      APainter->drawText(ALabel.rect,flags,text);
      break;
    }
  default:
    break;   
  }
}

void RosterIndexDelegate::drawBackground(QPainter *APainter, const QStyleOptionViewItem &AOption,  
                                         const QRect &ARect, const QModelIndex &AIndex) const
{
  if (AOption.showDecorationSelected && (AOption.state & QStyle::State_Selected)) 
  {
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

  LabelItem decoration;
  decoration.id = RLID_DECORATION;
  decoration.order = RLO_DECORATION;
  decoration.flags = 0;
  decoration.value = AIndex.data(Qt::DecorationRole);
  labels.append(decoration);

  LabelItem display;
  display.id = RLID_DISPLAY;
  display.order = RLO_DISPLAY;
  display.flags = 0;
  display.value = AIndex.data(Qt::DisplayRole);
  labels.append(display);

  return labels;
}

QList<LabelItem> RosterIndexDelegate::itemFooters(const QModelIndex &AIndex) const
{
  QList<LabelItem> footers;
  QMap<QString,QVariant> footerMap = AIndex.data(RDR_FooterText).toMap();
  QMap<QString,QVariant>::const_iterator fit = footerMap.constBegin();
  while (fit != footerMap.constEnd())
  {
    LabelItem footer;
    footer.id = RLID_FOOTER_TEXT;
    footer.order = fit.key().toInt();
    footer.flags = 0;
    footer.value = fit.value().type()==QVariant::Int ? AIndex.data(fit.value().toInt()) : fit.value();
    footers.append(footer);
    fit++;
  }
  return footers;
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
      break;
    }
  case QVariant::Image:
    {
      QImage image = qvariant_cast<QImage>(AValue);
      if (!image.isNull())
        return image.size();
      break;
    }
  case QVariant::Icon:
    {
      QIcon icon = qvariant_cast<QIcon>(AValue);
      if (!icon.isNull())
        return AOption.decorationSize;
      break;
    }
  case QVariant::String:
    {
      QString text = AValue.toString();
      if (!text.isEmpty())
      {
        QFontMetrics fontMetrics(AOption.font);
        return fontMetrics.size(AOption.direction | Qt::TextSingleLine,text); 
      }
      break;
    }
  default:
    break;
  }
  return QSize(0,0);
}

void RosterIndexDelegate::getLabelsSize(const QStyleOptionViewItem &AOption, QList<LabelItem> &ALabels) const
{
  for (QList<LabelItem>::iterator it = ALabels.begin(); it != ALabels.end(); it++)
  {
    it->size = variantSize(it->id==RLID_FOOTER_TEXT ? indexFooterOptions(AOption) : AOption, it->value);
  }
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

