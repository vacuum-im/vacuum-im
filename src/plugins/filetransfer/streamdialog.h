#ifndef STREAMDIALOG_H
#define STREAMDIALOG_H

#include <QDialog>
#include <QCheckBox>
#include "../../interfaces/ifilestreamsmanager.h"
#include "../../interfaces/idatastreamsmanager.h"
#include "../../utils/jid.h"
#include "../../utils/iconstorage.h"
#include "ui_streamdialog.h"

class StreamDialog : 
  public QDialog
{
  Q_OBJECT;
public:
  StreamDialog(IDataStreamsManager *ADataManager, IFileStreamsManager *AFileManager, IFileStream *AFileStream, QWidget *AParent = NULL);
  ~StreamDialog();
  inline IFileStream *stream() const;
  QList<QString> selectedMethods() const;
  void setSelectableMethods(const QList<QString> &AMethods);
signals:
  void dialogDestroyed();
protected:
  bool acceptFileName(const QString AFile);
  QString sizeName(qint64 ABytes) const;
  inline qint64 curPosition() const;
  inline qint64 maxPosition() const;
protected slots:
  void onStreamStateChanged();
  void onStreamSpeedChanged();
  void onStreamPropertiesChanged();
  void onStreamDestroyed();
  void onFileButtonClicked(bool);
  void onDialogButtonClicked(QAbstractButton *AButton);
private:
  Ui::StreamDialogClass ui;
private:
  IFileStream *FFileStream;
  IFileStreamsManager *FFileManager;
  IDataStreamsManager *FDataManager;
private:
  QMap<QCheckBox *, QString> FMethodButtons;
};

#endif // STREAMDIALOG_H
