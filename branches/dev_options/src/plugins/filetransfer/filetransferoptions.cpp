#include "filetransferoptions.h"

FileTransferOptions::FileTransferOptions(IFileTransfer *AFileTransfer, QWidget *AParent) : QWidget(AParent)
{
  ui.setupUi(this);
  FFileTransfer = AFileTransfer;

  ui.chbAutoReceive->setChecked(FFileTransfer->autoReceive());
  ui.chbHideDialog->setChecked(FFileTransfer->hideDialogWhenStarted());
  ui.chbRemoveFinished->setChecked(FFileTransfer->removeTransferWhenFinished());
}

FileTransferOptions::~FileTransferOptions()
{

}

void FileTransferOptions::apply()
{
  FFileTransfer->setAutoReceive(ui.chbAutoReceive->isChecked());
  FFileTransfer->setHideDialogWhenStarted(ui.chbHideDialog->isChecked());
  FFileTransfer->setRemoveTransferWhenFinished(ui.chbRemoveFinished->isChecked());
  emit optionsAccepted();
}