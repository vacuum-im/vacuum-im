#ifndef DATAFIELDWIDGET_H
#define DATAFIELDWIDGET_H

#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QTextEdit>
#include <QDateEdit>
#include <QTimeEdit>
#include <QListWidget>
#include <QDateTimeEdit>
#include <interfaces/idataforms.h>

class TextEdit :
	public QTextEdit
{
	Q_OBJECT;
public:
	TextEdit(QWidget *AParent) : QTextEdit(AParent) {}
	QSize sizeHint() const {
		return minimumSizeHint();
	}
	QSize minimumSizeHint() const {
		return QSize(100, fontMetrics().lineSpacing()*5);
	}
};

class ListWidget :
	public QListWidget
{
	Q_OBJECT;
public:
	ListWidget(QWidget *AParent) : QListWidget(AParent) {};
	QSize sizeHint() const {
		return minimumSizeHint();
	}
	QSize minimumSizeHint() const {
		return QSize(100, fontMetrics().lineSpacing()*5);
	}
};

class DataFieldWidget :
	public QWidget,
	public IDataFieldWidget
{
	Q_OBJECT;
	Q_INTERFACES(IDataFieldWidget);
public:
	DataFieldWidget(IDataForms *ADataForms, const IDataField &AField, bool AReadOnly, QWidget *AParent);
	virtual QWidget *instance() { return this; }
	virtual bool isReadOnly() const;
	virtual IDataField dataField() const;
	virtual IDataField userDataField() const;
	virtual QVariant value() const;
	virtual void setValue(const QVariant &AValue);
	virtual IDataMediaWidget *mediaWidget() const;
signals:
	void changed();
	void focusIn(Qt::FocusReason AReason);
	void focusOut(Qt::FocusReason AReason);
protected:
	void appendLabel(const QString &AText, QWidget *ABuddy);
protected:
	bool eventFilter(QObject *AObject, QEvent *AEvent);
private:
	IDataForms *FDataForms;
	IDataMediaWidget *FMediaWidget;
private:
	QLabel *FLabel;
	QLineEdit *FLineEdit;
	QComboBox *FComboBox;
	QCheckBox *FCheckBox;
	QDateEdit *FDateEdit;
	QTimeEdit *FTimeEdit;
	QDateTimeEdit *FDateTimeEdit;
private:
	TextEdit *FTextEdit;
	ListWidget *FListWidget;
private:
	bool FReadOnly;
	IDataField FField;
};

#endif // DATAFIELDWIDGET_H
