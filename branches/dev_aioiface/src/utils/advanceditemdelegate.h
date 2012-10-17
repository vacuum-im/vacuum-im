#ifndef ADVANCEDITEMDELEGATE_H
#define ADVANCEDITEMDELEGATE_H

#include <QMap>
#include <QTimer>
#include <QWidget>
#include <QMargins>
#include <QVariant>
#include <QSizePolicy>
#include <QStyleOption>
#include <QStyledItemDelegate>
#include "utilsexport.h"

#define ADVANCED_DELEGATE_EDITOR_VALUE_PROPERY "AdvanceDelegateEditorValue"

struct AdvancedDelegateItem
{
	enum Id {
		NullId,
		BranchId,
		CheckStateId,
		DecorationId,
		DisplayId,
		DisplayStretchId,
		UserId = 16
	};
	enum Kind {
		Null,
		Branch,
		CheckBox,
		Decoration,
		Display,
		Stretch,
		CustomData,
		CustomWidget
	};
	enum Position {
		Top,
		MiddleLeft,
		MiddleCenter,
		MiddleRight,
		Bottom
	};
	enum Hint {
		SizeHint,
		FontSize,
		FontWeight,
		FontHint,
		FontStyle,
		FontUnderline,
		StatesForceOn,
		StatesForceOff,
		Foreground,
		Backgound,
		TextElideMode,
		TextAlignment,
		Opacity
	};
	enum Flag {
		BlinkHide          =0x00000001,
		BlinkFade          =0x00000002
	};
	struct AdvancedDelegateItemData {
		int refs;
		int kind;
		int order;
		int floor;
		int position;
		quint32 flags;
		QVariant data;
		QWidget *widget;
		QSizePolicy sizePolicy;
		QStyle::State showStates;
		QStyle::State hideStates;
		QMap<int,QVariant> hints;
	};

	AdvancedDelegateItem(int ADefaultsId = NullId);
	AdvancedDelegateItem(const AdvancedDelegateItem &AOther);
	~AdvancedDelegateItem();
	AdvancedDelegateItem &operator =(const AdvancedDelegateItem &AOther); 

	AdvancedDelegateItemData *d;
};
typedef QMap<int, AdvancedDelegateItem> AdvancedDelegateItems;

static const struct {int id; int position; int floor; int order;} AdvancedDelegateItemDefaults[] =
{
	{ AdvancedDelegateItem::NullId,           AdvancedDelegateItem::MiddleCenter, 500, 0     },
	{ AdvancedDelegateItem::BranchId,         AdvancedDelegateItem::MiddleLeft,   500, 10    },
	{ AdvancedDelegateItem::CheckStateId,     AdvancedDelegateItem::MiddleLeft,   500, 100   },
	{ AdvancedDelegateItem::DecorationId,     AdvancedDelegateItem::MiddleLeft,   500, 500   },
	{ AdvancedDelegateItem::DisplayId,        AdvancedDelegateItem::MiddleCenter, 500, 500   },
	{ AdvancedDelegateItem::DisplayStretchId, AdvancedDelegateItem::MiddleCenter, 500, 10000 },
};

class AdvancedItemDelegate;
class AdvancedDelegateEditProxy
{
public:
	virtual QWidget *createEditor(const AdvancedItemDelegate *ADelegate, QWidget *AParent, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
	virtual bool setEditorData(const AdvancedItemDelegate *ADelegate, QWidget *AEditor, const QModelIndex &AIndex) const;
	virtual bool setModelData(const AdvancedItemDelegate *ADelegate, QWidget *AEditor, QAbstractItemModel *AModel, const QModelIndex &AIndex) const;
	virtual bool updateEditorGeometry(const AdvancedItemDelegate *ADelegate, QWidget *AEditor, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
};

class UTILS_EXPORT AdvancedItemDelegate : 
	public QStyledItemDelegate
{
	Q_OBJECT;
	struct ItemsLayout;
public:
	AdvancedItemDelegate(QObject *AParent);
	~AdvancedItemDelegate();
	int itemsRole() const;
	void setItemsRole(int ARole);
	int verticalSpacing() const;
	void setVertialSpacing(int ASpacing);
	int horizontalSpacing() const;
	void setHorizontalSpacing(int ASpacing);
	bool focusRectVisible() const;
	void setFocusRectVisible(bool AVisible);
	bool defaultBranchItemEnabled() const;
	void setDefaultBranchItemEnabled(bool AEnabled);
	QMargins contentsMargins() const;
	void setContentsMargings(const QMargins &AMargins);
public:
	int editItemId() const;
	void setEditItemId(int AItemId);
	AdvancedDelegateEditProxy *editProxy() const;
	void setEditProxy(AdvancedDelegateEditProxy *AProxy);
	const QItemEditorFactory *editorFactory() const;
public:
	void paint(QPainter *APainter, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
	QSize sizeHint(const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
	QWidget *createEditor(QWidget *AParent, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
	void setEditorData(QWidget *AEditor, const QModelIndex &AIndex) const;
	void setModelData(QWidget *AEditor, QAbstractItemModel *AModel, const QModelIndex &AIndex) const;
	void updateEditorGeometry(QWidget *AEditor, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
public:
	AdvancedDelegateItems getIndexItems(const QModelIndex &AIndex, const QStyleOptionViewItemV4 &AIndexOption) const;
	QStyleOptionViewItemV4 indexStyleOption(const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
	QStyleOptionViewItemV4 itemStyleOption(const AdvancedDelegateItem &AItem, const QStyleOptionViewItemV4 &AIndexOption) const;
	ItemsLayout *createItemsLayout(const AdvancedDelegateItems &AItems, const QStyleOptionViewItemV4 &AIndexOption) const;
	void destroyItemsLayout(ItemsLayout *ALayout) const;
public:
	QRect itemRect(int AItemId, const ItemsLayout *ALayout, const QRect &AGeometry) const;
	QRect itemRect(int AItemId, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
	int itemAt(const QPoint &APoint, const ItemsLayout *ALayout, const QRect &AGeometry) const;
	int itemAt(const QPoint &APoint, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
public:
	static QSize itemSizeHint(const AdvancedDelegateItem &AItem, const QStyleOptionViewItemV4 &AItemOption);
	static bool isItemVisible(const AdvancedDelegateItem &AItem, const QStyleOptionViewItemV4 &AItemOption);
signals:
	void updateBlinkItems();
protected:
	void drawBackground(QPainter *APainter, const QStyleOptionViewItemV4 &AIndexOption) const;
	void drawFocusRect(QPainter *APainter, const QStyleOptionViewItemV4 &AIndexOption, const QRect &ARect) const;
protected:
	bool editorEvent(QEvent *AEvent, QAbstractItemModel *AModel, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex);
private:
	int FItemsRole;
	int FVerticalSpacing;
	int FHorizontalSpacing;
	bool FFocusRectVisible;
	bool FDefaultBranchEnabled;
	QMargins FMargins;
	QTimer FBlinkTimer;
private:
	int FEditItemId;
	AdvancedDelegateEditProxy *FEditProxy;
};

Q_DECLARE_METATYPE(AdvancedDelegateItems);

#endif // ADVANCEDITEMDELEGATE_H
