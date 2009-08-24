#include "streamdialog.h"

#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QFileDialog>
#include <QVBoxLayout>

StreamDialog::StreamDialog(IDataStreamsManager *ADataManager, IFileStreamsManager *AFileManager, 
                           IFileStream *AFileStream, QWidget *AParent) : QDialog(AParent)
{
  ui.setupUi(this);
  ui.grbConnections->setLayout(new QVBoxLayout);
  setAttribute(Qt::WA_DeleteOnClose,true);

  FFileStream = AFileStream;
  FFileManager = AFileManager;
  FDataManager = ADataManager;

  if (FFileStream->streamKind() == IFileStream::SendFile)
  {
    setWindowTitle(tr("Send File - %1").arg(FFileStream->streamJid().full()));
    ui.lblContactLabel->setText("To:");
  }
  else
  {
    setWindowTitle(tr("Receive File - %1").arg(FFileStream->streamJid().full()));
    ui.lblContactLabel->setText("From:");
  }
  ui.lblContact->setText(FFileStream->contactJid().hFull());

  connect(FFileStream->instance(),SIGNAL(stateChanged()),SLOT(onStreamStateChanged()));
  connect(FFileStream->instance(),SIGNAL(speedUpdated()),SLOT(onStreamSpeedUpdated()));
  connect(FFileStream->instance(),SIGNAL(propertiesChanged()),SLOT(onStreamPropertiesChanged()));
  connect(FFileStream->instance(),SIGNAL(streamDestroyed()),SLOT(onStreamDestroyed()));

  connect(ui.tlbFile,SIGNAL(clicked(bool)),SLOT(onFileButtonClicked(bool)));
  connect(ui.bbxButtons,SIGNAL(clicked(QAbstractButton *)),SLOT(onDialogButtonClicked(QAbstractButton *)));

  onStreamPropertiesChanged();
  onStreamStateChanged();
  onStreamSpeedUpdated();
}

StreamDialog::~StreamDialog()
{
  emit dialogDestroyed();
}

IFileStream *StreamDialog::stream() const
{
  return FFileStream;
}

QList<QString> StreamDialog::selectedMethods() const
{
  QList<QString> methods;
  foreach (QCheckBox *button, FMethodButtons.keys())
    if (button->isChecked())
      methods.append(FMethodButtons.value(button)); 
  return methods;
}

void StreamDialog::setSelectableMethods(const QList<QString> &AMethods)
{
  qDeleteAll(FMethodButtons.keys());
  FMethodButtons.clear();
  foreach(QString methodNS, AMethods)
  {
    IDataStreamMethod *method = FDataManager->method(methodNS);
    if (method)
    {
      QCheckBox *button = new QCheckBox(method->methodName(methodNS),ui.grbConnections);
      button->setToolTip(method->methodDescription(methodNS));
      button->setAutoExclusive(FFileStream->streamKind() == IFileStream::ReceiveFile);
      button->setChecked(FFileStream->streamKind()==IFileStream::SendFile || FFileManager->defaultStreamMethod()==methodNS);
      ui.grbConnections->layout()->addWidget(button);
      FMethodButtons.insert(button,methodNS);
    }
  }
}

QString StreamDialog::sizeName(qint64 ABytes) const
{
  int precision = 0;
  QString units = tr("B","Byte");
  qreal value = ABytes;

  if (value > 1024)
  {
    value = value / 1024;
    units = tr("KB","Kilobyte");
  }
  if (value > 1024)
  {
    value = value / 1024;
    units = tr("MB","Megabyte");
  }
  if (value > 1024)
  {
    value = value / 1024;
    units = tr("GB","Gigabyte");
  }

  if (value < 100)
    precision = 1;
  else if (value < 10)
    precision = 2;

  return QString::number(value,'f',precision)+units;
}

qint64 StreamDialog::curPosition() const
{
  return FFileStream->rangeOffset()>0 ? FFileStream->rangeOffset()+FFileStream->progress() : FFileStream->progress();
}

qint64 StreamDialog::maxPosition() const
{
  return FFileStream->rangeLength() >0 ? (FFileStream->rangeOffset()>0 ? FFileStream->rangeOffset()+FFileStream->rangeLength() : FFileStream->rangeLength()) : FFileStream->fileSize();
}

void StreamDialog::onStreamStateChanged()
{
  switch (FFileStream->streamState())
  {
  case IFileStream::Creating:
    ui.tlbFile->setEnabled(true);
    ui.lneFile->setReadOnly(FFileStream->streamKind()==IFileStream::SendFile);
    ui.pteDescription->setReadOnly(false);
    ui.grbConnections->setVisible(!FMethodButtons.isEmpty());
    if (FFileStream->streamKind()==IFileStream::SendFile)
      ui.bbxButtons->setStandardButtons(QDialogButtonBox::Ok|QDialogButtonBox::Close);
    else
      ui.bbxButtons->setStandardButtons(QDialogButtonBox::Ok|QDialogButtonBox::Abort);
    break;
  case IFileStream::Negotiating:
  case IFileStream::Connecting:
  case IFileStream::Transfering:
    ui.tlbFile->setEnabled(false);
    ui.lneFile->setReadOnly(true);
    ui.pteDescription->setReadOnly(true);
    ui.grbConnections->setVisible(false);
    ui.bbxButtons->setStandardButtons(QDialogButtonBox::Abort|QDialogButtonBox::Close);
    break;
  case IFileStream::Disconnecting:
  case IFileStream::Finished:
  case IFileStream::Canceled:
    ui.tlbFile->setEnabled(false);
    ui.lneFile->setReadOnly(true);
    ui.pteDescription->setReadOnly(true);
    ui.grbConnections->setVisible(false);
    ui.bbxButtons->setStandardButtons(QDialogButtonBox::Close);
    break;
  }
  ui.lblStatus->setText(FFileStream->stateString());
}

