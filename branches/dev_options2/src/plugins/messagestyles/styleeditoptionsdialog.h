#ifndef STYLEEDITOPTIONSDIALOG_H
#define STYLEEDITOPTIONSDIALOG_H

#include <QDialog>
#include <interfaces/iroster.h>
#include <interfaces/ipresence.h>
#include <interfaces/imessagestylemanager.h>
#include "ui_styleeditoptionsdialog.h"

class StyleEditOptionsDialog :
	public QDialog
{
	Q_OBJECT;
public:
	StyleEditOptionsDialog(IMessageStyleManager *AMessageStyleManager, const OptionsNode &AStyleNode, QWidget *AParent = NULL);
	~StyleEditOptionsDialog();
	virtual QWidget *instance() { return this; }
public slots: 
	virtual void accept();
protected:
	void createViewContent();
protected slots:
	void startStyleViewUpdate();
	void onUpdateStyleView();
private:
	Ui::StyleEditOptionsDialogClass ui;
private:
	QWidget *FStyleView;
	IMessageStyle *FStyle;
	IMessageStyleEngine *FStyleEngine;
	IOptionsDialogWidget *FStyleSettings;
	IMessageStyleManager *FMessageStyleManager;
private:
	int FMessageType;
	QString FContext;
	QString FEngineId;
	QString FStyleId;
	bool FUpdateStarted;
};

#endif // STYLEEDITOPTIONSDIALOG_H
