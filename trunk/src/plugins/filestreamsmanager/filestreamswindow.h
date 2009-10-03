#ifndef FILESTREAMSWINDOW_H
#define FILESTREAMSWINDOW_H

#include <QLabel>
#include <QMainWindow>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include "../../definations/menuicons.h"
#include "../../definations/resources.h"
#include "../../definations/statusbargroups.h"
#include "../../interfaces/ifilestreamsmanager.h"
#include "../../interfaces/isettings.h"
#include "../../utils/toolbarchanger.h"
#include "../../utils/statusbarchanger.h"
#include "../../utils/iconstorage.h"
#include "ui_filestreamswindow.h"

class FileStreamsWindow : 
  public QMainWindow
{
  Q_OBJECT;
public:
  FileStreamsWindow(IFileStreamsManager *AManager, ISettings *ASettings, QWidget *AParent = NULL);
  ~FileStreamsWindow();
protected:
  void initialize();
  void appendStream(IFileStream *AStream);
  void updateStreamState(IFileStream *AStream);
  void updateStreamSpeed(IFileStream *AStream);
  void updateStreamProgress(IFileStream *AStream);
  void updateStreamProperties(IFileStream *AStream);
  void removeStream(IFileStream *AStream);
  int streamRow(const QString &AStreamId) const;
  QList<QStandardItem *> streamColumns(const QString &AStreamId) const;
  QString sizeName(qint64 ABytes) const;
protected slots:
  void onStreamCreated(IFileStream *AStream);
  void onStreamStateChanged();
  void onStreamSpeedChanged();
  void onStreamProgressChanged();
  void onStreamPropertiesChanged();
  void onStreamDestroyed(IFileStream *AStream);
  void onTableIndexActivated(const QModelIndex &AIndex);
  void onUpdateStatusBar();
private:
  Ui::FileStreamsWindowClass ui;
private:
  ISettings *FSettings;
  IFileStreamsManager *FManager;
private:
  QLabel *FStreamsCount;
  QLabel *FStreamsSpeedIn;
  QLabel *FStreamsSpeedOut;
private:
  ToolBarChanger *FToolBarChanger;
  StatusBarChanger *FStatusBarChanger;
  QSortFilterProxyModel FProxy;
  QStandardItemModel FStreamsModel;
};

#endif // FILESTREAMSWINDOW_H
