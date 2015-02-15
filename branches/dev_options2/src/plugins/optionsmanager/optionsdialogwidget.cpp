#include "optionsdialogwidget.h"

#include <QKeyEvent>
#include <QKeySequence>
#include <utils/options.h>
#include <utils/logger.h>

OptionsDialogWidget::OptionsDialogWidget(const OptionsNode &ANode, const QString &ACaption, QWidget *AParent) : QWidget(AParent)
{
	QVariant::Type valueType = ANode.value().type();
	if (valueType == QVariant::Bool)
	{
		QCheckBox *editor = new QCheckBox(ACaption,this);
		rigisterEditor(ANode,ACaption,editor);
	}
	else if (valueType==QVariant::Time || valueType==QVariant::Date || valueType==QVariant::DateTime)
	{
		QDateTimeEdit *editor;
		if (valueType == QVariant::Time)
			editor = new QTimeEdit(this);
		else if (valueType == QVariant::Date)
			editor = new QDateEdit(this);
		else
			editor = new QDateTimeEdit(this);
		rigisterEditor(ANode,ACaption,editor);
	}
	else if (valueType == QVariant::Color)
	{
		QComboBox *editor = new QComboBox(this);
		foreach(const QString &color, QColor::colorNames())
		{
			editor->addItem(color,QColor(color));
			editor->setItemData(editor->count()-1,QColor(color),Qt::DecorationRole);
		}
		rigisterEditor(ANode,ACaption,editor);
	}
	else if (valueType == QVariant::Font)
	{
		QFontComboBox *editor = new QFontComboBox(this);
		rigisterEditor(ANode,ACaption,editor);
	}
	else if (valueType==QVariant::Int || valueType==QVariant::LongLong)
	{
		QSpinBox *editor = new QSpinBox(this);
		rigisterEditor(ANode,ACaption,editor);
	}
	else if (valueType==QVariant::UInt || valueType==QVariant::ULongLong)
	{
		QSpinBox *editor = new QSpinBox(this);
		editor->setMinimum(0);
		rigisterEditor(ANode,ACaption,editor);
	}
	else if (valueType == QVariant::Double)
	{
		QDoubleSpinBox *editor = new QDoubleSpinBox(this);
		rigisterEditor(ANode,ACaption,editor);
	}
	else if (valueType == QVariant::ByteArray)
	{
		QLineEdit *editor = new QLineEdit(this);
		editor->setEchoMode(QLineEdit::Password);
		rigisterEditor(ANode,ACaption,editor);
	}
	else if (valueType==QVariant::String || valueType==QVariant::KeySequence)
	{
		QLineEdit *editor = new QLineEdit(this);
		rigisterEditor(ANode,ACaption,editor);
	}
	else
	{
		REPORT_ERROR(QString("Unsupported options widget node value type=%1").arg(valueType));
	}
}

OptionsDialogWidget::OptionsDialogWidget(const OptionsNode &ANode, const QString &ACaption, QWidget *AEditor, QWidget *AParent) : QWidget(AParent)
{
	rigisterEditor(ANode,ACaption,AEditor);
}

OptionsDialogWidget::~OptionsDialogWidget()
{

}

void OptionsDialogWidget::apply()
{
	if (FCheckBox != NULL)
	{
		FValue = FCheckBox->isChecked();
	}
	else if (FLineEdit != NULL)
	{
		if (FLineEdit->echoMode() == QLineEdit::Password)
			FValue = Options::encrypt(FLineEdit->text());
		else
			FValue = FLineEdit->text();
	}
	else if (FFontComboBox != NULL)
	{
		FValue = FFontComboBox->currentFont();
	}
	else if (FComboBox != NULL)
	{
		if (FComboBox->currentIndex() >= 0)
			FValue = FComboBox->itemData(FComboBox->currentIndex());
	}
	else if (FTimeEdit != NULL)
	{
		FValue = FTimeEdit->time();
	}
	else if (FDateEdit != NULL)
	{
		FValue = FDateEdit->date();
	}
	else if (FDateTimeEdit != NULL)
	{
		FValue = FDateTimeEdit->dateTime();
	}
	else if (FDoubleSpinBox != NULL)
	{
		FValue = FDoubleSpinBox->value();
	}
	else if (FSpinBox)
	{
		if (FValue.type() == QVariant::UInt)
			FValue = (uint)FSpinBox->value();
		else if (FValue.type() == QVariant::LongLong)
			FValue = (qlonglong)FSpinBox->value();
		else if (FValue.type() == QVariant::ULongLong)
			FValue = (qulonglong)FSpinBox->value();
		else
			FValue = FSpinBox->value();
	}

	FNode.setValue(FValue);
	emit childApply();
}

