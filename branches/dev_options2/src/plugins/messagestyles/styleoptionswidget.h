#ifndef STYLEOPTIONSWIDGET_H
#define STYLEOPTIONSWIDGET_H

#include <interfaces/imessagestyles.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/ipresence.h>
#include <interfaces/iroster.h>
#include "ui_styleoptionswidget.h"

class StyleOptionsWidget :
	public QWidget,
	public IOptionsDialogWidget
{
	Q_OBJECT;
	Q_INTERFACES(IOptionsDialogWidget);
public:
	StyleOptionsWidget(IMessageStyles *AMessageStyles, QWidget *AParent);
	~StyleOptionsWidget();
	virtual QWidget *instance() { return this; }
public slots:
	void apply();
	void reset();
signals:
	void modified();
	void childApply();
	void childReset();
public slots:
	void startStyleViewUpdate();
protected:
	void createViewContent();
	QWidget *updateActiveSettings();
protected slots:
	void onUpdateStyleView();
	void onMessageTypeChanged(int AIndex);
	void onStyleEngineChanged(int AIndex);
private:
	Ui::StyleOptionsWidgetClass ui;
private:
	IMessageStyles *FMessageStyles;
private:
	bool FUpdateStarted;
	QWidget *FActiveView;
	IMessageStyle *FActiveStyle;
	IOptionsDialogWidget *FActiveSettings;
	QMap<int, QString> FMessagePlugin;
	QMap<int, IOptionsDialogWidget *> FMessageWidget;
};

#endif // STYLEOPTIONSWIDGET_H
