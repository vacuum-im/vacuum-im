#ifndef FILETRANSFEROPTIONS_H
#define FILETRANSFEROPTIONS_H

#include <QWidget>
#include <interfaces/ifiletransfer.h>
#include "ui_filetransferoptions.h"

class FileTransferOptions : 
  public QWidget
{
  Q_OBJECT;
public:
  FileTransferOptions(IFileTransfer *AFileTransfer, QWidget *AParent = NULL);
  ~FileTransferOptions();
public slots:
  void apply();
signals:
  void optionsAccepted();
private:
  Ui::FileTransferOptionsClass ui;
private:
  IFileTransfer *FFileTransfer;
};

#endif // FILETRANSFEROPTIONS_H
