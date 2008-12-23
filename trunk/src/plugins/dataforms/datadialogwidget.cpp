#include "datadialogwidget.h"

#include <QVBoxLayout>

DataDialogWidget::DataDialogWidget(IDataForms *ADataForms, const IDataForm &AForm, QWidget *AParent) : QDialog(AParent)
{
  setAttribute(Qt::WA_DeleteOnClose,true);
  setLayout(new QVBoxLayout(this));
  layout()->setMargin(5);
  resize(500,325);

  FFormWidget = NULL;
  FAllowInvalid = false;
  FDataForms = ADataForms;

  QToolBar *toolBar = new QToolBar(this);
  FToolBarChanger = new ToolBarChanger(toolBar);
  layout()->setMenuBar(toolBar);

  FFormHolder = new QWidget(this);
  FFormHolder->setLayout(new QVBoxLayout(FFormHolder));
  FFormHolder->layout()->setMargin(0);
  layout()->addWidget(FFormHolder);

  QFrame *hline = new QFrame(this);
  hline->setFrameShape(QFrame::HLine);
  hline->setFrameShadow(QFrame::Raised);
  layout()->addWidget(hline);

  FDialogButtons = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,Qt::Horizontal,this);
  connect(FDialogButtons,SIGNAL(clicked(QAbstractButton *)),SLOT(onDialogButtonClicked(QAbstractButton *)));
  layout()->addWidget(FDialogButtons);

  setForm(AForm);
}

DataDialogWidget::~DataDialogWidget()
{
  emit dialogDestroyed(this);
}

void DataDialogWidget::setForm(const IDataForm &AForm)
{
  if (FFormWidget)
  {
    FFormHolder->layout()->removeWidget(FFormWidget->instance());
    emit formWidgetDestroyed(FFormWidget);
    FFormWidget->instance()->deleteLater();
  }
  setWindowTitle(AForm.title);
  FFormWidget = FDataForms->formWidget(AForm,this);
  FFormHolder->layout()->addWidget(FFormWidget->instance());
  emit formWidgetCreated(FFormWidget);
}

void DataDialogWidget::onDialogButtonClicked(QAbstractButton *AButton)
{
  switch(FDialogButtons->standardButton(AButton))
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

