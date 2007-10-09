#include <QtDebug>
#include "rosterindexdelegate.h"

#include <QApplication>
#include <QPainter>
#include <QTextOption>
#include <QTextLayout>

#define BRANCH_WIDTH  10

RosterIndexDelegate::RosterIndexDelegate(QObject *AParent)
  : QAbstractItemDelegate(AParent)
{
  FShowBlinkLabels = true;
  FOptions = 0;
}

RosterIndexDelegate::~RosterIndexDelegate()
{

}

void RosterIndexDelegate::paint(QPainter *APainter, 
                                const QStyleOptionViewItem &AOption,  
                                const QModelIndex &AIndex) const
{
  drawIndex(APainter,AOption,AIndex);
}

QSize RosterIndexDelegate::sizeHint(const QStyleOptionViewItem &AOption,  
                                    const QModelIndex &AIndex) const
{
  QStyleOptionViewItem option(AOption);
  option.rect = QRect(0,0,INT_MAX,INT_MAX);
  return drawIndex(NULL,option,AIndex).value(RLID_SIZE_HINT).size();
}

int RosterIndexDelegate::labelAt(const QPoint &APoint, const QStyleOptionViewItem &AOption, 
                                 const QModelIndex &AIndex) const
{
  if (!AOption.rect.contains(APoint))
    return RLID_NULL;

  QHash<int,QRect> rectHash = drawIndex(NULL,AOption,AIndex);
  QHash<int,QRect>::const_iterator it = rectHash.constBegin();
  for(; it != rectHash.constEnd(); it++)
    if (it.key() != RLID_SIZE_HINT && it.value().contains(APoint))
      return it.key();

  return RLID_DISPLAY;
}

QRect RosterIndexDelegate::labelRect(int ALabelId, const QStyleOptionViewItem &AOption, 
                                     const QModelIndex &AIndex) const
{
  return drawIndex(NULL,AOption,AIndex).value(ALabelId);
}

bool RosterIndexDelegate::checkOption(IRostersView::Option AOption) const
{
  return (FOptions & AOption) > 0;
}

void RosterIndexDelegate::setOption(IRostersView::Option AOption, bool AValue)
{
  AValue ? FOptions |= AOption : FOptions &= ~AOption;
}

