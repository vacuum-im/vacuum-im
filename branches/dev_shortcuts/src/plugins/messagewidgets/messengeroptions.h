#ifndef MESSENGEROPTIONS_H
#define MESSENGEROPTIONS_H

#include <QShortcut>
#include <definitions/optionvalues.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/ioptionsmanager.h>
#include <utils/options.h>
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
protected:
	virtual bool eventFilter(QObject *AWatched, QEvent *AEvent);
private:
	Ui::MessengerOptionsClass ui;
private:
	IMessageWidgets *FMessageWidgets;
private:
	QKeySequence FSendKey;
};

#endif // MESSENGEROPTIONS_H
