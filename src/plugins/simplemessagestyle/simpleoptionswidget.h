#ifndef SIMPLEOPTIONSWIDGET_H
#define SIMPLEOPTIONSWIDGET_H

#include <QWidget>
#include <interfaces/imessagestylemanager.h>
#include <interfaces/ioptionsmanager.h>
#include "simplemessagestyleengine.h"
#include "ui_simpleoptionswidget.h"

class SimpleMessageStyleEngine;

class SimpleOptionsWidget :
	public QWidget,
	public IOptionsDialogWidget
{
	Q_OBJECT;
	Q_INTERFACES(IOptionsDialogWidget);
public:
	SimpleOptionsWidget(SimpleMessageStyleEngine *AEngine, const OptionsNode &ANode, QWidget *AParent);
	~SimpleOptionsWidget();
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
	void onImageChangeClicked();
	void onImageResetClicked();
private:
	Ui::SimpleOptionsWidgetClass ui;
private:
	OptionsNode FStyleNode;
	IMessageStyleOptions FStyleOptions;
	SimpleMessageStyleEngine *FStyleEngine;
};

#endif // SIMPLEOPTIONSWIDGET_H
