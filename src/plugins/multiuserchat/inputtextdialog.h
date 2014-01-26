#ifndef INPUTTEXTDIALOG_H
#define INPUTTEXTDIALOG_H

#include <QDialog>
#include "ui_inputtextdialog.h"

class InputTextDialog :
			public QDialog
{
	Q_OBJECT;
public:
	InputTextDialog(QWidget *AParent,const QString &ATitle,  const QString &ALabel, QString &AText);
	~InputTextDialog();
private:
	Ui::InputTextDialogClass ui;
protected slots:
	void onDialogButtonClicked(QAbstractButton *AButton);
private:
	QString &FText;
};

#endif // INPUTTEXTDIALOG_H
