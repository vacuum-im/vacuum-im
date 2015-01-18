#ifndef FILESTREAMSOPTIONS_H
#define FILESTREAMSOPTIONS_H

#include <QWidget>
#include <QCheckBox>
#include <interfaces/ifilestreamsmanager.h>
#include <interfaces/idatastreamsmanager.h>
#include <interfaces/ioptionsmanager.h>
#include "ui_filestreamsoptions.h"

class FileStreamsOptions :
	public QWidget,
	public IOptionsDialogWidget
{
	Q_OBJECT;
	Q_INTERFACES(IOptionsDialogWidget);
public:
	FileStreamsOptions(IDataStreamsManager *ADataManager, IFileStreamsManager *AFileManager, QWidget *AParent);
	~FileStreamsOptions();
	virtual QWidget* instance() { return this; }
public slots:
	virtual void apply();
	virtual void reset();
signals:
	void modified();
	void childApply();
	void childReset();
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
