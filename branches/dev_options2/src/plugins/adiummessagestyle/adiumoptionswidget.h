#ifndef ADIUMOPTIONSWIDGET_H
#define ADIUMOPTIONSWIDGET_H

#include <QWidget>
#include <interfaces/imessagestylemanager.h>
#include <interfaces/ioptionsmanager.h>
#include "adiummessagestyleengine.h"
#include "ui_adiumoptionswidget.h"

class AdiumMessageStyleEngine;

class AdiumOptionsWidget :
	public QWidget,
	public IOptionsDialogWidget
{
	Q_OBJECT;
	Q_INTERFACES(IOptionsDialogWidget);
public:
	AdiumOptionsWidget(AdiumMessageStyleEngine *AEngine, const OptionsNode &ANode, QWidget *AParent);
	~AdiumOptionsWidget();
	virtual QWidget *instance() { return this; }
	IMessageStyleOptions styleOptions() const;
public slots:
	virtual void apply();
	virtual void reset();
signals:
	void modified();
	void childApply();
	void childReset();
protected:
	void updateOptionsWidgets();
protected slots:
	void onVariantChanged(int AIndex);
	void onFontChangeClicked();
	void onFontResetClicked();
	void onColorChanged(int AIndex);
	void onImageLayoutChanged(int AIndex);
	void onImageChangeClicked();
	void onImageResetClicked();
private:
	Ui::AdiumOptionsWidgetClass ui;
private:
	OptionsNode FStyleNode;
	IMessageStyleOptions FStyleOptions;
	AdiumMessageStyleEngine *FStyleEngine;
};

#endif // ADIUMOPTIONSWIDGET_H