void StreamDialog::onStreamSpeedUpdated()
{
  ui.pgbPrgress->setValue(curPosition());
  ui.lblProgress->setText(tr("Transfered %1 of %2. Speed %3")
    .arg(sizeName(curPosition())).arg(sizeName(maxPosition())).arg(sizeName(FFileStream->speed())+tr("/sec")));
}

void StreamDialog::onStreamPropertiesChanged()
{
  ui.lneFile->setText(FFileStream->fileName());
  ui.pteDescription->setPlainText(FFileStream->fileDescription());
  ui.pgbPrgress->setMaximum(maxPosition());
  onStreamSpeedUpdated();
}

void StreamDialog::onStreamDestroyed()
{
  close();
}

void StreamDialog::onFileButtonClicked(bool)
{
  if (FFileStream->streamState() == IFileStream::Creating)
  {
    QString file;
    if (FFileStream->streamKind()==IFileStream::ReceiveFile) 
    {
      file = QFileDialog::getSaveFileName(this,tr("Select file for receive"),QString::null,QString::null,NULL,QFileDialog::DontConfirmOverwrite);
      QFileInfo fileInfo(file);
      if (fileInfo.exists())
      {
        if (FFileStream->isRangeSupported() && fileInfo.size()<FFileStream->fileSize())
        {
          QMessageBox::StandardButton button = QMessageBox::question(this,tr("Continue file transfer"),
            tr("A file with this name, but a smaller size already exists. Do you want to continue file transfer?"),
            QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel);
          if (button == QMessageBox::Yes)
          {
            FFileStream->setRangeOffset(fileInfo.size());
          }
          else if (button == QMessageBox::No)
          {
            if (!QFile::remove(file))
            {
              QMessageBox::warning(this,tr("Warning"),tr("Can not delete existing file"));
              file.clear();
            }
          }
          else
          {
            file.clear();
          }
        }
        else
        {
          QMessageBox::StandardButton button = QMessageBox::question(this,tr("Remove existing file"),
            tr("A file with this name already exists. Do you want to remove existing file?"),
            QMessageBox::Yes|QMessageBox::No);
          if (button == QMessageBox::Yes)
          {
            if (!QFile::remove(file))
            {
              QMessageBox::warning(this,tr("Warning"),tr("Can not delete existing file"));
              file.clear();
            }
          }
          else
          {
            file.clear();
          }
        }
      }
    }
    else
    {
      file = QFileDialog::getOpenFileName(this,tr("Select file to send"));
    }

    if (!file.isEmpty())
    {
      FFileStream->setFileName(file);
    }
  }
}

void StreamDialog::onDialogButtonClicked(QAbstractButton *AButton)
{
  if (ui.bbxButtons->standardButton(AButton) ==  QDialogButtonBox::Ok) 
  {
    QList<QString> methods = selectedMethods();
    if (!methods.isEmpty())
    {
      if (FFileStream->streamKind() == IFileStream::SendFile)
      {
        FFileStream->setFileName(ui.lneFile->text());
        FFileStream->setFileDescription(ui.pteDescription->toPlainText());
        if (!FFileStream->initStream(methods))
          QMessageBox::warning(this,tr("Warning"),tr("Unable to send request for file transfer, check settings and try again!"));
      }
      else
      {
        IDataStreamMethod *method = FDataManager->method(methods.first());
        if (method)
        {
          IDataStreamOptions options = method->dataStreamOptions(methods.first());
          FFileStream->setFileName(ui.lneFile->text());
          FFileStream->setFileDescription(ui.pteDescription->toPlainText());
          if (!FFileStream->startStream(options))
            QMessageBox::warning(this,tr("Warning"),tr("Unable to start the file transfer, check settings and try again!"));
        }
        else
          QMessageBox::warning(this,tr("Warning"),tr("Selected way to connect is not available"));
      }
    }
    else
      QMessageBox::warning(this,tr("Warning"),tr("Please select at least one way to connect"));
  }
  else if (ui.bbxButtons->standardButton(AButton) == QDialogButtonBox::Abort)
  {
    if (QMessageBox::question(this,tr("Cancel file transfer"),tr("Are you sure you want to cancel a file transfer?"),
      QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes)
    {
      FFileStream->cancelStream(tr("File transfer terminated by user"));
    }
  }
  else if (ui.bbxButtons->standardButton(AButton) == QDialogButtonBox::Close)
  {
    if (FFileStream->streamState()==IFileStream::Finished || FFileStream->streamState()==IFileStream::Canceled)
      delete FFileStream->instance();
    else if (FFileStream->streamKind()==IFileStream::SendFile && FFileStream->streamState()==IFileStream::Creating)
      delete FFileStream->instance();
    else
      close();
  }
}
