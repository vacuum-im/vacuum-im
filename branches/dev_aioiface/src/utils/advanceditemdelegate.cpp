#include "advanceditemdelegate.h"

#include <QStyle>
#include <QPainter>
#include <QKeyEvent>
#include <QMultiMap>
#include <QDateTime>
#include <QLayoutItem>
#include <QMouseEvent>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QApplication>
#include <QItemEditorFactory>
#include <QWindowsVistaStyle>

const qreal BlinkHideSteps[] = { 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
const qreal BlinkFadeSteps[] = { 1.0, 0.8, 0.6, 0.4, 0.2, 0.2, 0.4, 0.6, 0.8, 1.0 };
const int BlinkStepsCount = sizeof(BlinkHideSteps)/sizeof(BlinkHideSteps[0]);
const int BlinkStepsTime = 1000;
#define BLINK_STEP ((QDateTime::currentMSecsSinceEpoch() % BlinkStepsTime) * BlinkStepsCount / BlinkStepsTime)

void destroyLayoutRecursive(QLayout *ALayout)
{
	QLayoutItem *child;
	while ((child = ALayout->takeAt(0)) != NULL)
	{
		if (child->layout())
			destroyLayoutRecursive(child->layout());
		else
			delete child;
	}
	delete ALayout;
}

QString getSingleLineText(const QString &AText)
{
	QString text = AText;
	text.replace('\n',' ');
	return text.trimmed();
}

inline void setItemDefaults(int AItemId, AdvancedDelegateItem *AItem)
{
	int index = AdvancedDelegateItem::NullId<=AItemId && AItemId<=AdvancedDelegateItem::DisplayStretchId ? AItemId : AdvancedDelegateItem::NullId;
	AItem->d->position = AdvancedDelegateItemDefaults[index].position;
	AItem->d->floor = AdvancedDelegateItemDefaults[index].floor;
	AItem->d->order = AdvancedDelegateItemDefaults[index].order;
}

/*********************
	AdvancedDelegateItem
**********************/
AdvancedDelegateItem::AdvancedDelegateItem(int ADefaultsId)
{
	d = new AdvancedDelegateItemData;
	d->refs = 1;
	d->kind = Null;
	d->flags = 0;
	d->showStates = 0;
	d->hideStates = 0;
	d->widget = NULL;
	setItemDefaults(ADefaultsId, this);
}

AdvancedDelegateItem::AdvancedDelegateItem(const AdvancedDelegateItem &AOther)
{
	d = AOther.d;
	d->refs++;
}

AdvancedDelegateItem::~AdvancedDelegateItem()
{
	if (--d->refs == 0)
		delete d;
}

AdvancedDelegateItem &AdvancedDelegateItem::operator=(const AdvancedDelegateItem &AOther)
{
	if (this!=&AOther && d!=AOther.d)
	{
		if (--d->refs == 0)
			delete d;
		d = AOther.d;
		d->refs++;
	}
	return *this;
}

/***************************
	AdvancedDelegateLayoutItem
****************************/
class AdvancedDelegateLayoutItem :
	public QLayoutItem
{
public:
	AdvancedDelegateLayoutItem(const AdvancedDelegateItem &AItem, const QStyleOptionViewItemV4 &AOption) : 
			QLayoutItem(Qt::AlignCenter), FItem(AItem), FOption(AOption) 
	{

	}
	bool isEmpty() const 
	{
		return false;
	}
	void invalidate()
	{
		FSizeHint = QSize();
	}
	Qt::Orientations expandingDirections() const
	{
		return FItem.d->sizePolicy.expandingDirections();
	}
	QSize sizeHint() const
	{
		if (!FSizeHint.isValid())
		{
			FSizeHint = FItem.d->hints.value(AdvancedDelegateItem::SizeHint).toSize();
			if (!FSizeHint.isValid())
				FSizeHint = AdvancedItemDelegate::itemSizeHint(FItem,FOption);
		}
		return FSizeHint;
	}
	QSize minimumSize() const
	{
		QSize hint = sizeHint();
		const QSizePolicy &sizeP = FItem.d->sizePolicy;
		return QSize(sizeP.horizontalPolicy() & QSizePolicy::ShrinkFlag ? 0 : hint.width(), sizeP.verticalPolicy() & QSizePolicy::ShrinkFlag ? 0 : hint.height());
	}
	QSize maximumSize() const
	{
		QSize hint = sizeHint();
		const QSizePolicy &sizeP = FItem.d->sizePolicy;
		return QSize(sizeP.horizontalPolicy() & QSizePolicy::GrowFlag ? QLAYOUTSIZE_MAX : hint.width(), sizeP.verticalPolicy() & QSizePolicy::GrowFlag ? QLAYOUTSIZE_MAX : hint.height());
	}
	QRect geometry() const
	{
		return FOption.rect;
	}
	void setGeometry(const QRect &ARect)
	{
		FOption.rect = ARect;
	}
	const AdvancedDelegateItem &delegateItem() const
	{
		return FItem;
	}
	const QStyleOptionViewItemV4 &itemOption() const
	{
		return FOption;
	}
	void drawVariant(QPainter *APainter, const QVariant &AValue)
	{
		QStyle *style = FOption.widget ? FOption.widget->style() : QApplication::style();
		
		switch (AValue.type())
		{
		case QVariant::Color:
			{
				QColor color = qvariant_cast<QColor>(AValue);
				APainter->fillRect(FOption.rect,color);
				break;
			}
		case QVariant::Pixmap:
			{
				QPixmap pixmap = qvariant_cast<QPixmap>(AValue);
				style->proxy()->drawItemPixmap(APainter,FOption.rect,Qt::AlignCenter,pixmap);
				break;
			}
		case QVariant::Image:
			{
				QImage image = qvariant_cast<QImage>(AValue);
				APainter->drawImage(FOption.rect.topLeft(),image);
				break;
			}
		case QVariant::Icon:
			{
				QIcon::Mode mode = QIcon::Normal;
				if (FOption.state & QStyle::State_Selected)
					mode = QIcon::Selected;
				else if (!(FOption.state & QStyle::State_Enabled))
					mode = QIcon::Disabled;
				
				QIcon icon = qvariant_cast<QIcon>(AValue);
				QPixmap pixmap = style->generatedIconPixmap(mode,icon.pixmap(FOption.decorationSize),&FOption);
				style->proxy()->drawItemPixmap(APainter,FOption.rect,FOption.decorationAlignment,pixmap);
				break;
			}
		default:
			{
				if (!FOption.text.isEmpty())
				{
					APainter->setFont(FOption.font);
					int flags = FOption.displayAlignment | Qt::TextSingleLine;
					QPalette::ColorRole role = FOption.state & QStyle::State_Selected ? QPalette::HighlightedText : QPalette::Text;
					QString text = FOption.fontMetrics.elidedText(FOption.text,FOption.textElideMode,FOption.rect.width(),flags);
					style->proxy()->drawItemText(APainter,FOption.rect,flags,FOption.palette,(FOption.state & QStyle::State_Enabled)>0,text,role);
				}
				break;
			}
		}
	}
	void drawItem(QPainter *APainter)
	{
		if (!FOption.rect.isEmpty())
		{
			APainter->save();
			APainter->setClipRect(FOption.rect);

			if (FItem.d->hints.contains(AdvancedDelegateItem::Opacity))
				APainter->setOpacity(FItem.d->hints.value(AdvancedDelegateItem::Opacity).toReal());
			
			if (FItem.d->flags & AdvancedDelegateItem::BlinkHide)
				APainter->setOpacity(APainter->opacity() * BlinkHideSteps[BLINK_STEP]);
			else if (FItem.d->flags & AdvancedDelegateItem::BlinkFade)
				APainter->setOpacity(APainter->opacity() * BlinkFadeSteps[BLINK_STEP]);

			switch (FItem.d->kind)
			{
			case AdvancedDelegateItem::Null:
				break;
			case AdvancedDelegateItem::Stretch:
				break;
			case AdvancedDelegateItem::Branch:
				{
					QStyleOption brachOption(FOption);
					QStyle *style = FOption.widget ? FOption.widget->style() : QApplication::style();
					style->proxy()->drawPrimitive(QStyle::PE_IndicatorBranch, &brachOption, APainter);
				}
			case AdvancedDelegateItem::CheckBox:
				{
					QStyle *style = FOption.widget ? FOption.widget->style() : QApplication::style();
					style->proxy()->drawPrimitive(QStyle::PE_IndicatorViewItemCheck, &FOption, APainter, FOption.widget);
					break;
				}
			case AdvancedDelegateItem::CustomWidget:
				break;
			default:
				drawVariant(APainter,FItem.d->data);
			}
			APainter->restore();
		}
	}
private:
	mutable QSize FSizeHint;
	AdvancedDelegateItem FItem;
	QStyleOptionViewItemV4 FOption;
};

/**************************
	AdvancedDelegateEditProxy
***************************/
QWidget *AdvancedDelegateEditProxy::createEditor(const AdvancedItemDelegate *ADelegate, QWidget *AParent, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex)  const
{
	Q_UNUSED(ADelegate); Q_UNUSED(AParent); Q_UNUSED(AOption); Q_UNUSED(AIndex);
	return NULL;
}

bool AdvancedDelegateEditProxy::setEditorData(const AdvancedItemDelegate *ADelegate, QWidget *AEditor, const QModelIndex &AIndex) const
{
	Q_UNUSED(ADelegate); Q_UNUSED(AEditor); Q_UNUSED(AIndex);
	return false;
}

bool AdvancedDelegateEditProxy::setModelData(const AdvancedItemDelegate *ADelegate, QWidget *AEditor, QAbstractItemModel *AModel, const QModelIndex &AIndex) const
{
	Q_UNUSED(ADelegate); Q_UNUSED(AEditor); Q_UNUSED(AModel); Q_UNUSED(AIndex);
	return false;
}

bool AdvancedDelegateEditProxy::updateEditorGeometry(const AdvancedItemDelegate *ADelegate, QWidget *AEditor, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const
{
	Q_UNUSED(ADelegate); Q_UNUSED(AEditor); Q_UNUSED(AOption); Q_UNUSED(AIndex);
	return false;
}

/*********************
	AdvancedItemDelegate
**********************/
struct AdvancedItemDelegate::ItemsLayout
{
	QBoxLayout *mainLayout;
	QBoxLayout *middleLayout;
	QStyleOptionViewItemV4 indexOption;
	QMap<int/*id*/, AdvancedDelegateLayoutItem *> items;
	QMap<int/*position*/, QBoxLayout *> positionLayouts;
	QMap<int/*position*/, QMap<int/*floor*/, QBoxLayout *> > floorLayouts;
};

AdvancedItemDelegate::AdvancedItemDelegate(QObject *AParent) : QStyledItemDelegate(AParent)
{
	FItemsRole = Qt::UserRole+1000;
	FVerticalSpacing = -1;
	FHorizontalSpacing = -1;
	FFocusRectVisible = false;
	FDefaultBranchEnabled = false;
	FMargins = QMargins(1,1,1,1);

	FEditProxy = NULL;
	FEditItemId = AdvancedDelegateItem::NullId;
	
	FBlinkTimer.setSingleShot(false);
	FBlinkTimer.setInterval(BlinkStepsTime/BlinkStepsCount);
	connect(&FBlinkTimer,SIGNAL(timeout()),SIGNAL(updateBlinkItems()));
}

AdvancedItemDelegate::~AdvancedItemDelegate()
{

}

int AdvancedItemDelegate::itemsRole() const
{
	return FItemsRole;
}

void AdvancedItemDelegate::setItemsRole(int ARole)
{
	FItemsRole = ARole;
}

int AdvancedItemDelegate::verticalSpacing() const
{
	return FVerticalSpacing;
}

void AdvancedItemDelegate::setVertialSpacing(int ASpacing)
{
	FVerticalSpacing = ASpacing;
}

int AdvancedItemDelegate::horizontalSpacing() const
{
	return FHorizontalSpacing;
}

void AdvancedItemDelegate::setHorizontalSpacing(int ASpacing)
{
	FHorizontalSpacing = ASpacing;
}

bool AdvancedItemDelegate::focusRectVisible() const
{
	return FFocusRectVisible;
}

void AdvancedItemDelegate::setFocusRectVisible(bool AVisible) 
{
	FFocusRectVisible = AVisible;
}

bool AdvancedItemDelegate::defaultBranchItemEnabled() const
{
	return FDefaultBranchEnabled;
}

void AdvancedItemDelegate::setDefaultBranchItemEnabled(bool AEnabled)
{
	FDefaultBranchEnabled = AEnabled;
}

QMargins AdvancedItemDelegate::contentsMargins() const
{
	return FMargins;
}

void AdvancedItemDelegate::setContentsMargings(const QMargins &AMargins)
{
	FMargins = AMargins;
}

int AdvancedItemDelegate::editItemId() const
{
	return FEditItemId;
}

void AdvancedItemDelegate::setEditItemId(int AItemId)
{
	FEditItemId = AItemId;
}

AdvancedDelegateEditProxy *AdvancedItemDelegate::editProxy() const
{
	return FEditProxy;
}

void AdvancedItemDelegate::setEditProxy(AdvancedDelegateEditProxy *AProxy)
{
	FEditProxy = AProxy;
}

const QItemEditorFactory *AdvancedItemDelegate::editorFactory() const
{
	return itemEditorFactory() ? itemEditorFactory() : QItemEditorFactory::defaultFactory();
}

void AdvancedItemDelegate::paint(QPainter *APainter, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const
{
	QStyleOptionViewItemV4 indexOption = indexStyleOption(AOption,AIndex);

#if defined(Q_WS_WIN) && !defined(QT_NO_STYLE_WINDOWSVISTA)
	QStyle *style = indexOption.widget ? indexOption.widget->style() : QApplication::style();
	if (qobject_cast<QWindowsVistaStyle *>(style))
	{
		indexOption.palette.setColor(QPalette::All, QPalette::HighlightedText, indexOption.palette.color(QPalette::Active, QPalette::Text));
		indexOption.palette.setColor(QPalette::All, QPalette::Highlight, indexOption.palette.base().color().darker(108));
	}
#endif

	APainter->save();
	APainter->setClipping(true);
	APainter->setClipRect(indexOption.rect);

	drawBackground(APainter,indexOption);

	ItemsLayout *layout = createItemsLayout(getIndexItems(AIndex,indexOption),indexOption);
	layout->mainLayout->setGeometry(indexOption.rect.adjusted(FMargins.left(),FMargins.top(),-FMargins.right(),-FMargins.bottom()));
	for (QMap<int, AdvancedDelegateLayoutItem *>::const_iterator it=layout->items.constBegin(); it!=layout->items.constEnd(); ++it)
		it.value()->drawItem(APainter);
	destroyItemsLayout(layout);

	drawFocusRect(APainter,indexOption,indexOption.rect);

	APainter->restore();
}

QSize AdvancedItemDelegate::sizeHint(const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const
{
	QVariant hint = AIndex.data(Qt::SizeHintRole);
	if (hint.isValid())
		return qvariant_cast<QSize>(hint);

	QStyleOptionViewItemV4 indexOption = indexStyleOption(AOption,AIndex);
	
	// Some states are not set for sizeHint o_O
	if (AIndex.model()->hasChildren(AIndex))
		indexOption.state |= QStyle::State_Children;
	if (AIndex.sibling(AIndex.row()+1,AIndex.column()).isValid())
		indexOption.state |= QStyle::State_Sibling;
	
	ItemsLayout *layout = createItemsLayout(getIndexItems(AIndex,indexOption),indexOption);
	QSize size = layout->mainLayout->sizeHint() + QSize(FMargins.left()+FMargins.right(),FMargins.top()+FMargins.bottom());
	destroyItemsLayout(layout);

	return size;
}

QWidget *AdvancedItemDelegate::createEditor(QWidget *AParent, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const
{
	QWidget *widget = NULL;
	if (AIndex.isValid())
	{
		widget = FEditProxy!=NULL ? FEditProxy->createEditor(this,AParent,AOption,AIndex) : NULL;
		if (widget == NULL)
		{
			QVariant value = AIndex.data(Qt::EditRole);
			if (FEditItemId != AdvancedDelegateItem::NullId)
			{
				QStyleOptionViewItemV4 indexOption = indexStyleOption(AOption,AIndex);
				AdvancedDelegateItems items = getIndexItems(AIndex,indexOption);
				value = items.value(FEditItemId).d->data;
			}
			
			QVariant::Type type = static_cast<QVariant::Type>(value.userType());
			widget = editorFactory()->createEditor(type, AParent);
			
			if (widget)
				widget->setProperty(ADVANCED_DELEGATE_EDITOR_VALUE_PROPERY,value);
		}
	}
	return widget;
}

void AdvancedItemDelegate::setEditorData(QWidget *AEditor, const QModelIndex &AIndex) const
{
	if (FEditProxy==NULL || !FEditProxy->setEditorData(this,AEditor,AIndex))
	{
		QVariant value = AEditor->property(ADVANCED_DELEGATE_EDITOR_VALUE_PROPERY);
		QByteArray name = editorFactory()->valuePropertyName(value.type());
		if (!name.isEmpty()) 
			AEditor->setProperty(name, value);
	}
}

void AdvancedItemDelegate::setModelData(QWidget *AEditor, QAbstractItemModel *AModel, const QModelIndex &AIndex) const
{
	if (FEditProxy==NULL || !FEditProxy->setModelData(this,AEditor,AModel,AIndex))
	{
		QVariant value = AEditor->property(ADVANCED_DELEGATE_EDITOR_VALUE_PROPERY);
		QByteArray name = editorFactory()->valuePropertyName(value.type());
		if (!name.isEmpty()) 
			AModel->setData(AIndex, AEditor->property(name), Qt::EditRole);
	}
}

void AdvancedItemDelegate::updateEditorGeometry(QWidget *AEditor, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const
{
	if (FEditProxy==NULL || !FEditProxy->updateEditorGeometry(this,AEditor,AOption,AIndex))
	{
		int itemId = FEditItemId!=AdvancedDelegateItem::NullId ? FEditItemId : AdvancedDelegateItem::DisplayId;
		QRect rect = itemRect(itemId,AOption,AIndex);
		rect.adjust(-1,-1,1,1);
		AEditor->setGeometry(rect);
	}
}

AdvancedDelegateItems AdvancedItemDelegate::getIndexItems(const QModelIndex &AIndex, const QStyleOptionViewItemV4 &AIndexOption) const
{
	AdvancedDelegateItems items = AIndex.data(FItemsRole).value<AdvancedDelegateItems>();

	if (FDefaultBranchEnabled && (AIndexOption.state & QStyle::State_Children)>0)
	{
		AdvancedDelegateItem &branchItem = items[AdvancedDelegateItem::BranchId];
		if (branchItem.d->kind == AdvancedDelegateItem::Null)
		{
			branchItem.d->kind = AdvancedDelegateItem::Branch;
			setItemDefaults(AdvancedDelegateItem::BranchId,&branchItem);
		}
	}

	if (AIndexOption.features & QStyleOptionViewItemV4::HasCheckIndicator)
	{
		AdvancedDelegateItem &checkItem = items[AdvancedDelegateItem::CheckStateId];
		if (checkItem.d->kind == AdvancedDelegateItem::Null)
		{
			checkItem.d->kind = AdvancedDelegateItem::CheckBox;
			setItemDefaults(AdvancedDelegateItem::CheckStateId,&checkItem);
			checkItem.d->data = Qt::CheckStateRole;
		}
	}

	if (AIndexOption.features & QStyleOptionViewItemV4::HasDecoration)
	{
		AdvancedDelegateItem &decorationItem = items[AdvancedDelegateItem::DecorationId];
		if (decorationItem.d->kind == AdvancedDelegateItem::Null)
		{
			decorationItem.d->kind = AdvancedDelegateItem::Decoration;
			setItemDefaults(AdvancedDelegateItem::DecorationId,&decorationItem);
			decorationItem.d->data = Qt::DecorationRole;
		}
	}

	if (AIndexOption.features & QStyleOptionViewItemV4::HasDisplay)
	{
		AdvancedDelegateItem &displayItem = items[AdvancedDelegateItem::DisplayId];
		if (displayItem.d->kind == AdvancedDelegateItem::Null)
		{
			displayItem.d->kind = AdvancedDelegateItem::Display;
			setItemDefaults(AdvancedDelegateItem::DisplayId,&displayItem);
			displayItem.d->data = Qt::DisplayRole;
		}
	}

	AdvancedDelegateItem &stretchItem = items[AdvancedDelegateItem::DisplayStretchId];
	if (stretchItem.d->kind == AdvancedDelegateItem::Null)
	{
		stretchItem.d->kind = AdvancedDelegateItem::Stretch;
		setItemDefaults(AdvancedDelegateItem::DisplayStretchId,&stretchItem);
		stretchItem.d->sizePolicy = QSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
	}

	for (AdvancedDelegateItems::iterator it = items.begin(); it!=items.end(); ++it)
	{
		if (it->d->kind==AdvancedDelegateItem::Decoration || it->d->kind==AdvancedDelegateItem::Display || it->d->kind==AdvancedDelegateItem::CustomData)
			if (it->d->data.type() == QVariant::Int)
				it->d->data = AIndex.data(it->d->data.toInt());
	}

	return items;
}

QStyleOptionViewItemV4 AdvancedItemDelegate::indexStyleOption(const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const
{
	QStyleOptionViewItemV4 indexOption = AOption;

	indexOption.index = AIndex;

	QVariant value = AIndex.data(Qt::FontRole);
	if (value.isValid() && !value.isNull()) 
	{
		indexOption.font = qvariant_cast<QFont>(value).resolve(indexOption.font);
		indexOption.fontMetrics = QFontMetrics(indexOption.font);
	}

	value = AIndex.data(Qt::TextAlignmentRole);
	if (value.isValid() && !value.isNull())
		indexOption.displayAlignment = Qt::Alignment(value.toInt());

	value = AIndex.data(Qt::ForegroundRole);
	if (value.canConvert<QBrush>())
		indexOption.palette.setBrush(QPalette::Text, qvariant_cast<QBrush>(value));

	value = AIndex.data(Qt::CheckStateRole);
	if (value.isValid() && !value.isNull()) 
		indexOption.features |= QStyleOptionViewItemV2::HasCheckIndicator;

	value = AIndex.data(Qt::DecorationRole);
	if (value.isValid() && !value.isNull()) 
		indexOption.features |= QStyleOptionViewItemV2::HasDecoration;

	value = AIndex.data(Qt::DisplayRole);
	if (value.isValid() && !value.isNull()) 
		indexOption.features |= QStyleOptionViewItemV2::HasDisplay;

	indexOption.backgroundBrush = qvariant_cast<QBrush>(AIndex.data(Qt::BackgroundRole));

	return indexOption;
}

QStyleOptionViewItemV4 AdvancedItemDelegate::itemStyleOption(const AdvancedDelegateItem &AItem, const QStyleOptionViewItemV4 &AIndexOption) const
{
	QStyleOptionViewItemV4 itemOption = AIndexOption;

	if (AItem.d->kind == AdvancedDelegateItem::Branch)
	{
		itemOption.state &= ~QStyle::State_Sibling;
	}
	
	if (AItem.d->kind == AdvancedDelegateItem::CheckBox)
	{
		itemOption.state &= ~QStyle::State_HasFocus;
		itemOption.features |= QStyleOptionViewItemV2::HasCheckIndicator;
		itemOption.checkState = static_cast<Qt::CheckState>(AItem.d->data.toInt());

		switch (itemOption.checkState)
		{
		case Qt::Unchecked:
			itemOption.state |= QStyle::State_Off;
			break;
		case Qt::PartiallyChecked:
			itemOption.state |= QStyle::State_NoChange;
			break;
		case Qt::Checked:
			itemOption.state |= QStyle::State_On;
			break;
		}
	}
	else
	{
		itemOption.features &= ~QStyleOptionViewItemV2::HasCheckIndicator;
	}

	if (!AItem.d->data.isNull() && AItem.d->data.canConvert<QString>())
		itemOption.text = getSingleLineText(displayText(AItem.d->data,itemOption.locale));

	if (!AItem.d->hints.isEmpty())
	{
		QVariant hint = AItem.d->hints.value(AdvancedDelegateItem::FontSize);
		if (!hint.isNull())
			itemOption.font.setPointSize(hint.toInt());

		hint = AItem.d->hints.value(AdvancedDelegateItem::FontWeight);
		if (!hint.isNull())
			itemOption.font.setWeight(hint.toInt());

		hint = AItem.d->hints.value(AdvancedDelegateItem::FontHint);
		if (!hint.isNull())
			itemOption.font.setStyleHint((QFont::StyleHint)hint.toInt());

		hint = AItem.d->hints.value(AdvancedDelegateItem::FontStyle);
		if (!hint.isNull())
			itemOption.font.setStyle((QFont::Style)hint.toInt());

		hint = AItem.d->hints.value(AdvancedDelegateItem::FontUnderline);
		if (!hint.isNull())
			itemOption.font.setUnderline(hint.toBool());

		hint = AItem.d->hints.value(AdvancedDelegateItem::StatesForceOn);
		if (!hint.isNull())
			itemOption.state |= (QStyle::State)hint.toInt();

		hint = AItem.d->hints.value(AdvancedDelegateItem::StatesForceOff);
		if (!hint.isNull())
			itemOption.state &= ~(QStyle::State)hint.toInt();

		hint = AItem.d->hints.value(AdvancedDelegateItem::Foreground);
		if (!hint.isNull())
			itemOption.palette.setBrush(QPalette::Text, qvariant_cast<QBrush>(hint));

		hint = AItem.d->hints.value(AdvancedDelegateItem::Backgound);
		if (!hint.isNull())
			itemOption.backgroundBrush = qvariant_cast<QBrush>(hint);

		hint = AItem.d->hints.value(AdvancedDelegateItem::TextElideMode);
		if (!hint.isNull())
			itemOption.textElideMode = (Qt::TextElideMode)hint.toInt();

		hint = AItem.d->hints.value(AdvancedDelegateItem::TextAlignment);
		if (!hint.isNull())
			itemOption.displayAlignment = (Qt::Alignment)hint.toInt();

		itemOption.fontMetrics = QFontMetrics(itemOption.font);
	}

	return itemOption;
}

AdvancedItemDelegate::ItemsLayout *AdvancedItemDelegate::createItemsLayout(const AdvancedDelegateItems &AItems, const QStyleOptionViewItemV4 &AIndexOption) const
{
	QStyle *style = AIndexOption.widget ? AIndexOption.widget->style() : QApplication::style();

	const int vSpacing = FVerticalSpacing<0 ? style->proxy()->pixelMetric(QStyle::PM_FocusFrameVMargin)+1 : FVerticalSpacing;
	const int hSpacing = FHorizontalSpacing<0 ? style->proxy()->pixelMetric(QStyle::PM_FocusFrameHMargin)+1 : FHorizontalSpacing;

	ItemsLayout *layout = new ItemsLayout;
	layout->middleLayout = NULL;
	layout->indexOption = AIndexOption;

	layout->mainLayout = new QVBoxLayout;
	layout->mainLayout->setSpacing(vSpacing);

	QMap<int, QMap<int, QMultiMap<int, AdvancedDelegateLayoutItem *> > > orderedItems;
	for (AdvancedDelegateItems::const_iterator it = AItems.constBegin(); it!=AItems.constEnd(); ++it)
	{
		QStyleOptionViewItemV4 itemOption = itemStyleOption(it.value(),AIndexOption);
		if (isItemVisible(it.value(),itemOption))
		{
			QMap<int, QBoxLayout *> &posLayouts = layout->positionLayouts;
			QBoxLayout *&posLayout = posLayouts[it->d->position];
			if (posLayout == NULL)
			{
				posLayout = new QVBoxLayout;
				posLayout->setSpacing(vSpacing);
			}

			QMap<int, QBoxLayout *> &floorLayouts = layout->floorLayouts[it->d->position];
			QBoxLayout *&floorLayout = floorLayouts[it->d->floor];
			if (floorLayout == NULL)
			{
				floorLayout = new QHBoxLayout;
				floorLayout->setSpacing(hSpacing);
			}

			AdvancedDelegateLayoutItem *item = new AdvancedDelegateLayoutItem(it.value(),itemOption);
			layout->items.insert(it.key(),item);
			orderedItems[it->d->position][it->d->floor].insertMulti(it->d->order,item);
		}
	}

	for (QMap<int, QMap<int, QMultiMap<int, AdvancedDelegateLayoutItem *> > >::const_iterator pos_it=orderedItems.constBegin(); pos_it!=orderedItems.constEnd(); ++pos_it)
	{
		QBoxLayout *posLayout = layout->positionLayouts.value(pos_it.key());

		if (pos_it.key()>=AdvancedDelegateItem::MiddleLeft && pos_it.key()<=AdvancedDelegateItem::MiddleRight)
		{
			if (layout->middleLayout == NULL)
			{
				layout->middleLayout = new QHBoxLayout;
				layout->middleLayout->setSpacing(hSpacing);
				layout->mainLayout->addLayout(layout->middleLayout);
			}
			layout->middleLayout->addLayout(posLayout);
		}
		else
		{
			layout->mainLayout->addLayout(posLayout);
		}

		QMap<int, QBoxLayout *> &floorLayouts = layout->floorLayouts[pos_it.key()];
		for (QMap<int, QMultiMap<int, AdvancedDelegateLayoutItem *> >::const_iterator floor_it=pos_it->constBegin(); floor_it!=pos_it->constEnd(); ++ floor_it)
		{
			QBoxLayout *floorLayout = floorLayouts.value(floor_it.key());
			posLayout->addLayout(floorLayout);
			for (QMultiMap<int, AdvancedDelegateLayoutItem *>::const_iterator order_it=floor_it->constBegin(); order_it!=floor_it->constEnd(); ++order_it)
			{
				floorLayout->addItem(order_it.value());
			}
		}
	}

	return layout;
}

void AdvancedItemDelegate::destroyItemsLayout(ItemsLayout *ALayout) const
{
	destroyLayoutRecursive(ALayout->mainLayout);
	delete ALayout;
}

QRect AdvancedItemDelegate::itemRect(int AItemId, const ItemsLayout *ALayout, const QRect &AGeometry) const
{
	QRect rect;
	if (ALayout->items.contains(AItemId))
	{
		if (ALayout->mainLayout->geometry() != AGeometry)
			ALayout->mainLayout->setGeometry(AGeometry.adjusted(FMargins.left(),FMargins.top(),-FMargins.right(),-FMargins.bottom()));
		rect = ALayout->items.value(AItemId)->geometry();
	}
	return rect;
}

QRect AdvancedItemDelegate::itemRect(int AItemId, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const
{
	QRect rect;
	if (AIndex.isValid() && !AOption.rect.isEmpty())
	{
		QStyleOptionViewItemV4 indexOption = indexStyleOption(AOption,AIndex);
		ItemsLayout *layout = createItemsLayout(getIndexItems(AIndex,indexOption),indexOption);
		rect = itemRect(AItemId,layout,indexOption.rect);
		destroyItemsLayout(layout);
	}
	return rect;
}

int AdvancedItemDelegate::itemAt(const QPoint &APoint, const ItemsLayout *ALayout, const QRect &AGeometry) const
{
	if (AGeometry.contains(APoint))
	{
		if (ALayout->mainLayout->geometry() != AGeometry)
			ALayout->mainLayout->setGeometry(AGeometry.adjusted(FMargins.left(),FMargins.top(),-FMargins.right(),-FMargins.bottom()));

		for (QMap<int,AdvancedDelegateLayoutItem *>::const_iterator it=ALayout->items.constBegin(); it!=ALayout->items.constEnd(); ++it)
		{
			if (it.value()->geometry().contains(APoint))
				return it.key();
		}
	}
	return AdvancedDelegateItem::NullId;
}

int AdvancedItemDelegate::itemAt(const QPoint &APoint, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const
{
	int itemId = AdvancedDelegateItem::NullId;
	if (AIndex.isValid() && !AOption.rect.isEmpty())
	{
		QStyleOptionViewItemV4 indexOption = indexStyleOption(AOption,AIndex);
		ItemsLayout *layout = createItemsLayout(getIndexItems(AIndex,indexOption),indexOption);
		itemId = itemAt(APoint,layout,indexOption.rect);
		destroyItemsLayout(layout);
	}
	return itemId;
}

QSize AdvancedItemDelegate::itemSizeHint(const AdvancedDelegateItem &AItem, const QStyleOptionViewItemV4 &AItemOption)
{
	static const QSize zeroSize = QSize(0,0);
	static const QSize branchSize = QSize(12,12);

	QStyle *style = AItemOption.widget!=NULL ? AItemOption.widget->style() : QApplication::style();

	switch (AItem.d->kind)
	{
	case AdvancedDelegateItem::Null:
	case AdvancedDelegateItem::Stretch:
		{
			return zeroSize;
		}
	case AdvancedDelegateItem::Branch:
		{
			return branchSize;
		}
	case AdvancedDelegateItem::CheckBox:
		{
			int width = style->proxy()->pixelMetric(QStyle::PM_IndicatorWidth, &AItemOption, AItemOption.widget);
			int height = style->proxy()->pixelMetric(QStyle::PM_IndicatorHeight, &AItemOption, AItemOption.widget);
			return QSize(width,height);
		}
	case AdvancedDelegateItem::CustomWidget:
		{
			return AItem.d->widget!=NULL ? AItem.d->widget->sizeHint() : zeroSize;
		}
	default:
		switch (AItem.d->data.type())
		{
		case QVariant::Pixmap:
			{
				QPixmap pixmap = qvariant_cast<QPixmap>(AItem.d->data);
				if (!pixmap.isNull())
					return pixmap.size();
				break;
			}
		case QVariant::Image:
			{
				QImage image = qvariant_cast<QImage>(AItem.d->data);
				if (!image.isNull())
					return image.size();
				break;
			}
		case QVariant::Icon:
			{
				QIcon icon = qvariant_cast<QIcon>(AItem.d->data);
				if (!icon.isNull())
					return icon.actualSize(AItemOption.decorationSize);
				break;
			}
		default:
			{
				if (!AItemOption.text.isEmpty())
					return AItemOption.fontMetrics.size(AItemOption.direction|Qt::TextSingleLine,AItemOption.text);
				break;
			}
		}
		return zeroSize;
	}
}

bool AdvancedItemDelegate::isItemVisible(const AdvancedDelegateItem &AItem, const QStyleOptionViewItemV4 &AItemOption)
{
	if (( AItemOption.state & AItem.d->showStates) != AItem.d->showStates)
		return false;

	if (( AItemOption.state & AItem.d->hideStates) > 0)
		return false;

	switch (AItem.d->kind)
	{
	case AdvancedDelegateItem::Null:
		return false;
	case AdvancedDelegateItem::Branch:
		return (AItemOption.state & QStyle::State_Children)>0;
	case AdvancedDelegateItem::CheckBox:
		return (AItemOption.features & QStyleOptionViewItemV4::HasCheckIndicator)>0;
	case AdvancedDelegateItem::Stretch:
		return true;
	case AdvancedDelegateItem::CustomWidget:
		return AItem.d->widget!=NULL;
	default:
		return !AItem.d->data.isNull();
	}
}

void AdvancedItemDelegate::drawBackground(QPainter *APainter, const QStyleOptionViewItemV4 &AIndexOption) const
{
	QStyle *style = AIndexOption.widget ? AIndexOption.widget->style() : QApplication::style();
	style->proxy()->drawPrimitive(QStyle::PE_PanelItemViewItem,&AIndexOption,APainter,AIndexOption.widget);
}

void AdvancedItemDelegate::drawFocusRect(QPainter *APainter, const QStyleOptionViewItemV4 &AIndexOption, const QRect &ARect) const
{
	Q_UNUSED(ARect);
	if (FFocusRectVisible && (AIndexOption.state & QStyle::State_HasFocus)>0)
	{
		QStyle *style = AIndexOption.widget ? AIndexOption.widget->style() : QApplication::style();

		QStyleOptionFocusRect focusOption;
		focusOption.QStyleOption::operator=(AIndexOption);
		focusOption.rect = style->proxy()->subElementRect(QStyle::SE_ItemViewItemFocusRect, &AIndexOption, AIndexOption.widget);
		focusOption.state |= QStyle::State_KeyboardFocusChange|QStyle::State_Item;

		QPalette::ColorGroup cg = (AIndexOption.state & QStyle::State_Enabled) ? QPalette::Normal : QPalette::Disabled;
		QPalette::ColorRole cr = (AIndexOption.state & QStyle::State_Selected) ? QPalette::Highlight : QPalette::Window;
		focusOption.backgroundColor = AIndexOption.palette.color(cg,cr);

		style->proxy()->drawPrimitive(QStyle::PE_FrameFocusRect, &focusOption, APainter);
	}
}

bool AdvancedItemDelegate::editorEvent(QEvent *AEvent, QAbstractItemModel *AModel, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex)
{
	Qt::ItemFlags flags = AModel->flags(AIndex);
	if ((flags & Qt::ItemIsUserCheckable)==0 || (AOption.state & QStyle::State_Enabled)==0 || (flags & Qt::ItemIsEnabled)==0)
		return false;

	QVariant value = AIndex.data(Qt::CheckStateRole);
	if (value.isNull() || !value.isValid())
		return false;

	if ((AEvent->type()==QEvent::MouseButtonRelease) || (AEvent->type()==QEvent::MouseButtonDblClick) || (AEvent->type()==QEvent::MouseButtonPress)) 
	{
		QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(AEvent);
		if (mouseEvent->button()!=Qt::LeftButton)
			return false;

		QRect checkRect = itemRect(AdvancedDelegateItem::CheckStateId,AOption,AIndex);
		if (!checkRect.contains(mouseEvent->pos()))
			return false;
		else if ((AEvent->type()==QEvent::MouseButtonPress) || (AEvent->type()==QEvent::MouseButtonDblClick))
			return true;
	} 
	else if (AEvent->type() == QEvent::KeyPress) 
	{
		QKeyEvent *keyEvent = static_cast<QKeyEvent *>(AEvent);
		if (keyEvent->key()!=Qt::Key_Space && keyEvent->key()!=Qt::Key_Select)
			return false;
	} 
	else 
	{
		return false;
	}

	Qt::CheckState state;
	if ((flags & Qt::ItemIsTristate) > 0)
		state = static_cast<Qt::CheckState>((value.toInt()+1) % 3);
	else if (value.toInt() == Qt::Unchecked)
		state = Qt::Checked;
	else
		state =  Qt::Unchecked;

	return AModel->setData(AIndex, state, Qt::CheckStateRole);
}
