#include "filestreamsoptions.h"

#include <QFileDialog>

FileStreamsOptions::FileStreamsOptions(IDataStreamsManager *ADataManager, IFileStreamsManager *AFileManager, QWidget *AParent) : QWidget(AParent)
{
  ui.setupUi(this);
  FDataManager = ADataManager;
  FFileManager = AFileManager;

  ui.lneDirectory->setText(FFileManager->defaultDirectory());
  ui.chbSeparateDirectories->setChecked(FFileManager->separateDirectories());

  foreach(QString methodNS, FFileManager->streamMethods())
    ui.cmbMethod->addItem(FDataManager->method(methodNS)->methodName(), methodNS);
  ui.cmbMethod->setCurrentIndex(ui.cmbMethod->findData(FFileManager->defaultStreamMethod()));

  connect(ui.pbtDirectory, SIGNAL(clicked()), SLOT(onDirectoryButtonClicked()));
}

FileStreamsOptions::~FileStreamsOptions()
{

}

void FileStreamsOptions::apply()
{
  FFileManager->setDefaultDirectory(ui.lneDirectory->text());
  FFileManager->setSeparateDirectories(ui.chbSeparateDirectories->isChecked());
  FFileManager->setDefaultStreamMethod(ui.cmbMethod->itemData(ui.cmbMethod->currentIndex()).toString());
  emit optionsAccepted();
}

void FileStreamsOptions::onDirectoryButtonClicked()
{
  QString dir = QFileDialog::getExistingDirectory(this,tr("Select default directory"), ui.lneDirectory->text());
  if (!dir.isEmpty())
    ui.lneDirectory->setText(dir);
}
