#include "dataformwidget.h"

#include <QLabel>
#include <QGroupBox>
#include <QTabWidget>
#include <QScrollBar>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QTextDocument>
#include <QApplication>
#include <QDesktopWidget>

class ScrollArea : 
	public QScrollArea
{
public:
	ScrollArea(QWidget *AParent = NULL): QScrollArea(AParent)
	{
		setWidgetResizable(true);
		setFrameShape(QFrame::NoFrame);
	}
	virtual QSize sizeHint() const
	{
		QSize sh(2*frameWidth()+1, 2*frameWidth()+1);
		if (verticalScrollBar())
			sh.rwidth() += verticalScrollBar()->sizeHint().width();
		if (horizontalScrollBar())
			sh.rheight() += horizontalScrollBar()->sizeHint().height();
		if (widget())
			sh += widgetResizable() ? widget()->sizeHint() : widget()->size();

		QSize desktopSize = QApplication::desktop()->availableGeometry(this).size();
		return sh.boundedTo(desktopSize/2);
	}
	virtual QSize minimumSizeHint() const
	{
		QSize desktopSize = QApplication::desktop()->availableGeometry(this).size();
		return sizeHint().boundedTo(desktopSize/4);
	}
	virtual bool event(QEvent *AEvent)
	{
		if (AEvent->type() == QEvent::LayoutRequest)
			updateGeometry();
		return QScrollArea::event(AEvent);
	}
};

DataFormWidget::DataFormWidget(IDataForms *ADataForms, const IDataForm &AForm, QWidget *AParent) : QWidget(AParent)
{
	FForm = AForm;
	FTableWidget = NULL;
	FDataForms = ADataForms;

	QVBoxLayout *formLayout = new QVBoxLayout(this);
	formLayout->setMargin(0);

	foreach(const QString &text, FForm.instructions)
	{
		QLabel *label = new QLabel(this);
		label->setText(text);
		label->setWordWrap(true);
		label->setTextFormat(Qt::PlainText);
		label->setAlignment(Qt::AlignCenter);
		formLayout->addWidget(label);
	}

	if (!FForm.fields.isEmpty() || !FForm.table.columns.isEmpty())
	{
		foreach(const IDataField &field, FForm.fields)
		{
			IDataFieldWidget *fwidget = FDataForms->fieldWidget(field,!FForm.type.isEmpty() && FForm.type!=DATAFORM_TYPE_FORM,this);
			fwidget->instance()->setVisible(false);
			if (fwidget->mediaWidget() != NULL)
			{
				IDataMediaWidget *mwidget = fwidget->mediaWidget();
				connect(mwidget->instance(),SIGNAL(mediaShown()),SLOT(onFieldMediaShown()));
				connect(mwidget->instance(),SIGNAL(mediaError(const XmppError &)),SLOT(onFieldMediaError(const XmppError &)));
			}
			connect(fwidget->instance(),SIGNAL(changed()),SLOT(onFieldChanged()));
			connect(fwidget->instance(),SIGNAL(focusIn(Qt::FocusReason)),SLOT(onFieldFocusIn(Qt::FocusReason)));
			connect(fwidget->instance(),SIGNAL(focusOut(Qt::FocusReason)),SLOT(onFieldFocusOut(Qt::FocusReason)));
			FFieldWidgets.append(fwidget);
		}

		if (!FForm.table.columns.isEmpty())
		{
			FTableWidget = FDataForms->tableWidget(FForm.table,this);
			FTableWidget->instance()->setVisible(false);
			connect(FTableWidget->instance(),SIGNAL(activated(int, int)),SIGNAL(cellActivated(int, int)));
			connect(FTableWidget->instance(),SIGNAL(changed(int,int,int,int)),SIGNAL(cellChanged(int,int,int,int)));
		}

		if (FForm.pages.count() < 2)
		{
			ScrollArea *scroll = new ScrollArea(this);
			formLayout->addWidget(scroll);

			QWidget *scrollWidget = new QWidget(scroll);
			QVBoxLayout *scrollLayout = new QVBoxLayout(scrollWidget);
			scrollLayout->setMargin(0);

			bool stretch = true;
			if (FForm.pages.count() == 0)
			{
				foreach(IDataFieldWidget *fwidget, FFieldWidgets)
				{
					stretch &= !isStretch(fwidget);
					scrollLayout->addWidget(fwidget->instance());
					fwidget->instance()->setVisible(fwidget->dataField().type != DATAFIELD_TYPE_HIDDEN);
				}

				if (FTableWidget)
				{
					stretch = false;
					scrollLayout->addWidget(FTableWidget->instance());
					FTableWidget->instance()->setVisible(true);
				}
			}
			else
			{
				stretch = insertLayout(FForm.pages.first(),scrollWidget);
			}

			if (stretch)
				scrollLayout->addStretch();

			scroll->setWidget(scrollWidget);
		}
		else
		{
			QTabWidget *tabs = new QTabWidget(this);
			formLayout->addWidget(tabs);

			foreach(const IDataLayout &page, FForm.pages)
			{
				ScrollArea *scroll = new ScrollArea(tabs);
				tabs->addTab(scroll, !page.label.isEmpty() ? page.label : tr("Page %1").arg(tabs->count()+1));

				QWidget *scrollWidget = new QWidget(scroll);
				QVBoxLayout *scrollLayout = new QVBoxLayout(scrollWidget);
				scrollLayout->setMargin(0);

				if (insertLayout(page,scrollWidget))
					scrollLayout->addStretch();

				scroll->setWidget(scrollWidget);
			}
		}
	}
}

bool DataFormWidget::isSubmitValid() const
{
	return FDataForms->isSubmitValid(dataForm(),userDataForm());
}

