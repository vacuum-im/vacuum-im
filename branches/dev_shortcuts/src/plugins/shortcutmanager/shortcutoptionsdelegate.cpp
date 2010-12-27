#include "shortcutoptionsdelegate.h"

#include <QKeyEvent>
#include <QLineEdit>

ShortcutOptionsDelegate::ShortcutOptionsDelegate(QObject *AParent) : QStyledItemDelegate(AParent)
{
	FFilter = new QObject(this);
	FFilter->installEventFilter(this);

	QLineEdit *editor = new QLineEdit(NULL);
	FMinHeight = editor->sizeHint().height();
	delete editor;
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

QSize ShortcutOptionsDelegate::sizeHint(const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const
{
	QSize hint = QStyledItemDelegate::sizeHint(AOption,AIndex);
	hint.setHeight(qMax(hint.height(),FMinHeight));
	return hint;
}

bool ShortcutOptionsDelegate::eventFilter(QObject *AWatched, QEvent *AEvent)
{
	QLineEdit *editor = qobject_cast<QLineEdit *>(AWatched);
	if (editor && AEvent->type()==QEvent::KeyPress)
	{
		static const QList<int> controlKeys =  QList<int>() << Qt::Key_unknown <<  Qt::Key_Shift << Qt::Key_Control << Qt::Key_Meta << Qt::Key_Alt << Qt::Key_AltGr;
		QKeyEvent *keyEvent = static_cast<QKeyEvent *>(AEvent);
		if (!controlKeys.contains(keyEvent->key()))
			editor->setText(QKeySequence((keyEvent->modifiers() & ~Qt::KeypadModifier) | keyEvent->key()).toString(QKeySequence::NativeText));
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
