#ifndef PASSWORDDIALOG_H
#define PASSWORDDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QDialogButtonBox>
#include "utilsexport.h"

class UTILS_EXPORT PasswordDialog :
	public QDialog
{
	Q_OBJECT;
public:
	PasswordDialog(QWidget *AParent = NULL);
	~PasswordDialog();
	QString labelText() const;
	void setLabelText(const QString &AText);
	bool savePassword() const;
	void setSavePassword(bool ASave);
	void setSavePasswordVisible(bool AVisible);
	QString password() const;
	void setPassword(const QString &APassword);
	QString okButtonText() const;
	void setOkButtonText(const QString &AText);
	bool okButtonEnabled() const;
	void setOkButtonEnabled(bool AEnabled);
	QString cancelButtonText() const;
	void setCancelButtonText(const QString &AText);
	QLineEdit::EchoMode echoMode() const;
	void setEchoMode(QLineEdit::EchoMode AMode);
signals:
	void passwordChanged(const QString &APassword);
private:
	QLabel *lblLabel;
	QLineEdit *lnePassword;
	QCheckBox *chbSavePassword;
	QDialogButtonBox *dbbButtons;
};

#endif // PASSWORDDIALOG_H
