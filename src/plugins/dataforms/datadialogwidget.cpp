#include "datadialogwidget.h"

#include <QVBoxLayout>
#include <QPushButton>
#include <utils/logger.h>

DataDialogWidget::DataDialogWidget(IDataForms *ADataForms, const IDataForm &AForm, QWidget *AParent) : QDialog(AParent)
{
	REPORT_VIEW;
	setAttribute(Qt::WA_DeleteOnClose,true);

	FFormWidget = NULL;
	FAllowInvalid = false;
	FDataForms = ADataForms;

	QToolBar *toolBar = new QToolBar(this);
	FToolBarChanger = new ToolBarChanger(toolBar);

	FFormHolder = new QWidget(this);
	FFormHolder->setLayout(new QVBoxLayout());
	FFormHolder->layout()->setMargin(0);

	QFrame *hline = new QFrame(this);
	hline->setFrameShape(QFrame::HLine);
	hline->setFrameShadow(QFrame::Raised);

	FDialogButtons = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,Qt::Horizontal,this);
	connect(FDialogButtons,SIGNAL(clicked(QAbstractButton *)),SLOT(onDialogButtonClicked(QAbstractButton *)));

	QVBoxLayout *dialogLayout = new QVBoxLayout(this);
	dialogLayout->setMargin(5);
	dialogLayout->setMenuBar(toolBar);
	dialogLayout->addWidget(FFormHolder);
	dialogLayout->addWidget(hline);
	dialogLayout->addWidget(FDialogButtons);

	setForm(AForm);
}

DataDialogWidget::~DataDialogWidget()
{
	emit dialogDestroyed(this);
}

ToolBarChanger *DataDialogWidget::toolBarChanged() const
{
	return FToolBarChanger;
}

QDialogButtonBox *DataDialogWidget::dialogButtons() const
{
	return FDialogButtons;
}

IDataFormWidget *DataDialogWidget::formWidget() const
{
	return FFormWidget;
}

void DataDialogWidget::setForm(const IDataForm &AForm)
{
	if (FFormWidget)
	{
		FFormHolder->layout()->removeWidget(FFormWidget->instance());
		FFormWidget->instance()->deleteLater();
		emit formWidgetDestroyed(FFormWidget);
	}

	setWindowTitle(AForm.title);
	FFormWidget = FDataForms->formWidget(AForm,FFormHolder);
	FFormHolder->layout()->addWidget(FFormWidget->instance());
	connect(FFormWidget->instance(),SIGNAL(fieldChanged(IDataFieldWidget *)),SLOT(onFormFieldChanged()));

	onFormFieldChanged();
	emit formWidgetCreated(FFormWidget);
}

bool DataDialogWidget::allowInvalid() const
{
	return FAllowInvalid;
}

void DataDialogWidget::setAllowInvalid(bool AAllowInvalid)
{
	FAllowInvalid = AAllowInvalid;
	onFormFieldChanged();
}

void DataDialogWidget::onFormFieldChanged()
{
	if (FFormWidget)
	{
		bool valid = FAllowInvalid || FFormWidget->isSubmitValid();
		FDialogButtons->button(QDialogButtonBox::Ok)->setEnabled(valid);
	}
}

void DataDialogWidget::onDialogButtonClicked(QAbstractButton *AButton)
{
	switch (FDialogButtons->standardButton(AButton))
	{
	case QDialogButtonBox::Ok:
		if (FFormWidget->checkForm(FAllowInvalid))
			accept();
		break;
	case QDialogButtonBox::Cancel:
		reject();
		break;
	default:
		break;
	}
}