QHash<int,QRect> RosterIndexDelegate::drawIndex(QPainter *APainter, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const
{
  QHash<int,QRect> rectHash;

  QStyleOptionViewItem option = setOptions(AIndex,AOption);

  const bool isLeftToRight = option.direction == Qt::LeftToRight;
  const int hMargin = QApplication::style()->pixelMetric(QStyle::PM_FocusFrameHMargin) >> 1;
  const int vMargin = QApplication::style()->pixelMetric(QStyle::PM_FocusFrameHMargin) >> 1;

  QRect paintRect(option.rect.adjusted(hMargin,vMargin,-hMargin,-vMargin));
  QPoint startPoint = isLeftToRight ? paintRect.topLeft() : paintRect.topRight();
  QRect sizeHintRect(startPoint,startPoint);
  QRect labelsRect = paintRect;
  QRect footerRect = paintRect;
  int footerRemovedWidth = 0;
  bool showFooterText = checkOption(IRostersView::ShowFooterText) && AIndex.data(RDR_FooterText).isValid();

  if (APainter)
  {
    APainter->save();
    drawBackground(APainter,option,option.rect,AIndex);
  }

  if (AIndex.parent().isValid() && AIndex.model()->hasChildren(AIndex))
  {
    QStyleOption brachOption(option);
    brachOption.state |= QStyle::State_Children;
    brachOption.rect = QStyle::alignedRect(option.direction,Qt::AlignVCenter,QSize(BRANCH_WIDTH,BRANCH_WIDTH),labelsRect);
    if (APainter)
      qApp->style()->drawPrimitive(QStyle::PE_IndicatorBranch, &brachOption, APainter);
    rectHash.insert(RLID_INDICATORBRANCH,brachOption.rect);
    addSize(sizeHintRect,brachOption.rect,isLeftToRight);
    removeWidth(labelsRect,brachOption.rect.width(),isLeftToRight);
    removeWidth(footerRect,brachOption.rect.width(),isLeftToRight);
  }

  LabelsMap map = labelsMap(AIndex);
  for(LabelsMap::const_iterator it = map.constBegin(); it != map.constEnd(); it++)
  {
    option.decorationAlignment = labelAlignment(it.key());
    option.displayAlignment = option.decorationAlignment;
    bool paintLabel = FShowBlinkLabels || !FBlinkLabels.contains(it.value().first);
    QRect rect = drawVariant(paintLabel ? APainter : NULL,option,labelsRect,it.value().second);

    if (!rect.isEmpty())
    {
      if (showFooterText)
      {
        if (it.key() < RLO_DECORATION || it.key() >= RLO_FULL_HEIGHT_LABELS)
        {
          footerRemovedWidth += rect.width()+spacing;
          removeWidth(footerRect,rect.width(),isLeftToRight^(it.key()>=RLO_RIGHTALIGN));
        }
        if (it.key() >= RLO_DECORATION && it.key() <= RLO_FULL_HEIGHT_LABELS)
          footerRect.setTop(qMax(footerRect.top(),rect.bottom()));
      }
      addSize(sizeHintRect,rect,isLeftToRight);
      removeWidth(labelsRect,rect.width(),isLeftToRight^(it.key()>=RLO_RIGHTALIGN));
    }
    rectHash.insert(it.value().first,rect);
  }
  
  if (showFooterText)
  {
    rectHash.insert(RLID_FOOTER_TEXT,footerRect);
    int footerMaxWidth = sizeHintRect.width()-footerRemovedWidth;
    option = setFooterOptions(AIndex,option);
    option.displayAlignment = labelAlignment(RLO_DISPLAY);
    QMap<QString,QVariant> footerMap = AIndex.data(RDR_FooterText).toMap();
    QMap<QString,QVariant>::const_iterator fit = footerMap.constBegin();
    while (fit != footerMap.constEnd())
    {
      QRect rect = drawVariant(APainter,option,footerRect,fit.value());
      if (!rect.isEmpty())
      {
        if (footerMaxWidth < rect.width())
          addSize(sizeHintRect,QRect(0,0,rect.width()-footerMaxWidth,0),isLeftToRight);
        sizeHintRect.setBottom(qMax(sizeHintRect.bottom(), rect.bottom()));
        footerRect.setTop(rect.bottom());
      }
      fit++;
    }
  }
  
  sizeHintRect.adjust(-hMargin,-vMargin,hMargin,vMargin);
  rectHash.insert(RLID_SIZE_HINT,sizeHintRect);

  if (APainter)
  {
    drawFocus(APainter,option,option.rect);
    APainter->restore();
  }

  return rectHash;
}

QRect RosterIndexDelegate::drawVariant(QPainter *APainter, const QStyleOptionViewItem &AOption, 
                                       const QRect &ARect, const QVariant &AValue) const
{
  if (!ARect.isValid() || ARect.isEmpty() || !AValue.isValid())
    return QRect();

  switch(AValue.type())
  {
  case QVariant::Pixmap:
    {
      QPixmap pixmap = qvariant_cast<QPixmap>(AValue);
      QRect rect = QStyle::alignedRect(AOption.direction,AOption.decorationAlignment,pixmap.size(),ARect);
      rect = rect.intersected(ARect); 
      if (APainter)
        APainter->drawPixmap(rect.topLeft(),pixmap);
      return rect;
    }
  case QVariant::Image:
    {
      QImage image = qvariant_cast<QImage>(AValue);
      QRect rect = QStyle::alignedRect(AOption.direction,AOption.decorationAlignment,image.size(),ARect);
      rect = rect.intersected(ARect); 
      if (APainter)
        APainter->drawImage(rect.topLeft(),image);
      return rect;
    }
  case QVariant::Icon:
    {
      QIcon icon = qvariant_cast<QIcon>(AValue);
      QPixmap pixmap = icon.pixmap(AOption.decorationSize,getIconMode(AOption.state),getIconState(AOption.state));
      QRect rect = QStyle::alignedRect(AOption.direction,AOption.decorationAlignment,pixmap.size(),ARect);
      rect = rect.intersected(ARect);
      if (APainter)
        APainter->drawPixmap(rect.topLeft(),pixmap);
      return rect;
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
          APainter->fillRect(ARect, AOption.palette.brush(cg, QPalette::Highlight));
          APainter->setPen(AOption.palette.color(cg, QPalette::HighlightedText));
        } 
        else 
        {
          APainter->setPen(AOption.palette.color(cg, QPalette::Text));
        }
        APainter->setFont(AOption.font);
      }
  
      QString text = AValue.toString();
      QFontMetrics fontMetrics(AOption.font, APainter!=NULL ? APainter->device(): NULL);
      int flags = AOption.direction | Qt::TextSingleLine;
      QSize textSize = fontMetrics.size(flags,text); 
      QRect rect = QStyle::alignedRect(AOption.direction,AOption.displayAlignment,textSize,ARect).intersect(ARect);
      if (APainter)
        APainter->drawText(rect,flags,text);
      return rect;
    }
  default:
    return QRect();
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

void RosterIndexDelegate::drawFocus(QPainter *APainter, const QStyleOptionViewItem &AOption,  
                                     const QRect &ARect) const 
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
    QApplication::style()->drawPrimitive(QStyle::PE_FrameFocusRect, &focusOption, APainter);
  }
}

