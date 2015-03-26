#ifndef FILESTREAMSOPTIONS_H
#define FILESTREAMSOPTIONS_H

#include <QWidget>
#include <QCheckBox>
#include <interfaces/ifilestreamsmanager.h>
#include <interfaces/idatastreamsmanager.h>
#include <interfaces/ioptionsmanager.h>
#include "ui_filestreamsoptionswidget.h"

class FileStreamsOptionsWidget :
	public QWidget,
	public IOptionsDialogWidget
{
	Q_OBJECT;
	Q_INTERFACES(IOptionsDialogWidget);
public:
	FileStreamsOptionsWidget(IFileStreamsManager *AFileManager, QWidget *AParent);
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
private:
	Ui::FileStreamsOptionsWidgetClass ui;
private:
	IFileStreamsManager *FFileManager;
};

#endif // FILESTREAMSOPTIONS_H
