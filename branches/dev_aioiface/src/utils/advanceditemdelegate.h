#ifndef ADVANCEDITEMDELEGATE_H
#define ADVANCEDITEMDELEGATE_H

#include <QMap>
#include <QWidget>
#include <QMargins>
#include <QVariant>
#include <QSizePolicy>
#include <QLayoutItem>
#include <QStyleOption>
#include <QSharedDataPointer>
#include <QStyledItemDelegate>
#include "utilsexport.h"

struct AdvancedDelegateItem
{
	enum Id {
		NullId,
		BranchId,
		CheckStateId,
		DecorationId,
		DisplayId,
		Stretch0Id,
		UserId = 16
	};
	enum Kind {
		Null,
		Branch,
		CheckState,
		Decoration,
		Display,
		Stretch,
		CustomData,
		CustomWidget
	};
	enum Position {
		Left,
		Middle,
		Right
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
		Blink              =0x00000001
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
		Qt::Alignment alignment;
		QStyle::State showStates;
		QStyle::State hideStates;
		QMap<int,QVariant> hints;
	};

	AdvancedDelegateItem();
	AdvancedDelegateItem(const AdvancedDelegateItem &AOther);
	~AdvancedDelegateItem();
	AdvancedDelegateItem &operator =(const AdvancedDelegateItem &AOther); 
	bool operator <(const AdvancedDelegateItem &AItem) const;

	AdvancedDelegateItemData *d;
};

typedef QMap<int, AdvancedDelegateItem> AdvancedDelegateItems;

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
	QMargins contentsMargins() const;
	void setContentsMargings(const QMargins &AMargins);
	bool defaultBranchItemEnabled() const;
	void setDefaultBranchItemEnabled(bool AEnabled);
	//QAbstractItemDelegate
	virtual void paint(QPainter *APainter, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
	virtual QSize sizeHint(const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
public:
	AdvancedDelegateItems getIndexItems(const QModelIndex &AIndex) const;
	QStyleOptionViewItemV4 indexStyleOption(const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
	ItemsLayout *createItemsLayout(const AdvancedDelegateItems &AItems, const QStyleOptionViewItemV4 &AIndexOption) const;
	void destroyItemsLayout(ItemsLayout *ALayout) const;
public:
	static QStyleOptionViewItemV4 itemStyleOption(const AdvancedDelegateItem &AItem, const QStyleOptionViewItemV4 &AIndexOption);
	static QSize itemSizeHint(const AdvancedDelegateItem &AItem, const QStyleOptionViewItemV4 &AItemOption);
	static bool isItemVisible(const AdvancedDelegateItem &AItem, const QStyleOptionViewItemV4 &AItemOption);
protected:
	void drawBackground(QPainter *APainter, const QStyleOptionViewItemV4 &AIndexOption) const;
	void drawFocusRect(QPainter *APainter, const QStyleOptionViewItemV4 &AIndexOption, const QRect &ARect) const;
private:
	int FItemsRole;
	int FVerticalSpacing;
	int FHorizontalSpacing;
	bool FDefaultBranchEnabled;
	QMargins FMargins;
};

Q_DECLARE_METATYPE(AdvancedDelegateItems);

#endif // ADVANCEDITEMDELEGATE_H
