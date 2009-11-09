#ifndef STREAMDIALOG_H
#define STREAMDIALOG_H

#include <QDialog>
#include <QCheckBox>
#include <interfaces/ifiletransfer.h>
#include <interfaces/ifilestreamsmanager.h>
#include <interfaces/idatastreamsmanager.h>
#include <utils/jid.h>
#include <utils/iconstorage.h>
#include "ui_streamdialog.h"

class StreamDialog : 
  public QDialog
{
  Q_OBJECT;
public:
  StreamDialog(IDataStreamsManager *ADataManager, IFileStreamsManager *AFileManager, IFileTransfer *AFileTransfer, 
    IFileStream *AFileStream, QWidget *AParent = NULL);
  ~StreamDialog();
  inline IFileStream *stream() const;
  QList<QString> selectedMethods() const;
  void setSelectableMethods(const QList<QString> &AMethods);
signals:
  void dialogDestroyed();
protected:
  bool acceptFileName(const QString AFile);
  QString sizeName(qint64 ABytes) const;
  inline qint64 minPosition() const;
  inline qint64 maxPosition() const;
  inline qint64 curPosition() const;
  inline int curPercentPosition() const;
protected slots:
  void onStreamStateChanged();
  void onStreamSpeedChanged();
  void onStreamPropertiesChanged();
  void onStreamDestroyed();
  void onFileButtonClicked(bool);
  void onDialogButtonClicked(QAbstractButton *AButton);
  void onMethodSettingsChanged(int AIndex);
  void onMethodSettingsInserted(const QString &ASettingsNS, const QString &ASettingsName);
  void onMethodSettingsRemoved(const QString &ASettingsNS);
private:
  Ui::StreamDialogClass ui;
private:
  IFileStream *FFileStream;
  IFileTransfer *FFileTransfer;
  IFileStreamsManager *FFileManager;
  IDataStreamsManager *FDataManager;
private:
  QMap<QCheckBox *, QString> FMethodButtons;
};

#endif // STREAMDIALOG_H
