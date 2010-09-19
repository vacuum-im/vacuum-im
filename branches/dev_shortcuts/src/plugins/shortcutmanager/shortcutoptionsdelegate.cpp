#include "shortcutoptionsdelegate.h"

#include <QKeyEvent>
#include <QLineEdit>

ShortcutOptionsDelegate::ShortcutOptionsDelegate(QObject *AParent) : QStyledItemDelegate(AParent)
{
	FFilter = new QObject(this);
	FFilter->installEventFilter(this);
}

ShortcutOptionsDelegate::~ShortcutOptionsDelegate()
{

}

QWidget *ShortcutOptionsDelegate::createEditor(QWidget *AParent, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const
{
	Q_UNUSED(AOption);
	if (AIndex.data(MDR_SHORTCUTID).isValid())
	{
		QLineEdit *editor = new QLineEdit(AParent);
		editor->installEventFilter(FFilter);
		return editor;
	}
	return NULL;
}

void ShortcutOptionsDelegate::setEditorData(QWidget *AEditor, const QModelIndex &AIndex) const
{
	QLineEdit *editor = qobject_cast<QLineEdit *>(AEditor);
	if (editor)
	{
		QKeySequence key = qvariant_cast<QKeySequence>(AIndex.data(MDR_ACTIVE_KEYSEQUENCE));
		editor->setText(key.toString(QKeySequence::NativeText));
	}
}

void ShortcutOptionsDelegate::setModelData(QWidget *AEditor, QAbstractItemModel *AModel, const QModelIndex &AIndex) const
{
	QLineEdit *editor = qobject_cast<QLineEdit *>(AEditor);
	if (editor)
	{
		QKeySequence key = editor->text().isEmpty() ? qvariant_cast<QKeySequence>(AIndex.data(MDR_DEFAULT_KEYSEQUENCE)) : QKeySequence(editor->text());
		AModel->setData(AIndex,key.toString(QKeySequence::NativeText),Qt::DisplayRole);
		AModel->setData(AIndex,key,MDR_ACTIVE_KEYSEQUENCE);
	}
}

void ShortcutOptionsDelegate::updateEditorGeometry(QWidget *AEditor, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const
{
	QStyledItemDelegate::updateEditorGeometry(AEditor,AOption,AIndex);
}

bool ShortcutOptionsDelegate::eventFilter(QObject *AWatched, QEvent *AEvent)
{
	QLineEdit *editor = qobject_cast<QLineEdit *>(AWatched);
	if (editor && AEvent->type()==QEvent::KeyPress)
	{
		QKeyEvent *keyEvent = static_cast<QKeyEvent *>(AEvent);
		if (keyEvent->key()==Qt::Key_Space && keyEvent->modifiers()==0)
			editor->setText(QString::null);
		else
			editor->setText(QKeySequence(keyEvent->modifiers() | keyEvent->key()).toString(QKeySequence::NativeText));
		return true;
	}
	else if (editor && AEvent->type()==QEvent::KeyRelease)
	{
		emit commitData(editor);
		emit closeEditor(editor);
		return true;
	}
	return QStyledItemDelegate::eventFilter(AWatched,AEvent);
}
