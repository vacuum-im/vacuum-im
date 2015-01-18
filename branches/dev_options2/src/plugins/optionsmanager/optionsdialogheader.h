#ifndef OPTIONSHEADER_H
#define OPTIONSHEADER_H

#include <QLabel>
#include <definitions/resources.h>
#include <interfaces/ioptionsmanager.h>
#include <utils/iconstorage.h>

class OptionsDialogHeader : 
	public QLabel,
	public IOptionsDialogWidget
{
	Q_OBJECT;
	Q_INTERFACES(IOptionsDialogWidget);
public:
	OptionsDialogHeader(const QString &ACaption, QWidget *AParent);
	~OptionsDialogHeader();
	virtual QWidget *instance() { return this; }
public slots:
	virtual void apply();
	virtual void reset();
signals:
	void modified();
	void childApply();
	void childReset();
};

#endif // OPTIONSHEADER_H
