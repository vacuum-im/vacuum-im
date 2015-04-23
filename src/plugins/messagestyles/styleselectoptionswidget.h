#ifndef STYLESELECTOPTIONSWIDGET_H
#define STYLESELECTOPTIONSWIDGET_H

#include <QLabel>
#include <QWidget>
#include <QComboBox>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/imessagestylemanager.h>
#include <utils/message.h>

class StyleSelectOptionsWidget : 
	public QWidget,
	public IOptionsDialogWidget
{
	Q_OBJECT;
	Q_INTERFACES(IOptionsDialogWidget);
public:
	StyleSelectOptionsWidget(IMessageStyleManager *AMessageStyleManager, int AMessageType, QWidget *AParent);
	virtual QWidget *instance() { return this; }
public slots:
	virtual void apply();
	virtual void reset();
signals:
	void modified();
	void childApply();
	void childReset();
protected slots:
	void onEditStyleButtonClicked();
private:
	QLabel *lblType;
	QComboBox *cmbStyle;
private:
	int FMessageType;
	IMessageStyleManager *FMessageStyleManager;
};

#endif // STYLESELECTOPTIONSWIDGET_H
