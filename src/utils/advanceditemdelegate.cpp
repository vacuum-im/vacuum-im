#include "advanceditemdelegate.h"

#include <QStyle>
#include <QPainter>
#include <QKeyEvent>
#include <QMultiMap>
#include <QDateTime>
#include <QDataStream>
#include <QLayoutItem>
#include <QMouseEvent>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QApplication>
#include <QItemEditorFactory>

static const qreal BlinkHideSteps[] = { 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
static const qreal BlinkFadeSteps[] = { 1.0, 0.8, 0.6, 0.4, 0.2, 0.2, 0.4, 0.6, 0.8, 1.0 };
static const int BlinkStepsCount = sizeof(BlinkHideSteps)/sizeof(BlinkHideSteps[0]);
static const int BlinkStepsTime = 1000;
#define BLINK_STEP ((QDateTime::currentMSecsSinceEpoch() % BlinkStepsTime) * BlinkStepsCount / BlinkStepsTime)

const quint32 AdvancedDelegateItem::NullId        = 0;
const quint32 AdvancedDelegateItem::BranchId      = AdvancedDelegateItem::makeId(AdvancedDelegateItem::MiddleLeft,128,10);
const quint32 AdvancedDelegateItem::CheckStateId  = AdvancedDelegateItem::makeId(AdvancedDelegateItem::MiddleLeft,128,100);
const quint32 AdvancedDelegateItem::DecorationId  = AdvancedDelegateItem::makeId(AdvancedDelegateItem::MiddleLeft,128,500);
const quint32 AdvancedDelegateItem::DisplayId     = AdvancedDelegateItem::makeId(AdvancedDelegateItem::MiddleCenter,128,500);

void registerAdvancedDelegateItemStreamOperators()
{
	static bool typeStreamOperatorsRegistered = false;
	if (!typeStreamOperatorsRegistered)
	{
		typeStreamOperatorsRegistered = true;
		qRegisterMetaTypeStreamOperators<AdvancedDelegateItem>("AdvancedDelegateItem");
		qRegisterMetaTypeStreamOperators<AdvancedDelegateItems>("AdvancedDelegateItems");
	}
}

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
	static const QChar space(' ');
	static const QChar lineBreak('\n');
	static const QChar lineSep(QChar::LineSeparator);
	static const QChar paragraphSep(QChar::ParagraphSeparator);

	QString text = AText;
	for (int i=0; i<text.size(); i++)
	{
		const QChar textChar = text.at(i);
		if (textChar==lineBreak || textChar==lineSep || textChar==paragraphSep)
			text[i] = space;
	}

	return text.trimmed();
}

/*********************
 AdvancedDelegateItem
**********************/
QDataStream &operator<<(QDataStream &AStream, const AdvancedDelegateItem &AItem)
{
	AStream 
		<< AItem.d->id
		<< AItem.d->kind
		<< AItem.d->flags
		<< AItem.d->data
		<< (qint64)AItem.d->widget
		<< AItem.d->sizePolicy
		<< (int)AItem.d->showStates
		<< (int)AItem.d->hideStates
		<< AItem.d->hints
		<< AItem.d->properties
		<< AItem.c->value
		<< AItem.c->blinkOpacity;

	return AStream;
}

QDataStream &operator>>(QDataStream &AStream, AdvancedDelegateItem &AItem)
{
	qint64 widget;
	int showStates, hideStates;

	AStream 
		>> AItem.d->id 
		>> AItem.d->kind 
		>> AItem.d->flags 
		>> AItem.d->data 
		>> widget
		>> AItem.d->sizePolicy
		>> showStates
		>> hideStates
		>> AItem.d->hints
		>> AItem.d->properties
		>> AItem.c->value
		>> AItem.c->blinkOpacity;

	AItem.d->widget = (QWidget *)widget;
	AItem.d->showStates = (QStyle::State)showStates;
	AItem.d->hideStates = (QStyle::State)hideStates;

	return AStream;
}

AdvancedDelegateItem::AdvancedDelegateItem(quint32 AId)
{
	d = new ExplicitData;
	d->refs = 1;
	d->id = AId;
	d->kind = Null;
	d->flags = 0;
	d->showStates = 0;
	d->hideStates = 0;
	d->widget = NULL;

	c = new ContextData;
	c->blinkOpacity = 1.0;

	registerAdvancedDelegateItemStreamOperators();
}

