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

#define ADVANCED_DELEGATE_EDITOR_VALUE_PROPERTY "AdvanceDelegateEditorValue"

struct UTILS_EXPORT AdvancedDelegateItem
{
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
		FontHint,
		FontStyle,
		FontWeight,
		FontSize,
		FontSizeDelta,
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
		Hidden         =0x00000001,
		Blink          =0x00000002
	};
	struct ExplicitData {
		int refs;
		quint32 id;
		quint8 kind;
		quint32 flags;
		QVariant data;
		QWidget *widget;
		QSizePolicy sizePolicy;
		QStyle::State showStates;
		QStyle::State hideStates;
		QMap<int,QVariant> hints;
		QMap<int,QVariant> properties;
	};
	struct ContextData {
		QVariant value;
		qreal blinkOpacity;
	};

	AdvancedDelegateItem(quint32 AId = NullId);
	AdvancedDelegateItem(const AdvancedDelegateItem &AOther);
	~AdvancedDelegateItem();

	void detach();
	AdvancedDelegateItem &operator =(const AdvancedDelegateItem &AOther);
	
	static const quint16 AlignRightOrderMask = 0x8000;
	static quint32 makeId(quint8 APosition, quint8 AFloor, quint16 AOrder);
	static quint32 makeStretchId(quint8 APosition, quint8 AFloor);
	static quint8 getPosition(quint32 AItemId);
	static quint8 getFloor(quint32 AItemId);
	static quint16 getOrder(quint32 AItemId);

	static const quint32 NullId;        // =0
	static const quint32 BranchId;      // =makeId(MiddleLeft,128,10)
	static const quint32 CheckStateId;  // =makeId(MiddleLeft,128,100)
	static const quint32 DecorationId;  // =makeId(MiddleLeft,128,500)
	static const quint32 DisplayId;     // =makeId(MiddleCenter,128,500)

	ContextData *c;
	ExplicitData *d;
};
typedef QMap<quint32, AdvancedDelegateItem> AdvancedDelegateItems;

class AdvancedItemDelegate;
class UTILS_EXPORT AdvancedDelegateEditProxy
{
public:
	virtual QWidget *createEditor(const AdvancedItemDelegate *ADelegate, QWidget *AParent, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex);
	virtual bool setEditorData(const AdvancedItemDelegate *ADelegate, QWidget *AEditor, const QModelIndex &AIndex);
	virtual bool setModelData(const AdvancedItemDelegate *ADelegate, QWidget *AEditor, QAbstractItemModel *AModel, const QModelIndex &AIndex);
	virtual bool updateEditorGeometry(const AdvancedItemDelegate *ADelegate, QWidget *AEditor, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
};

class UTILS_EXPORT AdvancedItemDelegate : 
	public QStyledItemDelegate
{
	Q_OBJECT;
	struct ItemsLayout;
public:
	enum BlinkMode {
		BlinkHide,
		BlinkFade
	};
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
	BlinkMode blinkMode() const;
	void setBlinkMode(BlinkMode AMode);
public:
	int editRole() const;
	void setEditRole(int ARole);
	quint32 editItemId() const;
	void setEditItemId(quint32 AItemId);
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
	QStyleOptionViewItemV4 indexStyleOption(const QStyleOptionViewItem &AOption, const QModelIndex &AIndex, bool ACorrect=false) const;
	QStyleOptionViewItemV4 itemStyleOption(const AdvancedDelegateItem &AItem, const QStyleOptionViewItemV4 &AIndexOption) const;
	ItemsLayout *createItemsLayout(const AdvancedDelegateItems &AItems, const QStyleOptionViewItemV4 &AIndexOption) const;
	void destroyItemsLayout(ItemsLayout *ALayout) const;
public:
	QRect itemRect(quint32 AItemId, const ItemsLayout *ALayout, const QRect &AGeometry) const;
	QRect itemRect(quint32 AItemId, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
	quint32 itemAt(const QPoint &APoint, const ItemsLayout *ALayout, const QRect &AGeometry) const;
	quint32 itemAt(const QPoint &APoint, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
signals:
	void updateBlinkItems();
public:
	static bool isItemVisible(const AdvancedDelegateItem &AItem, const QStyleOptionViewItemV4 &AItemOption);
	static QSize itemSizeHint(const AdvancedDelegateItem &AItem, const QStyleOptionViewItemV4 &AItemOption);
protected:
	void drawBackground(QPainter *APainter, const QStyleOptionViewItemV4 &AIndexOption) const;
	void drawFocusRect(QPainter *APainter, const QStyleOptionViewItemV4 &AIndexOption, const QRect &ARect) const;
protected:
	bool editorEvent(QEvent *AEvent, QAbstractItemModel *AModel, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex);
protected slots:
	void onBlinkTimerTimeout();
private:
	int FItemsRole;
	int FVerticalSpacing;
	int FHorizontalSpacing;
	bool FFocusRectVisible;
	bool FDefaultBranchEnabled;
	QMargins FMargins;
private:
	qreal FBlinkOpacity;
	BlinkMode FBlinkMode;
	QTimer FBlinkTimer;
private:
	int FEditRole;
	quint32 FEditItemId;
	AdvancedDelegateEditProxy *FEditProxy;
};

UTILS_EXPORT QDataStream &operator>>(QDataStream &AStream, AdvancedDelegateItem &AItem);
UTILS_EXPORT QDataStream &operator<<(QDataStream &AStream, const AdvancedDelegateItem &AItem);

Q_DECLARE_METATYPE(AdvancedDelegateItem);
Q_DECLARE_METATYPE(AdvancedDelegateItems);

#endif // ADVANCEDITEMDELEGATE_H