void OptionsDialogWidget::reset()
{
	if (FCheckBox != NULL)
	{
		FCheckBox->setChecked(FValue.toBool());
	}
	else if (FLineEdit != NULL)
	{
		if (FLineEdit->echoMode() == QLineEdit::Password)
			FLineEdit->setText(Options::decrypt(FValue.toByteArray()).toString());
		else
			FLineEdit->setText(FValue.toString());
	}
	else if (FFontComboBox != NULL)
	{
		FFontComboBox->setCurrentFont(FValue.value<QFont>());
	}
	else if (FComboBox != NULL)
	{
		FComboBox->setCurrentIndex(FComboBox->findData(FValue));
	}
	else if (FTimeEdit != NULL)
	{
		FTimeEdit->setTime(FValue.toTime());
	}
	else if (FDateEdit != NULL)
	{
		FDateEdit->setDate(FValue.toDate());
	}
	else if (FDateTimeEdit != NULL)
	{
		FDateTimeEdit->setDateTime(FValue.toDateTime());
	}
	else if (FDoubleSpinBox != NULL)
	{
		FDoubleSpinBox->setValue(FValue.toDouble());
	}
	else if (FSpinBox)
	{
		FSpinBox->setValue(FValue.toInt());
	}

	emit childReset();
}

bool OptionsDialogWidget::eventFilter(QObject *AWatched, QEvent *AEvent)
{
	if (FValue.type()==QVariant::KeySequence && AWatched==FLineEdit && AEvent->type()==QEvent::KeyPress)
	{
		static const int extKeyMask = 0x01000000;
		static const int modifMask = Qt::META|Qt::SHIFT|Qt::CTRL|Qt::ALT;
		static const QList<int> controlKeys =  QList<int>() <<  Qt::Key_Shift << Qt::Key_Control << Qt::Key_Meta << Qt::Key_Alt << Qt::Key_AltGr;

		QKeyEvent *keyEvent = static_cast<QKeyEvent *>(AEvent);
		if (keyEvent->key()==0 || keyEvent->key()==Qt::Key_unknown)
			return true;
		if (keyEvent->key()>0x7F && (keyEvent->key() & extKeyMask)==0)
			return true;
		if (controlKeys.contains(keyEvent->key()))
			return true;
		if ((keyEvent->modifiers() & modifMask)==Qt::SHIFT && (keyEvent->key() & extKeyMask)==0)
			return true;

		QKeySequence keySeq((keyEvent->modifiers() & modifMask) | keyEvent->key());
		FLineEdit->setText(keySeq.toString(QKeySequence::NativeText));
		return true;
	}
	return QWidget::eventFilter(AWatched,AEvent);
}

void OptionsDialogWidget::insertEditor(const QString &ACaption, QWidget *AEditor, QHBoxLayout *ALayout)
{
	if (!ACaption.isEmpty())
	{
		FCaption = new QLabel(this);
		FCaption->setTextFormat(Qt::PlainText);
		FCaption->setText(ACaption);
		FCaption->setBuddy(AEditor);
		ALayout->addWidget(FCaption,0);
	}
	ALayout->addWidget(AEditor,1);
}

