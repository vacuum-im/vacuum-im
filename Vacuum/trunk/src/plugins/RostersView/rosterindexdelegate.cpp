#include "rosterindexdelegate.h"

#include <QApplication>
#include <QPainter>
#include <QTextOption>
#include <QTextLayout>
#include "../../definations/rosterlabelorders.h"
#include "../../utils/skin.h"

RosterIndexDelegate::RosterIndexDelegate(QObject *AParent)
  : QAbstractItemDelegate(AParent)
{

}

RosterIndexDelegate::~RosterIndexDelegate()
{

}

void RosterIndexDelegate::paint(QPainter *APainter, 
                                const QStyleOptionViewItem &AOption,  
                                const QModelIndex &AIndex) const
{
  QStyleOptionViewItem option = setOptions(AIndex,AOption);

  APainter->save();

  const int spacing = 2;
  const int halfTextMargin = QApplication::style()->pixelMetric(QStyle::PM_FocusFrameHMargin) >> 1;
  
  QRect freeRect = option.rect.adjusted(halfTextMargin,halfTextMargin,-halfTextMargin,-halfTextMargin);
  QRect usedRect;

  drawBackground(APainter,option,AIndex);

  option.decorationAlignment = Qt::AlignLeft | Qt::AlignTop;
  option.displayAlignment = Qt::AlignLeft | Qt::AlignTop;
  
  QList<QVariant> labelOrders = AIndex.data(IRosterIndex::DR_LabelOrders).toList();
  QList<QVariant> labelValues = AIndex.data(IRosterIndex::DR_LabelValues).toList();

  APainter->setClipRect(freeRect);
  usedRect = drawVariant(APainter,option,freeRect,AIndex.data(Qt::DecorationRole)); 
  if (!usedRect.isNull())
    freeRect.setLeft(usedRect.right()+spacing);

  int ilabel = 0;
  while (ilabel < labelOrders.count() && labelOrders.at(ilabel).toInt() < BEFOUR_DISPLAY_ORDERS)
  {
    APainter->setClipRect(freeRect);
    usedRect = drawVariant(APainter,option,freeRect,labelValues.at(ilabel));
    if (!usedRect.isNull())
      freeRect.setLeft(usedRect.right()+spacing);
    ilabel++;
  }

  APainter->setClipRect(freeRect);
  usedRect = drawVariant(APainter,option,freeRect,AIndex.data(Qt::DisplayRole));
  if (!usedRect.isNull())
    freeRect.setLeft(usedRect.right()+spacing);

  while (ilabel < labelOrders.count() && labelOrders.at(ilabel).toInt() < RIGHTALIGN_ORDERS)
  {
    APainter->setClipRect(freeRect);
    usedRect = drawVariant(APainter,option,freeRect,labelValues.at(ilabel));
    if (!usedRect.isNull())
      freeRect.setLeft(usedRect.right()+spacing);
    ilabel++;
  }

  option.decorationAlignment = Qt::AlignRight | Qt::AlignTop;
  option.displayAlignment = Qt::AlignRight | Qt::AlignTop;
  while (ilabel < labelOrders.count() && labelOrders.at(ilabel).toInt() > RIGHTALIGN_ORDERS)
  {
    APainter->setClipRect(freeRect);
    usedRect = drawVariant(APainter,option,freeRect,labelValues.at(ilabel));
    if (!usedRect.isNull())
      freeRect.setRight(usedRect.left()-spacing);
    ilabel++;
  }

  APainter->setClipRect(option.rect);
  drawFocus(APainter,option,option.rect);

  APainter->restore();
}

QSize RosterIndexDelegate::sizeHint(const QStyleOptionViewItem &AOption,  
                                    const QModelIndex &AIndex) const
{
  QVariant value = AIndex.data(Qt::SizeHintRole);
  if (value.isValid())
    return value.toSize();

  const int spacing = 2;
  const int textMargin = QApplication::style()->pixelMetric(QStyle::PM_FocusFrameHMargin);
  
  int width = 0;
  int height = 0;
  
  QSize size = variantSize(AOption,AIndex.data(Qt::DecorationRole));
  width += size.width() + spacing;
  height = qMax(height,size.height());

  size = variantSize(AOption,AIndex.data(Qt::DisplayRole));
  width += size.width() + spacing;
  height = qMax(height,size.height());

  QList<QVariant> labels = AIndex.data(IRosterIndex::DR_LabelValues).toList();
  foreach(QVariant label, labels)
  {
    size = variantSize(AOption,label);
    width += size.width() + spacing;
    height = qMax(height,size.height());
  }

  return QSize(width+textMargin,height+textMargin); 
}

