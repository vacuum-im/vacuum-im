#ifndef SHORTCUTOPTIONSDELEGATE_H
#define SHORTCUTOPTIONSDELEGATE_H

#include <QStyledItemDelegate>

enum ShortcutColumns {
	SCL_NAME,
	SCL_KEY
};

enum ShortcutDataRoles {
	SDR_SHORTCUTID              = Qt::UserRole,
	SDR_ACTIVE_KEYSEQUENCE,
	SDR_DEFAULT_KEYSEQUENCE,
	SDR_SORTROLE
};

class ShortcutOptionsDelegate : 
	public QStyledItemDelegate
{
	Q_OBJECT;
public:
	ShortcutOptionsDelegate(QObject *AParent);
	virtual QWidget *createEditor(QWidget *AParent, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
	virtual void setEditorData(QWidget *AEditor, const QModelIndex &AIndex) const;
	virtual void setModelData(QWidget *AEditor, QAbstractItemModel *AModel, const QModelIndex &AIndex) const;
	virtual void updateEditorGeometry(QWidget *AEditor, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
	virtual QSize sizeHint(const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
protected:
	virtual bool eventFilter(QObject *AWatched, QEvent *AEvent);
private:
	int FMinHeight;
	QObject *FFilter;
};

#endif // SHORTCUTOPTIONSDELEGATE_H
