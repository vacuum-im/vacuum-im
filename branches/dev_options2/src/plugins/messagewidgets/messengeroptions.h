#ifndef MESSENGEROPTIONS_H
#define MESSENGEROPTIONS_H

#include <interfaces/imessagewidgets.h>
#include <interfaces/ioptionsmanager.h>
#include "ui_messengeroptions.h"

class MessengerOptions :
	public QWidget,
	public IOptionsWidget
{
	Q_OBJECT;
	Q_INTERFACES(IOptionsWidget);
public:
	MessengerOptions(IMessageWidgets *AMessageWidgets, QWidget *AParent);
	~MessengerOptions();
	virtual QWidget* instance() { return this; }
public slots:
	virtual void apply();
	virtual void reset();
signals:
	void modified();
	void childApply();
	void childReset();
private:
	Ui::MessengerOptionsClass ui;
private:
	IMessageWidgets *FMessageWidgets;
};

#endif // MESSENGEROPTIONS_H