AdvancedDelegateItem::AdvancedDelegateItem(const AdvancedDelegateItem &AOther)
{
	d = AOther.d;
	d->refs++;

	c = new ContextData;
	*c = *AOther.c;
}

AdvancedDelegateItem::~AdvancedDelegateItem()
{
	if (--d->refs == 0)
		delete d;
	delete c;
}

void AdvancedDelegateItem::detach()
{
	if (d->refs > 1)
	{
		ExplicitData *old = d;
		old->refs--;

		d = new ExplicitData;
		*d = *old;
		d->refs=1;
	}
}

AdvancedDelegateItem &AdvancedDelegateItem::operator=(const AdvancedDelegateItem &AOther)
{
	if (d != AOther.d)
	{
		if (--d->refs == 0)
			delete d;
		d = AOther.d;
		d->refs++;
	}

	*c = *AOther.c;
	return *this;
}

quint32 AdvancedDelegateItem::makeId(quint8 APosition, quint8 AFloor, quint16 AOrder)
{
	return (((quint32)(APosition+1))<<24) + (((quint32)AFloor)<<16) + (quint32)AOrder;
}

quint32 AdvancedDelegateItem::makeStretchId( quint8 APosition, quint8 AFloor )
{
	return makeId(APosition,AFloor,AlignRightOrderMask);
}

quint8 AdvancedDelegateItem::getPosition(quint32 AItemId)
{
	return (AItemId >> 24) - 1;
}

quint8 AdvancedDelegateItem::getFloor(quint32 AItemId)
{
	return (AItemId & 0x00FF0000) >> 16;
}

quint16 AdvancedDelegateItem::getOrder(quint32 AItemId)
{
	return AItemId & 0x0000FFFF;
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
		if (FItem.d->widget != NULL)
			FItem.d->widget->setGeometry(ARect);
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
			APainter->setOpacity(APainter->opacity() * FItem.c->blinkOpacity);
			
			switch (FItem.d->kind)
			{
			case AdvancedDelegateItem::Null:
				{
					break;
				}
			case AdvancedDelegateItem::Stretch:
				{
					break;
				}
			case AdvancedDelegateItem::Branch:
				{
					QStyleOptionViewItemV4 option(FOption);
					option.rect = QStyle::alignedRect(option.direction,Qt::AlignCenter,FSizeHint,option.rect);
					QStyle *style = option.widget ? option.widget->style() : QApplication::style();
					style->proxy()->drawPrimitive(QStyle::PE_IndicatorBranch, &option, APainter, FOption.widget);
					break;
				}
			case AdvancedDelegateItem::CheckBox:
				{
					QStyle *style = FOption.widget ? FOption.widget->style() : QApplication::style();
					style->proxy()->drawPrimitive(QStyle::PE_IndicatorViewItemCheck, &FOption, APainter, FOption.widget);
					break;
				}
			case AdvancedDelegateItem::CustomWidget:
				{
					break;
				}
			default:
				{
					drawVariant(APainter,FItem.c->value);
				}
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
QWidget *AdvancedDelegateEditProxy::createEditor(const AdvancedItemDelegate *ADelegate, QWidget *AParent, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex)
{
	Q_UNUSED(ADelegate); Q_UNUSED(AParent); Q_UNUSED(AOption); Q_UNUSED(AIndex);
	return NULL;
}

bool AdvancedDelegateEditProxy::setEditorData(const AdvancedItemDelegate *ADelegate, QWidget *AEditor, const QModelIndex &AIndex)
{
	Q_UNUSED(ADelegate); Q_UNUSED(AEditor); Q_UNUSED(AIndex);
	return false;
}

bool AdvancedDelegateEditProxy::setModelData(const AdvancedItemDelegate *ADelegate, QWidget *AEditor, QAbstractItemModel *AModel, const QModelIndex &AIndex)
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
	FEditRole = Qt::EditRole;
	FEditItemId = AdvancedDelegateItem::NullId;
	
	FBlinkOpacity = 1.0;
	FBlinkMode = BlinkHide;
	FBlinkTimer.setSingleShot(false);
	FBlinkTimer.setInterval(BlinkStepsTime/BlinkStepsCount);
	connect(&FBlinkTimer,SIGNAL(timeout()),SLOT(onBlinkTimerTimeout()));
	FBlinkTimer.start();
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

AdvancedItemDelegate::BlinkMode AdvancedItemDelegate::blinkMode() const
{
	return FBlinkMode;
}

void AdvancedItemDelegate::setBlinkMode(BlinkMode AMode)
{
	FBlinkMode = AMode;
}

int AdvancedItemDelegate::editRole() const
{
	return FEditRole;
}

void AdvancedItemDelegate::setEditRole(int ARole)
{
	FEditRole = ARole;
}

quint32 AdvancedItemDelegate::editItemId() const
{
	return FEditItemId;
}

void AdvancedItemDelegate::setEditItemId(quint32 AItemId)
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

#if defined(Q_OS_WIN)
	QStyle *style = indexOption.widget ? indexOption.widget->style() : QApplication::style();
	if (style && style->objectName()=="windowsvista")
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

	QStyleOptionViewItemV4 indexOption = indexStyleOption(AOption,AIndex,true);
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
			QVariant value = AIndex.data(FEditRole);
			if (FEditItemId != AdvancedDelegateItem::NullId)
			{
				QStyleOptionViewItemV4 indexOption = indexStyleOption(AOption,AIndex,true);
				AdvancedDelegateItems items = getIndexItems(AIndex,indexOption);
				value = items.value(FEditItemId).c->value;
			}
			
			QVariant::Type type = static_cast<QVariant::Type>(value.userType());
			widget = editorFactory()->createEditor(type, AParent);
			
			if (widget)
				widget->setProperty(ADVANCED_DELEGATE_EDITOR_VALUE_PROPERTY,value);
		}
	}
	return widget;
}

