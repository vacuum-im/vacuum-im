#include "datadialog.h"

#include <QMessageBox>

DataDialog::DataDialog(IDataForm *ADataForm, QWidget *AParent) : IDataDialog(AParent)
{
  ui.setupUi(this);
  setAttribute(Qt::WA_DeleteOnClose,true);

  FAutoAccept = true;
  FDataForm = ADataForm;
  ui.wdtForm->setLayout(new QVBoxLayout);
  ui.wdtForm->layout()->addWidget(FDataForm->instance());
  ui.wdtForm->layout()->setMargin(0);
  setWindowTitle(FDataForm->title().isEmpty() ? tr("Fill out this form") : FDataForm->title());

  QToolBar *toolBar = new QToolBar(ui.wdtToolBar);
  ui.wdtToolBar->setLayout(new QVBoxLayout);
  ui.wdtToolBar->layout()->addWidget(toolBar);
  ui.wdtToolBar->layout()->setMargin(0);
  FToolBarChanger = new ToolBarChanger(toolBar);

  QString instr = FDataForm->instructions().join("\n");
  if (!instr.isEmpty())
    ui.lblInsructions->setText(instr);
  else
    ui.lblInsructions->setVisible(false);

  if (FDataForm->pageCount() > 1)
  {
    connect(ui.tlbPrevPage,SIGNAL(clicked()),SLOT(onPrevPageClicked()));
    connect(ui.tlbNextPage,SIGNAL(clicked()),SLOT(onNextPageClicked()));
  }
  else
    ui.wdtPages->setVisible(false);

  connect(ui.dbbDialogButtons,SIGNAL(accepted()),SLOT(onAcceptClicked()));
  connect(ui.dbbDialogButtons,SIGNAL(rejected()),SLOT(onRejectClicked()));

  updateDialog();
}

DataDialog::~DataDialog()
{

}

void DataDialog::showPage(int APage)
{
  if (APage >= 0 && APage < FDataForm->pageCount())
  {
    FDataForm->setCurrentPage(APage);
    updateDialog();
    emit currentPageChanged(APage);
  }
}

void DataDialog::showPrevPage()
{
  showPage(FDataForm->currentPage()-1);
}

void DataDialog::showNextPage()
{
  showPage(FDataForm->currentPage()+1);
}

void DataDialog::setAutoAccept(bool AAuto)
{
  FAutoAccept = AAuto;
}

void DataDialog::updateDialog()
{
  QString pageLabel = FDataForm->pageLabel(FDataForm->currentPage());
  ui.lblPageLabel->setText(pageLabel);
  ui.lblPageLabel->setVisible(!pageLabel.isEmpty());
  ui.lblPages->setText(tr("Page %1 of %2").arg(FDataForm->currentPage()+1).arg(FDataForm->pageCount()));
}

void DataDialog::onPrevPageClicked()
{
  showPrevPage();
}

void DataDialog::onNextPageClicked()
{
  showNextPage();
}

void DataDialog::onAcceptClicked()
{
  if (FAutoAccept)
  {
    if (!FDataForm->isValid())
    {
      QString message = tr("Current form values are not acceptable:");
      message += tr("<br>Invalid values in fields:");
      foreach(IDataField *field, FDataForm->fields())
        if (!field->isValid())
          message += tr("<br>- %1").arg(field->label());
      QMessageBox::StandardButton button = QMessageBox::warning(this,tr("Not Acceptable"),message,QMessageBox::Ok|QMessageBox::Ignore);
      if (button == QMessageBox::Ignore)
        accept();
    }
    else
      accept();
  }
}

void DataDialog::onRejectClicked()
{
  if (FAutoAccept)
    reject();
}

