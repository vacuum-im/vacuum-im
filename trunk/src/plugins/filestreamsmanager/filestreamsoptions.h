#ifndef FILESTREAMSOPTIONS_H
#define FILESTREAMSOPTIONS_H

#include <QWidget>
#include "ui_filestreamsoptions.h"
#include "../../interfaces/ifilestreamsmanager.h"
#include "../../interfaces/idatastreamsmanager.h"

class FileStreamsOptions : 
  public QWidget
{
  Q_OBJECT;
public:
  FileStreamsOptions(IDataStreamsManager *ADataManager, IFileStreamsManager *AFileManager, QWidget *AParent=NULL);
  ~FileStreamsOptions();
public slots:
  void apply();
signals:
  void optionsAccepted();
protected slots:
  void onDirectoryButtonClicked();
private:
  Ui::FileStreamsOptionsClass ui;
private:
  IDataStreamsManager *FDataManager;
  IFileStreamsManager *FFileManager;
};

#endif // FILESTREAMSOPTIONS_H
