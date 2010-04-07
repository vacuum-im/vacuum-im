#include "filestreamsoptions.h"

#include <QVBoxLayout>
#include <QFileDialog>

FileStreamsOptions::FileStreamsOptions(IDataStreamsManager *ADataManager, IFileStreamsManager *AFileManager, QWidget *AParent) : QWidget(AParent)
{
  ui.setupUi(this);
  FDataManager = ADataManager;
  FFileManager = AFileManager;

  ui.lneDirectory->setText(FFileManager->defaultDirectory());
  ui.chbSeparateDirectories->setChecked(FFileManager->separateDirectories());

  ui.grbMethods->setLayout(new QVBoxLayout);
  foreach(QString methodNS, FDataManager->methods())
  {
    IDataStreamMethod *streamMethod = FDataManager->method(methodNS);
    if (streamMethod)
    {
      QCheckBox *button = new QCheckBox(streamMethod->methodName());
      button->setToolTip(streamMethod->methodDescription());
      connect(button, SIGNAL(toggled(bool)),SLOT(onMethodButtonToggled(bool)));
      FMethods.insert(button, methodNS);
      button->setChecked(FFileManager->streamMethods().contains(methodNS));
      ui.grbMethods->layout()->addWidget(button);
    }
  }
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

  QList<QString> oldMethods = FFileManager->streamMethods();
  foreach(QCheckBox *button, FMethods.keys())
  {
    if (button->isChecked())
    {
      QString methodNS = FMethods.value(button);
      FFileManager->insertStreamMethod(methodNS);
      oldMethods.removeAt(oldMethods.indexOf(methodNS));
    }
  }

  foreach(QString methodNS, oldMethods)
    FFileManager->removeStreamMethod(methodNS);
  
  FFileManager->setDefaultStreamMethod(ui.cmbMethod->itemData(ui.cmbMethod->currentIndex()).toString());

  emit optionsAccepted();
}

void FileStreamsOptions::onDirectoryButtonClicked()
{
  QString dir = QFileDialog::getExistingDirectory(this,tr("Select default directory"), ui.lneDirectory->text());
  if (!dir.isEmpty())
    ui.lneDirectory->setText(dir);
}

void FileStreamsOptions::onMethodButtonToggled(bool ACkecked)
{
  QString methodNS = FMethods.value(qobject_cast<QCheckBox *>(sender()));
  IDataStreamMethod *streamMethod = FDataManager->method(methodNS);
  if (streamMethod)
  {
    if (ACkecked)
      ui.cmbMethod->addItem(streamMethod->methodName(), methodNS);
    else
      ui.cmbMethod->removeItem(ui.cmbMethod->findData(methodNS));
  }
}