void OptionsDialogWidget::rigisterEditor(const OptionsNode &ANode, const QString &ACaption, QWidget *AEditor)
{
	FNode = ANode;
	FValue = ANode.value();
	QHBoxLayout *hlayout = new QHBoxLayout(this);

	FCaption = NULL;

	FCheckBox = qobject_cast<QCheckBox *>(AEditor);
	FLineEdit = qobject_cast<QLineEdit *>(AEditor);

	FComboBox = qobject_cast<QComboBox *>(AEditor);
	FFontComboBox = qobject_cast<QFontComboBox *>(AEditor);

	FSpinBox = qobject_cast<QSpinBox *>(AEditor);
	FTimeEdit = qobject_cast<QTimeEdit *>(AEditor);
	FDateEdit = qobject_cast<QDateEdit *>(AEditor);
	FDateTimeEdit = qobject_cast<QDateTimeEdit *>(AEditor);
	FDoubleSpinBox = qobject_cast<QDoubleSpinBox *>(AEditor);

	// Order is important
	if (FCheckBox != NULL)
	{
		FCheckBox->setChecked(FValue.toBool());
		connect(FCheckBox,SIGNAL(stateChanged(int)),SIGNAL(modified()));
		insertEditor(QString::null,FCheckBox,hlayout);
	}
	else if (FLineEdit != NULL)
	{
		if (FValue.type() == QVariant::KeySequence)
			FLineEdit->installEventFilter(this);

		if (FLineEdit->echoMode() == QLineEdit::Password)
			FLineEdit->setText(Options::decrypt(FValue.toByteArray()).toString());
		else
			FLineEdit->setText(FValue.toString());

		connect(FLineEdit,SIGNAL(textChanged(const QString &)),SIGNAL(modified()));
		insertEditor(ACaption,FLineEdit,hlayout);
	}
	else if (FFontComboBox != NULL)
	{
		FFontComboBox->setCurrentFont(FValue.value<QFont>());
		connect(FFontComboBox,SIGNAL(currentFontChanged(const QFont &)),SIGNAL(modified()));
		insertEditor(ACaption,FFontComboBox,hlayout);
	}
	else if (FComboBox != NULL)
	{
		FComboBox->setCurrentIndex(FComboBox->findData(FValue));
		connect(FComboBox,SIGNAL(currentIndexChanged(int)),SIGNAL(modified()));
		insertEditor(ACaption,FComboBox,hlayout);
	}
	else if (FTimeEdit != NULL)
	{
		FTimeEdit->setTime(FValue.toTime());
		connect(FTimeEdit,SIGNAL(dateTimeChanged(const QDateTime &)),SIGNAL(modified()));
		insertEditor(ACaption,FTimeEdit,hlayout);
	}
	else if (FDateEdit != NULL)
	{
		FDateEdit->setDate(FValue.toDate());
		connect(FDateEdit,SIGNAL(dateTimeChanged(const QDateTime &)),SIGNAL(modified()));
		insertEditor(ACaption,FDateEdit,hlayout);
	}
	else if (FDateTimeEdit != NULL)
	{
		FDateTimeEdit->setDateTime(FValue.toDateTime());
		connect(FDateTimeEdit,SIGNAL(dateTimeChanged(const QDateTime &)),SIGNAL(modified()));
		insertEditor(ACaption,FDateTimeEdit,hlayout);
	}
	else if (FDoubleSpinBox != NULL)
	{
		FDoubleSpinBox->setValue(FValue.toDouble());
		connect(FDoubleSpinBox,SIGNAL(valueChanged(double)),SIGNAL(modified()));
		insertEditor(ACaption,FDoubleSpinBox,hlayout);
	}
	else if (FSpinBox)
	{
		FSpinBox->setValue(FValue.toInt());
		connect(FSpinBox,SIGNAL(valueChanged(double)),SIGNAL(modified()));
		insertEditor(ACaption,FSpinBox,hlayout);
	}
	else
	{
		insertEditor(ACaption,AEditor,hlayout);
		REPORT_ERROR(QString("Unsupported options widget editor type=%1").arg(AEditor->objectName()));
	}

	setLayout(hlayout);
	layout()->setMargin(0);
}
