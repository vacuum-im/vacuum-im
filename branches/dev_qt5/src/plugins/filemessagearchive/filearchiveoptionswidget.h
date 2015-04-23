#ifndef FILEARCHIVEOPTIONS_H
#define FILEARCHIVEOPTIONS_H

#include <interfaces/ipluginmanager.h>
#include <interfaces/ioptionsmanager.h>
#include "ui_filearchiveoptionswidget.h"

class FileArchiveOptionsWidget : 
	public QWidget,
	public IOptionsDialogWidget
{
	Q_OBJECT;
	Q_INTERFACES(IOptionsDialogWidget);
public:
	FileArchiveOptionsWidget(IPluginManager *APluginManager, QWidget *AParent = NULL);
	~FileArchiveOptionsWidget();
	virtual QWidget *instance() { return this; }
public slots:
	virtual void apply();
	virtual void reset();
signals:
	void modified();
	void childApply();
	void childReset();
protected slots:
	void onSelectLocationFolder();
private:
	Ui::FileArchiveOptionsClass ui;
private:
	IPluginManager *FPluginManager;
};

#endif // FILEARCHIVEOPTIONS_H
