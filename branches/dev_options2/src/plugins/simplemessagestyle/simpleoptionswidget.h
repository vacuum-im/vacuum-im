#ifndef SIMPLEOPTIONSWIDGET_H
#define SIMPLEOPTIONSWIDGET_H

#include <QWidget>
#include <interfaces/imessagestyles.h>
#include <interfaces/ioptionsmanager.h>
#include "simplemessagestyleplugin.h"
#include "ui_simpleoptionswidget.h"

class SimpleMessageStylePlugin;

class SimpleOptionsWidget :
	public QWidget,
	public IOptionsWidget
{
	Q_OBJECT;
	Q_INTERFACES(IOptionsWidget);
public:
	SimpleOptionsWidget(SimpleMessageStylePlugin *APlugin, const OptionsNode &ANode, int AMessageType, QWidget *AParent = NULL);
	~SimpleOptionsWidget();
	virtual QWidget *instance() { return this; }
public slots:
	virtual void apply(OptionsNode ANode);
	virtual void apply();
	virtual void reset();
signals:
	void modified();
	void childApply();
	void childReset();
public:
	IMessageStyleOptions styleOptions() const;
protected:
	void updateOptionsWidgets();
protected slots:
	void onStyleChanged(int AIndex);
	void onVariantChanged(int AIndex);
	void onSetFontClicked();
	void onDefaultFontClicked();
	void onBackgroundColorChanged(int AIndex);
	void onSetImageClicked();
	void onDefaultImageClicked();
	void onAnimationEnableToggled(int AState);
private:
	Ui::SimpleOptionsWidgetClass ui;
private:
	SimpleMessageStylePlugin *FStylePlugin;
private:
	int FMessageType;
	OptionsNode FOptions;
	IMessageStyleOptions FStyleOptions;
};

#endif // SIMPLEOPTIONSWIDGET_H
