#ifndef SHORTCUTOPTIONSDELEGATE_H
#define SHORTCUTOPTIONSDELEGATE_H

#include <QStyledItemDelegate>

#define COL_NAME  0
#define COL_KEY   1

#define MDR_SHORTCUTID      Qt::UserRole+1
#define MDR_KEYSEQUENCE     Qt::UserRole+2

class ShortcutOptionsDelegate : 
	public QStyledItemDelegate
{
	Q_OBJECT;
public:
	ShortcutOptionsDelegate(QObject *AParent);
	~ShortcutOptionsDelegate();
	virtual QWidget *createEditor(QWidget *AParent, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
	virtual void setEditorData(QWidget *AEditor, const QModelIndex &AIndex) const;
	virtual void setModelData(QWidget *AEditor, QAbstractItemModel *AModel, const QModelIndex &AIndex) const;
	virtual void updateEditorGeometry(QWidget *AEditor, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
protected:
	virtual bool eventFilter(QObject *AWatched, QEvent *AEvent);
private:
	QObject *FFilter;
};

#endif // SHORTCUTOPTIONSDELEGATE_H
