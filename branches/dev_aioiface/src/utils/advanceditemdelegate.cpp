#include "advanceditemdelegate.h"

#include <QStyle>
#include <QPainter>
#include <QKeyEvent>
#include <QMultiMap>
#include <QMouseEvent>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QApplication>
#include <QWindowsVistaStyle>

AdvancedDelegateItem::AdvancedDelegateItem()
{
	d = new AdvancedDelegateItemData;
	d->refs = 1;
	d->kind = Null;
	d->order = 0;
	d->floor = 500;
	d->position = Middle;
	d->flags = 0;
	d->showStates = 0;
	d->hideStates = 0;
	d->widget = NULL;
	d->alignment = Qt::AlignLeft;
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

bool AdvancedDelegateItem::operator <(const AdvancedDelegateItem &AItem) const 
{
	return d->order < AItem.d->order;
}


class AdvancedDelegateLayoutItem :
	public QLayoutItem
{
public:
	AdvancedDelegateLayoutItem(const AdvancedDelegateItem &AItem, const QStyleOptionViewItemV4 &AOption) : 
			QLayoutItem(AItem.d->alignment), FItem(AItem), FOption(AOption) 
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
private:
	mutable QSize FSizeHint;
	AdvancedDelegateItem FItem;
	QStyleOptionViewItemV4 FOption;
};


struct AdvancedItemDelegate::ItemsLayout
{
	QHBoxLayout *mainLayout;
	QStyleOptionViewItemV4 indexOption;
	QMap<int/*id*/, AdvancedDelegateLayoutItem *> items;
	QMap<int/*position*/, QVBoxLayout *> positionLayouts;
	QMap<int/*position*/, QMap<int/*floor*/, QHBoxLayout *> > floorLayouts;
};

template <class T>
void insertLayout(int AOrder, T *ALayout, const QMap<int, T *> &AChilds, QBoxLayout *AParent)
{
	QMap<int, T *>::const_iterator pos_it = AChilds.upperBound(AOrder);
	T *before = pos_it!=AChilds.constEnd() ? pos_it.value() : NULL;
	if (before != NULL)
	{
		for (int index = 0; index<AParent->count(); index++)
		{
			if (AParent->itemAt(index) == before)
			{
				AParent->insertLayout(index,ALayout);
				break;
			}
		}
	}
	else
	{
		AParent->addLayout(ALayout);
	}
}

QIcon::Mode getIconMode(QStyle::State AState)
{
	if (!(AState & QStyle::State_Enabled))
		return QIcon::Disabled;
	if (AState & QStyle::State_Selected)
		return QIcon::Selected;
	return QIcon::Normal;
}

QIcon::State getIconState(QStyle::State AState)
{
	return AState & QStyle::State_Open ? QIcon::On : QIcon::Off;
}

QString getSingleLineText(const QString &AText)
{
	QString text = AText;
	text.replace('\n',' ');
	return text.trimmed();
}

void drawLayoutItem(QPainter *APainter, const AdvancedDelegateLayoutItem *ALayoutItem)
{
	if (!ALayoutItem->geometry().isEmpty())
	{
		const AdvancedDelegateItem &item = ALayoutItem->delegateItem();
		const QStyleOptionViewItemV4 &option = ALayoutItem->itemOption();
		QStyle *style = option.widget ? option.widget->style() : QApplication::style();

		APainter->save();
		APainter->setClipRect(option.rect);

		switch (item.d->kind)
		{
		case AdvancedDelegateItem::Null:
			break;
		case AdvancedDelegateItem::Stretch:
			break;
		case AdvancedDelegateItem::Branch:
			{
				QStyleOption brachOption(option);
				style->proxy()->drawPrimitive(QStyle::PE_IndicatorBranch, &brachOption, APainter);
			}
		case AdvancedDelegateItem::CheckState:
			{
				style->proxy()->drawPrimitive(QStyle::PE_IndicatorViewItemCheck, &option, APainter, option.widget);
				break;
			}
		case AdvancedDelegateItem::CustomWidget:
			break;
		default:
			switch (item.d->data.type())
			{
			case QVariant::Pixmap:
				{
					QPixmap pixmap = qvariant_cast<QPixmap>(item.d->data);
					style->drawItemPixmap(APainter,option.rect,Qt::AlignCenter,pixmap);
					break;
				}
			case QVariant::Image:
				{
					QImage image = qvariant_cast<QImage>(item.d->data);
					APainter->drawImage(option.rect.topLeft(),image);
					break;
				}
			case QVariant::Icon:
				{
					QIcon icon = qvariant_cast<QIcon>(item.d->data);
					QPixmap pixmap = style->generatedIconPixmap(getIconMode(option.state),icon.pixmap(option.decorationSize),&option);
					style->drawItemPixmap(APainter,option.rect,Qt::AlignCenter,pixmap);
					break;
				}
			default:
				{
					QString itemText = getSingleLineText(item.d->data.toString());
					if (!itemText.isEmpty())
					{
						APainter->setFont(option.font);
						int flags = option.direction | Qt::TextSingleLine;
						QPalette::ColorRole role = option.state & QStyle::State_Selected ? QPalette::HighlightedText : QPalette::Text;
						QString text = option.fontMetrics.elidedText(itemText,option.textElideMode,option.rect.width(),flags);
						style->proxy()->drawItemText(APainter,option.rect,flags,option.palette,(option.state & QStyle::State_Enabled)>0,text,role);
					}
					break;
				}
			}
		}

		APainter->restore();
	}
}

void destroyLayoutRecursive(QLayout *ALayout)
{
	QLayoutItem *child;
	while (child = ALayout->takeAt(0))
	{
		if (child->layout())
			destroyLayoutRecursive(child->layout());
		else
			delete child;
	}
	delete ALayout;
}

AdvancedItemDelegate::AdvancedItemDelegate(QObject *AParent) : QStyledItemDelegate(AParent)
{
	FItemsRole = Qt::UserRole+1000;
	FVerticalSpacing = -1;
	FHorizontalSpacing = -1;
	FFocusRectVisible = false;
	FDefaultBranchEnabled = false;
	FMargins = QMargins(1,1,1,1);
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

void AdvancedItemDelegate::paint(QPainter *APainter, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const
{
	QStyleOptionViewItemV4 option = indexStyleOption(AOption,AIndex);

#if defined(Q_WS_WIN) && !defined(QT_NO_STYLE_WINDOWSVISTA)
	QStyle *style = option.widget ? option.widget->style() : QApplication::style();
	if (qobject_cast<QWindowsVistaStyle *>(style))
	{
		option.palette.setColor(QPalette::All, QPalette::HighlightedText, option.palette.color(QPalette::Active, QPalette::Text));
		option.palette.setColor(QPalette::All, QPalette::Highlight, option.palette.base().color().darker(108));
	}
#endif

	APainter->save();
	APainter->setClipping(true);
	APainter->setClipRect(option.rect);

	drawBackground(APainter,option);

	ItemsLayout *layout = createItemsLayout(getIndexItems(AIndex),option);
	layout->mainLayout->setGeometry(option.rect.adjusted(FMargins.left(),FMargins.top(),-FMargins.right(),-FMargins.bottom()));
	for (QMap<int, AdvancedDelegateLayoutItem *>::const_iterator it=layout->items.constBegin(); it!=layout->items.constEnd(); ++it)
		drawLayoutItem(APainter,it.value());
	destroyItemsLayout(layout);

	drawFocusRect(APainter,option,option.rect);

	APainter->restore();
}

QSize AdvancedItemDelegate::sizeHint(const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const
{
	QVariant hint = AIndex.data(Qt::SizeHintRole);
	if (hint.isValid())
		return qvariant_cast<QSize>(hint);

	QStyleOptionViewItemV4 option = indexStyleOption(AOption,AIndex);
	
	// Some states are not set for sizeHint o_O
	if (AIndex.model()->hasChildren(AIndex))
		option.state |= QStyle::State_Children;
	if (AIndex.sibling(AIndex.row()+1,AIndex.column()).isValid())
		option.state |= QStyle::State_Sibling;
	
	ItemsLayout *layout = createItemsLayout(getIndexItems(AIndex),option);
	QSize size = layout->mainLayout->sizeHint() + QSize(FMargins.left()+FMargins.right(),FMargins.top()+FMargins.bottom());
	destroyItemsLayout(layout);

	return size;
}

AdvancedDelegateItems AdvancedItemDelegate::getIndexItems(const QModelIndex &AIndex) const
{
	AdvancedDelegateItems items = AIndex.data(FItemsRole).value<AdvancedDelegateItems>();

	if (FDefaultBranchEnabled)
	{
		AdvancedDelegateItem &branchItem = items[AdvancedDelegateItem::BranchId];
		if (branchItem.d->kind == AdvancedDelegateItem::Null)
		{
			branchItem.d->kind = AdvancedDelegateItem::Branch;
			branchItem.d->position = AdvancedDelegateItem::Left;
			branchItem.d->order = 100;
		}
	}

	AdvancedDelegateItem &checkItem = items[AdvancedDelegateItem::CheckStateId];
	if (checkItem.d->kind == AdvancedDelegateItem::Null)
	{
		checkItem.d->kind = AdvancedDelegateItem::CheckState;
		checkItem.d->position = AdvancedDelegateItem::Left;
		checkItem.d->order = 100;
	}
	checkItem.d->data = AIndex.data(Qt::CheckStateRole);

	AdvancedDelegateItem &decorationItem = items[AdvancedDelegateItem::DecorationId];
	if (decorationItem.d->kind == AdvancedDelegateItem::Null)
	{
		decorationItem.d->kind = AdvancedDelegateItem::Decoration;
		decorationItem.d->position = AdvancedDelegateItem::Left;
		decorationItem.d->order = 500;
	}
	decorationItem.d->data = AIndex.data(Qt::DecorationRole);

	AdvancedDelegateItem &displayItem = items[AdvancedDelegateItem::DisplayId];
	if (displayItem.d->kind == AdvancedDelegateItem::Null)
	{
		displayItem.d->kind = AdvancedDelegateItem::Display;
		displayItem.d->position = AdvancedDelegateItem::Middle;
		displayItem.d->order = 500;
		displayItem.d->sizePolicy = QSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed);
	}
	displayItem.d->data = AIndex.data(Qt::DisplayRole);

	AdvancedDelegateItem &stretch0Item = items[AdvancedDelegateItem::Stretch0Id];
	if (stretch0Item.d->kind == AdvancedDelegateItem::Null)
	{
		stretch0Item.d->kind = AdvancedDelegateItem::Stretch;
		stretch0Item.d->position = AdvancedDelegateItem::Middle;
		stretch0Item.d->order = 10000;
		stretch0Item.d->sizePolicy = QSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
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
	QStyleOptionViewItemV4 option = AOption;
	initStyleOption(&option,AIndex);
	return option;
}

AdvancedItemDelegate::ItemsLayout *AdvancedItemDelegate::createItemsLayout(const AdvancedDelegateItems &AItems, const QStyleOptionViewItemV4 &AIndexOption) const
{
	QStyle *style = AIndexOption.widget ? AIndexOption.widget->style() : QApplication::style();

	const int vSpacing = FVerticalSpacing<0 ? style->proxy()->pixelMetric(QStyle::PM_LayoutVerticalSpacing) : FVerticalSpacing;
	const int hSpacing = FHorizontalSpacing<0 ? style->proxy()->pixelMetric(QStyle::PM_LayoutHorizontalSpacing) : FHorizontalSpacing;

	ItemsLayout *layout = new ItemsLayout;
	layout->indexOption = AIndexOption;

	layout->mainLayout = new QHBoxLayout;
	layout->mainLayout->setSpacing(hSpacing);

	QMap<int, QMap<int, QMultiMap<int, AdvancedDelegateLayoutItem *> > > orderedItems;
	for (AdvancedDelegateItems::const_iterator it = AItems.constBegin(); it!=AItems.constEnd(); ++it)
	{
		QStyleOptionViewItemV4 option = itemStyleOption(it.value(),AIndexOption);
		if (isItemVisible(it.value(),option))
		{
			QMap<int, QVBoxLayout *> &posLayouts = layout->positionLayouts;
			QVBoxLayout *&posLayout = posLayouts[it->d->position];
			if (posLayout == NULL)
			{
				posLayout = new QVBoxLayout;
				posLayout->setSpacing(vSpacing);
				insertLayout<QVBoxLayout>(it->d->position,posLayout,posLayouts,layout->mainLayout);
			}

			QMap<int, QHBoxLayout *> &floorLayouts = layout->floorLayouts[it->d->position];
			QHBoxLayout *&floorLayout = floorLayouts[it->d->floor];
			if (floorLayout == NULL)
			{
				floorLayout = new QHBoxLayout;
				floorLayout->setSpacing(hSpacing);
				insertLayout<QHBoxLayout>(it->d->floor,floorLayout,floorLayouts,posLayout);
			}

			AdvancedDelegateLayoutItem *item = new AdvancedDelegateLayoutItem(it.value(),option);
			layout->items.insert(it.key(),item);
			orderedItems[it->d->position][it->d->floor].insertMulti(it->d->order,item);
		}
	}

	for (QMap<int, QMap<int, QMultiMap<int, AdvancedDelegateLayoutItem *> > >::const_iterator pos_it=orderedItems.constBegin(); pos_it!=orderedItems.constEnd(); ++pos_it)
	{
		QMap<int, QHBoxLayout *> &floorLayouts = layout->floorLayouts[pos_it.key()];
		for (QMap<int, QMultiMap<int, AdvancedDelegateLayoutItem *> >::const_iterator floor_it=pos_it->constBegin(); floor_it!=pos_it->constEnd(); ++ floor_it)
		{
			QHBoxLayout *floorLayout = floorLayouts[floor_it.key()];
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
		QStyleOptionViewItemV4 option = indexStyleOption(AOption,AIndex);
		ItemsLayout *layout = createItemsLayout(getIndexItems(AIndex),option);
		rect = itemRect(AItemId,layout,option.rect);
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
		QStyleOptionViewItemV4 option = indexStyleOption(AOption,AIndex);
		ItemsLayout *layout = createItemsLayout(getIndexItems(AIndex),option);
		itemId = itemAt(APoint,layout,option.rect);
		destroyItemsLayout(layout);
	}
	return itemId;
}

QStyleOptionViewItemV4 AdvancedItemDelegate::itemStyleOption(const AdvancedDelegateItem &AItem, const QStyleOptionViewItemV4 &AIndexOption)
{
	QStyleOptionViewItemV4 option = AIndexOption;

	if (AItem.d->kind == AdvancedDelegateItem::CheckState)
	{
		option.state = option.state & ~QStyle::State_HasFocus;
		switch (option.checkState) 
		{
		case Qt::Unchecked:
			option.state |= QStyle::State_Off;
			break;
		case Qt::PartiallyChecked:
			option.state |= QStyle::State_NoChange;
			break;
		case Qt::Checked:
			option.state |= QStyle::State_On;
			break;
		}
	}

	if (!AItem.d->hints.isEmpty())
	{
		QVariant hint = AItem.d->hints.value(AdvancedDelegateItem::FontSize);
		if (!hint.isNull())
			option.font.setPointSize(hint.toInt());

		hint = AItem.d->hints.value(AdvancedDelegateItem::FontWeight);
		if (!hint.isNull())
			option.font.setWeight(hint.toInt());

		hint = AItem.d->hints.value(AdvancedDelegateItem::FontHint);
		if (!hint.isNull())
			option.font.setStyleHint((QFont::StyleHint)hint.toInt());

		hint = AItem.d->hints.value(AdvancedDelegateItem::FontStyle);
		if (!hint.isNull())
			option.font.setStyle((QFont::Style)hint.toInt());

		hint = AItem.d->hints.value(AdvancedDelegateItem::FontUnderline);
		if (!hint.isNull())
			option.font.setUnderline(hint.toBool());

		hint = AItem.d->hints.value(AdvancedDelegateItem::StatesForceOn);
		if (!hint.isNull())
			option.state |= (QStyle::State)hint.toInt();

		hint = AItem.d->hints.value(AdvancedDelegateItem::StatesForceOff);
		if (!hint.isNull())
			option.state &= ~(QStyle::State)hint.toInt();

		hint = AItem.d->hints.value(AdvancedDelegateItem::Foreground);
		if (!hint.isNull())
			option.palette.setBrush(QPalette::Text, qvariant_cast<QBrush>(hint));

		hint = AItem.d->hints.value(AdvancedDelegateItem::Backgound);
		if (!hint.isNull())
			option.backgroundBrush = qvariant_cast<QBrush>(hint);

		hint = AItem.d->hints.value(AdvancedDelegateItem::TextElideMode);
		if (!hint.isNull())
			option.textElideMode = (Qt::TextElideMode)hint.toInt();

		hint = AItem.d->hints.value(AdvancedDelegateItem::TextAlignment);
		if (!hint.isNull())
			option.displayAlignment = (Qt::Alignment)hint.toInt();
	}

	option.fontMetrics = QFontMetrics(option.font);

	return option;
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
	case AdvancedDelegateItem::CheckState:
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
				QString itemText = getSingleLineText(AItem.d->data.toString());
				if (!itemText.isEmpty())
					return AItemOption.fontMetrics.size(AItemOption.direction|Qt::TextSingleLine,itemText);
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
	case AdvancedDelegateItem::CheckState:
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
	if (!value.isValid())
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
