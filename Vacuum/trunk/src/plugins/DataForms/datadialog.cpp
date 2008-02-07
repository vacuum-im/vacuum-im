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

  if (FDataForm->pageControl())
  {
    ui.wdtPages->setLayout(new QHBoxLayout);
    ui.wdtPages->layout()->addWidget(FDataForm->pageControl());
    ui.wdtPages->layout()->setMargin(0);
  }

  connect(ui.dbbDialogButtons,SIGNAL(accepted()),SLOT(onAcceptClicked()));
  connect(ui.dbbDialogButtons,SIGNAL(rejected()),SLOT(onRejectClicked()));
}

DataDialog::~DataDialog()
{

}

void DataDialog::showPage(int APage)
{
  FDataForm->setCurrentPage(APage);
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

void DataDialog::onAcceptClicked()
{
  if (FAutoAccept)
  {
    if (!FDataForm->isValid())
    {
      QMessageBox::StandardButton button = QMessageBox::warning(this,tr("Not Acceptable"),FDataForm->invalidMessage(),QMessageBox::Ok|QMessageBox::Ignore);
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

