#ifndef FILESTREAMSOPTIONS_H
#define FILESTREAMSOPTIONS_H

#include <QWidget>
#include <QCheckBox>
#include <interfaces/ifilestreamsmanager.h>
#include <interfaces/idatastreamsmanager.h>
#include "ui_filestreamsoptions.h"

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
  void onMethodButtonToggled(bool ACkecked);
private:
  Ui::FileStreamsOptionsClass ui;
private:
  IDataStreamsManager *FDataManager;
  IFileStreamsManager *FFileManager;
private:
  QMap<QCheckBox *, QString> FMethods;
};

#endif // FILESTREAMSOPTIONS_H
