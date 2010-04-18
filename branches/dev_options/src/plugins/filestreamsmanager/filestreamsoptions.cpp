#include "filestreamsoptions.h"

#include <QVBoxLayout>
#include <QFileDialog>

FileStreamsOptions::FileStreamsOptions(IDataStreamsManager *ADataManager, IFileStreamsManager *AFileManager, QWidget *AParent) : QWidget(AParent)
{
  ui.setupUi(this);
  ui.grbMethods->setLayout(new QVBoxLayout);

  FDataManager = ADataManager;
  FFileManager = AFileManager;

  connect(ui.pbtDirectory, SIGNAL(clicked()), SLOT(onDirectoryButtonClicked()));

  connect(ui.lneDirectory,SIGNAL(textChanged(const QString &)),SIGNAL(modified()));
  connect(ui.cmbMethod,SIGNAL(currentIndexChanged(int)),SIGNAL(modified()));

  reset();
}

FileStreamsOptions::~FileStreamsOptions()
{

}

void FileStreamsOptions::apply()
{
  Options::node(OPV_FILESTREAMS_DEFAULTDIR).setValue(ui.lneDirectory->text());

  QStringList acceptableMethods;
  foreach(QCheckBox *button, FMethods.keys())
    if (button->isChecked())
      acceptableMethods.append(FMethods.value(button));
  Options::node(OPV_FILESTREAMS_ACCEPTABLEMETHODS).setValue(acceptableMethods);

  Options::node(OPV_FILESTREAMS_DEFAULTMETHOD).setValue(ui.cmbMethod->itemData(ui.cmbMethod->currentIndex()).toString());

  emit childApply();
}

void FileStreamsOptions::reset()
{
  ui.lneDirectory->setText(Options::node(OPV_FILESTREAMS_DEFAULTDIR).value().toString());

  QStringList acceptableMethods = Options::node(OPV_FILESTREAMS_ACCEPTABLEMETHODS).value().toStringList();
  foreach(QString methodNS, FDataManager->methods())
  {
    IDataStreamMethod *streamMethod = FDataManager->method(methodNS);
    if (streamMethod)
    {
      QCheckBox *button = FMethods.key(methodNS);
      if (!button)
      {
        button = new QCheckBox(streamMethod->methodName());
        button->setToolTip(streamMethod->methodDescription());
        connect(button, SIGNAL(toggled(bool)),SLOT(onMethodButtonToggled(bool)));
        connect(button, SIGNAL(stateChanged(int)),SIGNAL(modified()));
        ui.grbMethods->layout()->addWidget(button);
        FMethods.insert(button, methodNS);
      }
      button->setChecked(acceptableMethods.contains(methodNS));
    }
  }

  ui.cmbMethod->setCurrentIndex(ui.cmbMethod->findData(Options::node(OPV_FILESTREAMS_DEFAULTMETHOD).value().toString()));

  emit childReset();
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
