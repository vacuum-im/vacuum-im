#ifndef INBANDOPTIONS_H
#define INBANDOPTIONS_H

#include <QWidget>
#include <interfaces/iinbandstreams.h>
#include <interfaces/ioptionsmanager.h>
#include "ui_inbandoptionswidget.h"

class InBandOptionsWidget :
	public QWidget,
	public IOptionsDialogWidget
{
	Q_OBJECT;
	Q_INTERFACES(IOptionsDialogWidget);
public:
	InBandOptionsWidget(IInBandStreams *AInBandStreams, const OptionsNode &ANode, QWidget *AParent);
	virtual QWidget *instance() { return this; }
public slots:
	virtual void apply();
	virtual void reset();
signals:
	void modified();
	void childApply();
	void childReset();
private:
	Ui::InBandOptionsWidgetClass ui;
private:
	OptionsNode FOptionsNode;
	IInBandStreams *FInBandStreams;
};

#endif // INBANDOPTIONS_H