int RosterIndexDelegate::labelAt(const QStyleOptionViewItem &/*AOption*/, const QPoint &/*APoint*/, 
                                 const QModelIndex &/*AIndex*/) const
{
  return 0;
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
      QRect rect = QStyle::alignedRect(Qt::LeftToRight,AOption.decorationAlignment,pixmap.size(),ARect);
      rect = rect.intersected(ARect); 
      APainter->drawPixmap(rect.topLeft(),pixmap);
      return rect;
    }
  case QVariant::Image:
    {
      QImage image = qvariant_cast<QImage>(AValue);
      QRect rect = QStyle::alignedRect(Qt::LeftToRight,AOption.decorationAlignment,image.size(),ARect);
      rect = rect.intersected(ARect); 
      APainter->drawImage(rect.topLeft(),image);
      return rect;
    }
  case QVariant::Icon:
    {
      QIcon icon = qvariant_cast<QIcon>(AValue);
      QPixmap pixmap = icon.pixmap(AOption.decorationSize,getIconMode(AOption.state),getIconState(AOption.state));
      QRect rect = QStyle::alignedRect(Qt::LeftToRight,AOption.decorationAlignment,pixmap.size(),ARect);
      rect = rect.intersected(ARect);
      APainter->drawPixmap(rect.topLeft(),pixmap);
      return rect;
    }
  case QVariant::String:
    {
      QString text = AValue.toString();

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

      QTextOption textOption;
      textOption.setWrapMode(QTextOption::ManualWrap);
      textOption.setTextDirection(AOption.direction);
      QTextLayout textLayout;
      textLayout.setTextOption(textOption);
      textLayout.setFont(AOption.font);
      textLayout.setText(text.replace(QLatin1Char('\n'),QChar::LineSeparator));
      
      QSize textSize = doTextLayout(textLayout);
      QRect rect = QStyle::alignedRect(AOption.direction,AOption.displayAlignment,textSize,ARect);
      rect = rect.intersected(ARect); 
      textLayout.draw(APainter, rect.topLeft());
      return rect;
    }
  default:
    return QRect();
  }
}

void RosterIndexDelegate::drawBackground(QPainter *APainter, const QStyleOptionViewItem &AOption,  
                                         const QModelIndex &AIndex) const
{
  if (AOption.showDecorationSelected && (AOption.state & QStyle::State_Selected)) {
    QPalette::ColorGroup cg = AOption.state & QStyle::State_Enabled ? QPalette::Normal : QPalette::Disabled;
    if (cg == QPalette::Normal && !(AOption.state & QStyle::State_Active))
      cg = QPalette::Inactive;
    APainter->fillRect(AOption.rect, AOption.palette.brush(cg, QPalette::Highlight));
  } 
  else 
  {
    QVariant value = AIndex.data(Qt::BackgroundRole);
    if (qVariantCanConvert<QBrush>(value)) 
    {
      QPointF oldBO = APainter->brushOrigin();
      APainter->setBrushOrigin(AOption.rect.topLeft());
      APainter->fillRect(AOption.rect, qvariant_cast<QBrush>(value));
      APainter->setBrushOrigin(oldBO);
    }
  }
}

void RosterIndexDelegate::drawFocus(QPainter *APainter, const QStyleOptionViewItem &AOption,  
                                     const QRect &ARect) const 
{
  if ((AOption.state & QStyle::State_HasFocus) == 0 || !ARect.isValid())
    return;

  QStyleOptionFocusRect focusOption;
  focusOption.QStyleOption::operator=(AOption);
  focusOption.rect = ARect;
  focusOption.state |= QStyle::State_KeyboardFocusChange;
  QPalette::ColorGroup cg = (AOption.state & QStyle::State_Enabled) ? QPalette::Normal : QPalette::Disabled;
  QPalette::ColorRole cr = (AOption.state & QStyle::State_Selected) ? QPalette::Highlight : QPalette::Window;
  focusOption.backgroundColor = AOption.palette.color(cg,cr);
  QApplication::style()->drawPrimitive(QStyle::PE_FrameFocusRect, &focusOption, APainter);
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

  data = AIndex.data(IRosterIndex::DR_FontHint);
  if (data.isValid())
    option.font.setStyleHint((QFont::StyleHint)data.toInt());
  
  data = AIndex.data(IRosterIndex::DR_FontSize);
  if (data.isValid())
    option.font.setPointSize(data.toInt());

  data = AIndex.data(IRosterIndex::DR_FontWeight);
  if (data.isValid())
    option.font.setWeight(data.toInt());

  data = AIndex.data(IRosterIndex::DR_FontStyle);
  if (data.isValid())
    option.font.setStyle((QFont::Style)data.toInt());

  data = AIndex.data(IRosterIndex::DR_FontUnderline);
  if (data.isValid())
    option.font.setUnderline(data.toBool());

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

QSize RosterIndexDelegate::variantSize(const QStyleOptionViewItem &AOption,
                                       const QVariant &AValue) const
{
  switch (AValue.type()) 
  {
    case QVariant::Pixmap:
      return qvariant_cast<QPixmap>(AValue).size();
    case QVariant::Image:
      return qvariant_cast<QImage>(AValue).size();
    case QVariant::Icon: 
      {
        QIcon icon = qvariant_cast<QIcon>(AValue);
        return icon.actualSize( AOption.decorationSize, 
                                getIconMode(AOption.state), 
                                getIconState(AOption.state));
      }
    case QVariant::Color:
      return AOption.decorationSize;
    case QVariant::String:
    {
      QString text = AValue.toString().replace(QLatin1Char('\n'),QChar::LineSeparator);
      QTextOption textOption;
      textOption.setAlignment(AOption.displayAlignment);
      textOption.setTextDirection(AOption.direction);
      QTextLayout textLayout;
      textLayout.setTextOption(textOption);
      textLayout.setFont(AOption.font);
      textLayout.setText(text);
      return doTextLayout(textLayout);
    }
    default:
      return QSize();
  }
}

QSize RosterIndexDelegate::doTextLayout(QTextLayout &ATextLayout, int ALineWidth) const
{
  qreal width = 0;
  qreal height = 0;
  QTextLine line;
  ATextLayout.beginLayout();
  while ((line = ATextLayout.createLine()).isValid())
  {
    line.setLineWidth(ALineWidth);
    line.setPosition(QPointF(0,height));
    height+=line.height();
    width = qMax(width,line.naturalTextWidth());
  }
  ATextLayout.endLayout();
  return QSizeF(width,height).toSize();
}


