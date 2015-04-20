#ifndef STREAMDIALOG_H
#define STREAMDIALOG_H

#include <QDialog>
#include <QCheckBox>
#include <interfaces/ifiletransfer.h>
#include <interfaces/ifilestreamsmanager.h>
#include <interfaces/idatastreamsmanager.h>
#include "ui_streamdialog.h"

class StreamDialog :
	public QDialog
{
	Q_OBJECT;
public:
	StreamDialog(IDataStreamsManager *ADataManager, IFileStreamsManager *AFileManager, IFileTransfer *AFileTransfer, IFileStream *AFileStream, QWidget *AParent = NULL);
	~StreamDialog();
	IFileStream *stream() const;
	void setContactName(const QString &AName);
	QList<QString> selectedMethods() const;
	void setSelectableMethods(const QList<QString> &AMethods);
signals:
	void dialogDestroyed();
protected:
	qint64 minPosition() const;
	qint64 maxPosition() const;
	qint64 curPosition() const;
	int curPercentPosition() const;
	bool acceptFileName(const QString &AFile);
	QString sizeName(qint64 ABytes) const;
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
	IFileTransfer *FFileTransfer;
	IFileStreamsManager *FFileManager;
	IDataStreamsManager *FDataManager;
};

#endif // STREAMDIALOG_H
