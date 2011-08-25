#ifndef ROSTERINDEXDELEGATE_H
#define ROSTERINDEXDELEGATE_H

#include <QStyle>
#include <QStyledItemDelegate>
#include <definitions/rosterlabelorders.h>
#include <definitions/rosterindextyperole.h>
#include <interfaces/irostersview.h>

typedef QMap<int, IRostersLabel> RostersLabelItems;
Q_DECLARE_METATYPE(RostersLabelItems);

struct LabelItem
{
	int id;
	int order;
	int flags;
	QSize size;
	QRect rect;
	QVariant value;
	bool operator <(const LabelItem &AItem) const {
		return order < AItem.order;
	}
};

class RosterIndexDelegate :
			public QStyledItemDelegate
{
	Q_OBJECT;
public:
	RosterIndexDelegate(QObject *AParent);
	~RosterIndexDelegate();
	//QAbstractItemDelegate
	virtual void paint(QPainter *APainter, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
	virtual QSize sizeHint(const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
	virtual QWidget *createEditor(QWidget *AParent, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
	virtual void setEditorData(QWidget *AEditor, const QModelIndex &AIndex) const;
	virtual void setModelData(QWidget *AEditor, QAbstractItemModel *AModel, const QModelIndex &AIndex) const;
	virtual void updateEditorGeometry(QWidget *AEditor, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
	//RosterIndexDelegate
	void setShowBlinkLabels(bool AShow);
	IRostersEditHandler *editHandler() const;
	void setEditHandler(int ADataRole, IRostersEditHandler *AHandler);
	int labelAt(const QPoint &APoint, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
	QRect labelRect(int ALabelId, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
protected:
	QHash<int,QRect> drawIndex(QPainter *APainter, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
	void drawLabelItem(QPainter *APainter, const QStyleOptionViewItemV4 &AOption, const LabelItem &ALabel) const;
	void drawBackground(QPainter *APainter, const QStyleOptionViewItemV4 &AOption) const;
	void drawFocus(QPainter *APainter, const QStyleOptionViewItemV4 &AOption, const QRect &ARect) const;
	QStyleOptionViewItemV4 indexOptions(const QModelIndex &AIndex, const QStyleOptionViewItem &AOption) const;
	QStyleOptionViewItemV4 indexFooterOptions(const QStyleOptionViewItemV4 &AOption) const;
	QList<LabelItem> itemLabels(const QModelIndex &AIndex) const;
	QList<LabelItem> itemFooters(const QModelIndex &AIndex) const;
	QSize variantSize(const QStyleOptionViewItemV4 &AOption, const QVariant &AValue) const;
	void getLabelsSize(const QStyleOptionViewItemV4 &AOption, QList<LabelItem> &ALabels) const;
	void removeWidth(QRect &ARect, int AWidth, bool AIsLeftToRight) const;
	QString prepareText(const QString &AText) const;
private:
	QIcon::Mode getIconMode(QStyle::State AState) const;
	QIcon::State getIconState(QStyle::State AState) const;
private:
	bool FShowBlinkLabels;
	int FEditRole;
	IRostersEditHandler *FEditHandler;
private:
	static const int spacing = 2;
};

#endif // ROSTERINDEXDELEGATE_H
