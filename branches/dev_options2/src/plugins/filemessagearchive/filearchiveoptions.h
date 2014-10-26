#ifndef FILEARCHIVEOPTIONS_H
#define FILEARCHIVEOPTIONS_H

#include <interfaces/ipluginmanager.h>
#include <interfaces/ioptionsmanager.h>
#include "ui_filearchiveoptions.h"

class FileArchiveOptions : 
	public QWidget,
	public IOptionsWidget
{
	Q_OBJECT;
	Q_INTERFACES(IOptionsWidget);
public:
	FileArchiveOptions(IPluginManager *APluginManager, QWidget *AParent = NULL);
	~FileArchiveOptions();
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