void AdvancedItemDelegate::setEditorData(QWidget *AEditor, const QModelIndex &AIndex) const
{
	if (FEditProxy==NULL || !FEditProxy->setEditorData(this,AEditor,AIndex))
	{
		QVariant value = AEditor->property(ADVANCED_DELEGATE_EDITOR_VALUE_PROPERTY);
		QByteArray name = editorFactory()->valuePropertyName(value.type());
		if (!name.isEmpty()) 
			AEditor->setProperty(name, value);
	}
}

void AdvancedItemDelegate::setModelData(QWidget *AEditor, QAbstractItemModel *AModel, const QModelIndex &AIndex) const
{
	if (FEditProxy==NULL || !FEditProxy->setModelData(this,AEditor,AModel,AIndex))
	{
		QVariant value = AEditor->property(ADVANCED_DELEGATE_EDITOR_VALUE_PROPERTY);
		QByteArray name = editorFactory()->valuePropertyName(value.type());
		if (!name.isEmpty())
			AModel->setData(AIndex, AEditor->property(name), FEditRole);
	}
}

void AdvancedItemDelegate::updateEditorGeometry(QWidget *AEditor, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const
{
	if (FEditProxy==NULL || !FEditProxy->updateEditorGeometry(this,AEditor,AOption,AIndex))
	{
		quint32 itemId = FEditItemId!=AdvancedDelegateItem::NullId ? FEditItemId : AdvancedDelegateItem::DisplayId;
		QRect rect = itemRect(itemId,indexStyleOption(AOption,AIndex,true),AIndex);
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
			branchItem.d->id = AdvancedDelegateItem::BranchId;
		}
	}

	if (AIndexOption.features & QStyleOptionViewItemV4::HasCheckIndicator)
	{
		AdvancedDelegateItem &checkItem = items[AdvancedDelegateItem::CheckStateId];
		if (checkItem.d->kind == AdvancedDelegateItem::Null)
		{
			checkItem.d->kind = AdvancedDelegateItem::CheckBox;
			checkItem.d->id = AdvancedDelegateItem::CheckStateId;
			checkItem.d->data = AIndex.data(Qt::CheckStateRole);
		}
	}

	if (AIndexOption.features & QStyleOptionViewItemV4::HasDecoration)
	{
		AdvancedDelegateItem &decorationItem = items[AdvancedDelegateItem::DecorationId];
		if (decorationItem.d->kind == AdvancedDelegateItem::Null)
		{
			decorationItem.d->kind = AdvancedDelegateItem::Decoration;
			decorationItem.d->id = AdvancedDelegateItem::DecorationId;
			decorationItem.d->data = AIndex.data(Qt::DecorationRole);
		}
	}

	if (AIndexOption.features & QStyleOptionViewItemV4::HasDisplay)
	{
		AdvancedDelegateItem &displayItem = items[AdvancedDelegateItem::DisplayId];
		if (displayItem.d->kind == AdvancedDelegateItem::Null)
		{
			displayItem.d->kind = AdvancedDelegateItem::Display;
			displayItem.d->id = AdvancedDelegateItem::DisplayId;
			displayItem.d->data = AIndex.data(Qt::DisplayRole);
		}
	}

	AdvancedDelegateItems stretches;
	for(AdvancedDelegateItems::iterator it=items.begin(); it!=items.end(); ++it)
	{
		quint8 position = AdvancedDelegateItem::getPosition(it->d->id);
		if (position == AdvancedDelegateItem::MiddleCenter)
		{
			quint8 floor = AdvancedDelegateItem::getFloor(it->d->id);
			quint32 stretchId = AdvancedDelegateItem::makeStretchId(position,floor);
			if (!stretches.contains(stretchId) && !items.contains(stretchId))
			{
				AdvancedDelegateItem stretchItem(stretchId);
				stretchItem.d->kind = AdvancedDelegateItem::Stretch;
				stretchItem.d->sizePolicy = QSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
				stretches.insertMulti(stretchId,stretchItem);
			}
		}

		it->c->value = it->d->data;
		if (it->d->kind==AdvancedDelegateItem::Decoration || it->d->kind==AdvancedDelegateItem::Display || it->d->kind==AdvancedDelegateItem::CustomData)
			if (it->d->data.type() == QVariant::Int)
				it->c->value = AIndex.data(it->d->data.toInt());
		
		it->c->blinkOpacity = (it->d->flags & AdvancedDelegateItem::Blink)>0 ? FBlinkOpacity : 1.0;
	}
	items.unite(stretches);
	
	return items;
}

