#include "dataformwidget.h"

#include <QLabel>
#include <QGroupBox>
#include <QScrollBar>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QTabWidget>
#include <QTextDocument>
#include <QMessageBox>
#include <utils/QtEscape.h>

class ScrollArea : 
   public QScrollArea
{
public:
   ScrollArea(QWidget *AParent = NULL): QScrollArea(AParent)
   {

   }
   virtual QSize sizeHint() const 
   {
      QSize sh(2*frameWidth()+1,2*frameWidth()+1);
      if (verticalScrollBar())
         sh.setWidth(sh.width() + verticalScrollBar()->sizeHint().width());
      if (horizontalScrollBar())
         sh.setHeight(sh.height() + horizontalScrollBar()->sizeHint().height());
      if (widget())
         sh += widgetResizable() ? widget()->sizeHint() : widget()->size();
      return sh;
   }
};

DataFormWidget::DataFormWidget(IDataForms *ADataForms, const IDataForm &AForm, QWidget *AParent) : QWidget(AParent)
{
	FForm = AForm;
	FDataForms = ADataForms;

	if (FForm.tabel.columns.count()>0)
	{
		FTableWidget = FDataForms->tableWidget(FForm.tabel,this);
		FTableWidget->instance()->setVisible(false);
		connect(FTableWidget->instance(),SIGNAL(activated(int, int)),SIGNAL(cellActivated(int, int)));
		connect(FTableWidget->instance(),SIGNAL(changed(int,int,int,int)),SIGNAL(cellChanged(int,int,int,int)));
	}
	else
		FTableWidget = NULL;

	foreach(IDataField field, FForm.fields)
	{
		IDataFieldWidget *fwidget = FDataForms->fieldWidget(field,!FForm.type.isEmpty() && FForm.type!=DATAFORM_TYPE_FORM,this);
		fwidget->instance()->setVisible(false);
		connect(fwidget->instance(),SIGNAL(focusIn(Qt::FocusReason)),SLOT(onFieldFocusIn(Qt::FocusReason)));
		connect(fwidget->instance(),SIGNAL(focusOut(Qt::FocusReason)),SLOT(onFieldFocusOut(Qt::FocusReason)));
		FFieldWidgets.append(fwidget);
	}

	setLayout(new QVBoxLayout(this));
	layout()->setMargin(0);

	foreach(QString text, FForm.instructions)
	{
		QLabel *label = new QLabel(this);
		label->setWordWrap(true);
		label->setTextFormat(Qt::PlainText);
		label->setAlignment(Qt::AlignCenter);
		label->setText(text);
		layout()->addWidget(label);
	}

	if (FForm.pages.count() < 2)
	{
		ScrollArea *scroll = new ScrollArea(this);
		scroll->setWidgetResizable(true);
		scroll->setFrameShape(QFrame::NoFrame);
		layout()->addWidget(scroll);

		QWidget *widget = new QWidget(scroll);
		widget->setLayout(new QVBoxLayout(widget));
		widget->layout()->setMargin(0);

		bool stretch = true;
		if (FForm.pages.count() == 0)
		{
			foreach(IDataFieldWidget *fwidget, FFieldWidgets)
			{
				stretch &= !isStretch(fwidget);
				widget->layout()->addWidget(fwidget->instance());
				fwidget->instance()->setVisible(fwidget->dataField().type!=DATAFIELD_TYPE_HIDDEN);
			}

			if (FTableWidget)
			{
				stretch = false;
				widget->layout()->addWidget(FTableWidget->instance());
				FTableWidget->instance()->setVisible(true);
			}
		}
		else
			stretch = insertLayout(FForm.pages.first(),widget);

		if (stretch)
			static_cast<QVBoxLayout *>(widget->layout())->addStretch();

		scroll->setWidget(widget);
	}
	else
	{
		QTabWidget *tabs = new QTabWidget(this);
		layout()->addWidget(tabs);

		foreach(IDataLayout page, FForm.pages)
		{
			ScrollArea *scroll = new ScrollArea(tabs);
			scroll->setWidgetResizable(true);
			scroll->setFrameShape(QFrame::NoFrame);
			tabs->addTab(scroll, !page.label.isEmpty() ? page.label : tr("Page %1").arg(tabs->count()+1));

			QWidget *pwidget = new QWidget(scroll);
			pwidget->setLayout(new QVBoxLayout(pwidget));
			if (insertLayout(page,pwidget))
				static_cast<QVBoxLayout *>(pwidget->layout())->addStretch();

			scroll->setWidget(pwidget);
		}
	}
}

DataFormWidget::~DataFormWidget()
{

}

bool DataFormWidget::checkForm(bool AAllowInvalid) const
{
	if (FForm.type.isEmpty() || FForm.type==DATAFORM_TYPE_FORM)
	{
		QString message;
		int invalidCount = 0;
		QList<IDataField> fields = userDataForm().fields;
		foreach(IDataField field, fields)
		{
			if (!field.var.isEmpty() && !FDataForms->isFieldValid(field,DATAFORM_TYPE_SUBMIT))
			{
				invalidCount++;
				message += QString("- <b>%2</b><br>").arg(Qt::escape(!field.label.isEmpty() ? field.label : field.var));
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

IDataForm DataFormWidget::userDataForm() const
{
	IDataForm form = FForm;
	for (int i=0; i<FFieldWidgets.count(); i++)
		form.fields[i] = FFieldWidgets.at(i)->userDataField();
	return form;
}

const IDataForm &DataFormWidget::dataForm() const
{
   return FForm;
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
	foreach(QString childName, ALayout.childOrder)
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