bool DataFormWidget::checkForm(bool AAllowInvalid) const
{
	if (FForm.type.isEmpty() || FForm.type==DATAFORM_TYPE_FORM)
	{
		QString message;
		int invalidCount = 0;
		QList<IDataField> fields = userDataForm().fields;
		foreach(const IDataField &field, fields)
		{
			if (!field.var.isEmpty() && !FDataForms->isFieldValid(field,DATAFORM_TYPE_SUBMIT))
			{
				invalidCount++;
				message += QString("- <b>%2</b><br>").arg(!field.label.isEmpty() ? field.label.toHtmlEscaped() : field.var.toHtmlEscaped());
			}
		}
		if (invalidCount > 0)
		{
			message = tr("The are %1 field(s) with invalid values:<br>").arg(invalidCount) + message;
			QMessageBox::StandardButtons buttons = QMessageBox::Ok;
			if (AAllowInvalid)
			{
				message += "<br>";
				message += tr("Do you want to continue with invalid values?");
				buttons = QMessageBox::Yes|QMessageBox::No;
			}
			return (QMessageBox::warning(NULL,windowTitle(),message,buttons) == QMessageBox::Yes);
		}
	}
	return true;
}

IDataTableWidget *DataFormWidget::tableWidget() const
{
	return FTableWidget;
}

IDataFieldWidget *DataFormWidget::fieldWidget(int AIndex) const
{
	return FFieldWidgets.value(AIndex);
}

IDataFieldWidget *DataFormWidget::fieldWidget(const QString &AVar) const
{
	return fieldWidget(FDataForms->fieldIndex(AVar,FForm.fields));
}

const IDataForm &DataFormWidget::dataForm() const
{
	return FForm;
}

IDataForm DataFormWidget::userDataForm() const
{
	IDataForm form = FForm;
	for (int i=0; i<FFieldWidgets.count(); i++)
		form.fields[i] = FFieldWidgets.at(i)->userDataField();
	return form;
}

IDataForm DataFormWidget::submitDataForm() const
{
	return FDataForms->dataSubmit(userDataForm());
}

bool DataFormWidget::isStretch(IDataFieldWidget *AWidget) const
{
	QString type = AWidget->dataField().type;
	return type == DATAFIELD_TYPE_LISTMULTI || type == DATAFIELD_TYPE_JIDMULTI || type == DATAFIELD_TYPE_TEXTMULTI;
}

bool DataFormWidget::insertLayout(const IDataLayout &ALayout, QWidget *AWidget)
{
	bool stretch = true;
	int textCounter = 0;
	int fieldCounter = 0;
	int sectionCounter = 0;
	foreach(const QString &childName, ALayout.childOrder)
	{
		if (childName == "text")
		{
			QLabel *label = new QLabel(AWidget);
			label->setWordWrap(true);
			label->setTextFormat(Qt::PlainText);
			label->setText(ALayout.text.value(textCounter++));
			AWidget->layout()->addWidget(label);
		}
		else if (childName == "fieldref")
		{
			QString var = ALayout.fieldrefs.value(fieldCounter++);
			IDataFieldWidget *fwidget = fieldWidget(var);
			if (fwidget)
			{
				stretch &= !isStretch(fwidget);
				AWidget->layout()->addWidget(fwidget->instance());
				fwidget->instance()->setVisible(fwidget->dataField().type!=DATAFIELD_TYPE_HIDDEN);
			}
		}
		else if (childName == "reportedref")
		{
			if (FTableWidget)
			{
				stretch = false;
				AWidget->layout()->addWidget(FTableWidget->instance());
			}
		}
		else if (childName == "section")
		{
			IDataLayout section = ALayout.sections.value(sectionCounter++);
			QGroupBox *groupBox = new QGroupBox(AWidget);
			groupBox->setLayout(new QVBoxLayout(groupBox));
			groupBox->setTitle(section.label);
			groupBox->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Minimum);
			stretch &= insertLayout(section,groupBox);
			AWidget->layout()->addWidget(groupBox);
		}
	}
	return stretch;
}

void DataFormWidget::onFieldMediaShown()
{
	IDataMediaWidget *mwidget = qobject_cast<IDataMediaWidget *>(sender());
	IDataFieldWidget *fwidget = mwidget!=NULL ? qobject_cast<IDataFieldWidget *>(mwidget->instance()->parentWidget()) : NULL;
	if (fwidget)
		emit fieldMediaShown(fwidget);
}

void DataFormWidget::onFieldMediaError(const XmppError &AError)
{
	IDataMediaWidget *mwidget = qobject_cast<IDataMediaWidget *>(sender());
	IDataFieldWidget *fwidget = mwidget!=NULL ? qobject_cast<IDataFieldWidget *>(mwidget->instance()->parentWidget()) : NULL;
	if (fwidget)
		emit fieldMediaError(fwidget,AError);
}

void DataFormWidget::onFieldChanged()
{
	IDataFieldWidget *fwidget = qobject_cast<IDataFieldWidget *>(sender());
	if (fwidget)
		fieldChanged(fwidget);
}

void DataFormWidget::onFieldFocusIn(Qt::FocusReason AReason)
{
	IDataFieldWidget *fwidget = qobject_cast<IDataFieldWidget *>(sender());
	if (fwidget)
		emit fieldFocusIn(fwidget,AReason);
}

void DataFormWidget::onFieldFocusOut(Qt::FocusReason AReason)
{
	IDataFieldWidget *fwidget = qobject_cast<IDataFieldWidget *>(sender());
	if (fwidget)
		emit fieldFocusOut(fwidget,AReason);
}