LabelsMap RosterIndexDelegate::labelsMap(const QModelIndex &AIndex) const
{
  typedef QPair<int,QVariant> pair;
  LabelsMap map;

  map.insert(RLO_DECORATION,pair(RLID_DISPLAY,AIndex.data(Qt::DecorationRole)));
  map.insert(RLO_DISPLAY,pair(RLID_DISPLAY,AIndex.data(Qt::DisplayRole)));

  QList<QVariant> labelIds = AIndex.data(RDR_LabelIds).toList();
  QList<QVariant> labelOrders = AIndex.data(RDR_LabelOrders).toList();
  QList<QVariant> labelValues = AIndex.data(RDR_LabelValues).toList();
  for (int ilabel = 0; ilabel < labelOrders.count(); ilabel++)
    map.insert(labelOrders.at(ilabel).toInt(),pair(labelIds.at(ilabel).toInt(),labelValues.at(ilabel)));

  return map;
}

QStyleOptionViewItem RosterIndexDelegate::setOptions(const QModelIndex &AIndex,
                                                     const QStyleOptionViewItem &AOption) const
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

QStyleOptionViewItem RosterIndexDelegate::setFooterOptions(const QModelIndex &/*AIndex*/, 
                                                           const QStyleOptionViewItem &AOption) const
{
  QStyleOptionViewItem option = AOption;

  option.font.setPointSize(option.font.pointSize()-1);
  option.font.setBold(false);
  option.font.setItalic(true);

  return option;
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

void RosterIndexDelegate::addSize(QRect &ARect,const QRect &AAddRect, bool AIsLeftToRight) const
{
  if (AIsLeftToRight)
    ARect.setRight(ARect.right()+AAddRect.width()+spacing);
  else
    ARect.setLeft(ARect.left()-AAddRect.width()-spacing);

  ARect.setHeight(qMax(ARect.height(), AAddRect.height()));
}

void RosterIndexDelegate::removeWidth(QRect &ARect,int AWidth, bool AIsLeftToRight) const
{
  if (AIsLeftToRight)
    ARect.setLeft(ARect.left()+AWidth+spacing);
  else
    ARect.setRight(ARect.right()-AWidth-spacing);
}

Qt::Alignment RosterIndexDelegate::labelAlignment(int ALabelOrder) const
{
  Qt::Alignment align = Qt::AlignTop;
  if (ALabelOrder < RLO_RIGHTALIGN)
    align |= Qt::AlignLeft;
  else
    align |= Qt::AlignRight;
  return align;
}