QStyleOptionViewItemV4 AdvancedItemDelegate::indexStyleOption(const QStyleOptionViewItem &AOption, const QModelIndex &AIndex, bool ACorrect) const
{
	QStyleOptionViewItemV4 indexOption = AOption;

	if (ACorrect)
	{
		// Some states are not set for sizeHint and createEditor
		if (AIndex.model()->hasChildren(AIndex))
			indexOption.state |= QStyle::State_Children;
	}

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

	if (AItem.d->kind == AdvancedDelegateItem::CheckBox)
	{
		itemOption.state &= ~QStyle::State_HasFocus;
		itemOption.features |= QStyleOptionViewItemV2::HasCheckIndicator;
		itemOption.checkState = static_cast<Qt::CheckState>(AItem.c->value.toInt());

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

	if (!AItem.c->value.isNull() && AItem.c->value.canConvert<QString>())
		itemOption.text = getSingleLineText(displayText(AItem.c->value,itemOption.locale));

	if (!AItem.d->hints.isEmpty())
	{
		QVariant hint = AItem.d->hints.value(AdvancedDelegateItem::FontWeight);
		if (!hint.isNull())
			itemOption.font.setWeight(hint.toInt());

		hint = AItem.d->hints.value(AdvancedDelegateItem::FontHint);
		if (!hint.isNull())
			itemOption.font.setStyleHint((QFont::StyleHint)hint.toInt());

		hint = AItem.d->hints.value(AdvancedDelegateItem::FontStyle);
		if (!hint.isNull())
			itemOption.font.setStyle((QFont::Style)hint.toInt());

		hint = AItem.d->hints.value(AdvancedDelegateItem::FontSize);
		if (!hint.isNull())
			itemOption.font.setPointSize(hint.toInt());

		hint = AItem.d->hints.value(AdvancedDelegateItem::FontSizeDelta);
		if (!hint.isNull())
			itemOption.font.setPointSize(itemOption.font.pointSize()+hint.toInt());

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
	static const Qt::Alignment layoutAlign = Qt::AlignLeft|Qt::AlignVCenter;

	QStyle *style = AIndexOption.widget ? AIndexOption.widget->style() : QApplication::style();

	const int vSpacing = FVerticalSpacing<0 ? style->proxy()->pixelMetric(QStyle::PM_FocusFrameVMargin)+1 : FVerticalSpacing;
	const int hSpacing = FHorizontalSpacing<0 ? style->proxy()->pixelMetric(QStyle::PM_FocusFrameHMargin)+1 : FHorizontalSpacing;

	ItemsLayout *layout = new ItemsLayout;
	layout->middleLayout = NULL;
	layout->indexOption = AIndexOption;

	layout->mainLayout = new QVBoxLayout;
	layout->mainLayout->setSpacing(vSpacing);
	layout->mainLayout->setAlignment(layoutAlign);

	QMap<int, QMap<int, QMultiMap<int, AdvancedDelegateLayoutItem *> > > orderedItems;
	for (AdvancedDelegateItems::const_iterator it = AItems.constBegin(); it!=AItems.constEnd(); ++it)
	{
		QStyleOptionViewItemV4 itemOption = itemStyleOption(it.value(),AIndexOption);
		if (isItemVisible(it.value(),itemOption))
		{
			quint8 position = AdvancedDelegateItem::getPosition(it->d->id);
			quint8 floor = AdvancedDelegateItem::getFloor(it->d->id);
			quint16 order = AdvancedDelegateItem::getOrder(it->d->id);

			QMap<int, QBoxLayout *> &posLayouts = layout->positionLayouts;
			QBoxLayout *&posLayout = posLayouts[position];
			if (posLayout == NULL)
			{
				posLayout = new QVBoxLayout;
				posLayout->setSpacing(vSpacing);
				posLayout->setAlignment(layoutAlign);
			}

			QMap<int, QBoxLayout *> &floorLayouts = layout->floorLayouts[position];
			QBoxLayout *&floorLayout = floorLayouts[floor];
			if (floorLayout == NULL)
			{
				floorLayout = new QHBoxLayout;
				floorLayout->setSpacing(hSpacing);
				floorLayout->setAlignment(layoutAlign);
			}

			AdvancedDelegateLayoutItem *item = new AdvancedDelegateLayoutItem(it.value(),itemOption);
			layout->items.insert(it.key(),item);
			orderedItems[position][floor].insertMulti(order,item);
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
				layout->middleLayout->setAlignment(layoutAlign);
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
				floorLayout->addItem(order_it.value());
		}
	}

	return layout;
}

void AdvancedItemDelegate::destroyItemsLayout(ItemsLayout *ALayout) const
{
	destroyLayoutRecursive(ALayout->mainLayout);
	delete ALayout;
}

QRect AdvancedItemDelegate::itemRect(quint32 AItemId, const ItemsLayout *ALayout, const QRect &AGeometry) const
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

QRect AdvancedItemDelegate::itemRect(quint32 AItemId, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const
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

quint32 AdvancedItemDelegate::itemAt(const QPoint &APoint, const ItemsLayout *ALayout, const QRect &AGeometry) const
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

quint32 AdvancedItemDelegate::itemAt(const QPoint &APoint, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const
{
	quint32 itemId = AdvancedDelegateItem::NullId;
	if (AIndex.isValid() && !AOption.rect.isEmpty())
	{
		QStyleOptionViewItemV4 indexOption = indexStyleOption(AOption,AIndex);
		ItemsLayout *layout = createItemsLayout(getIndexItems(AIndex,indexOption),indexOption);
		itemId = itemAt(APoint,layout,indexOption.rect);
		destroyItemsLayout(layout);
	}
	return itemId;
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
		return !AItem.c->value.isNull();
	}
}

QSize AdvancedItemDelegate::itemSizeHint(const AdvancedDelegateItem &AItem, const QStyleOptionViewItemV4 &AItemOption)
{
	static const QSize zeroSize = QSize(0,0);
	static const QSize branchSize = QSize(12,12);

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
			QStyle *style = AItemOption.widget!=NULL ? AItemOption.widget->style() : QApplication::style();
			int width = style->proxy()->pixelMetric(QStyle::PM_IndicatorWidth, &AItemOption, AItemOption.widget);
			int height = style->proxy()->pixelMetric(QStyle::PM_IndicatorHeight, &AItemOption, AItemOption.widget);
			return QSize(width,height);
		}
	case AdvancedDelegateItem::CustomWidget:
		{
			return AItem.d->widget!=NULL ? AItem.d->widget->sizeHint() : zeroSize;
		}
	default:
		{
			switch (AItem.c->value.type())
			{
			case QVariant::Pixmap:
				{
					QPixmap pixmap = qvariant_cast<QPixmap>(AItem.c->value);
					if (!pixmap.isNull())
						return pixmap.size();
					break;
				}
			case QVariant::Image:
				{
					QImage image = qvariant_cast<QImage>(AItem.c->value);
					if (!image.isNull())
						return image.size();
					break;
				}
			case QVariant::Icon:
				{
					QIcon icon = qvariant_cast<QIcon>(AItem.c->value);
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

void AdvancedItemDelegate::onBlinkTimerTimeout()
{
	qreal blinkOpacity = FBlinkMode==BlinkHide ? BlinkHideSteps[BLINK_STEP] : BlinkFadeSteps[BLINK_STEP];
	if (FBlinkOpacity != blinkOpacity)
	{
		FBlinkOpacity = blinkOpacity;
		emit updateBlinkItems();
	}
}
