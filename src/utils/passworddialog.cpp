#include "passworddialog.h"

#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "logger.h"

PasswordDialog::PasswordDialog(QWidget *AParent) : QDialog(AParent)
{
	REPORT_VIEW;

	lblLabel = new QLabel(tr("Enter password:"),this);
	
	lnePassword = new QLineEdit(this);
	lnePassword->setEchoMode(QLineEdit::Password);
	connect(lnePassword,SIGNAL(textChanged(const QString &)),SIGNAL(passwordChanged(const QString &)));

	chbSavePassword = new QCheckBox(tr("Save password"),this);
	
	dbbButtons = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,Qt::Horizontal,this);
	connect(dbbButtons,SIGNAL(accepted()),SLOT(accept()));
	connect(dbbButtons,SIGNAL(rejected()),SLOT(reject()));

	QVBoxLayout *vltLayout = new QVBoxLayout(this);
	vltLayout->addWidget(lblLabel);
	vltLayout->addWidget(lnePassword);
	vltLayout->addWidget(chbSavePassword);
	vltLayout->addWidget(dbbButtons);
}

PasswordDialog::~PasswordDialog()
{

}

QString PasswordDialog::labelText() const
{
	return lblLabel->text();
}

void PasswordDialog::setLabelText(const QString &AText)
{
	lblLabel->setText(AText);
}

bool PasswordDialog::savePassword() const
{
	return chbSavePassword->isChecked();
}

void PasswordDialog::setSavePassword(bool ASave)
{
	chbSavePassword->setChecked(ASave);
}

void PasswordDialog::setSavePasswordVisible(bool AVisible)
{
	chbSavePassword->setVisible(AVisible);
}

QString PasswordDialog::password() const
{
	return lnePassword->text();
}

void PasswordDialog::setPassword(const QString &APassword)
{
	lnePassword->setText(APassword);
	lnePassword->selectAll();
}

QString PasswordDialog::okButtonText() const
{
	return dbbButtons->button(QDialogButtonBox::Ok)->text();
}

void PasswordDialog::setOkButtonText(const QString &AText)
{
	dbbButtons->button(QDialogButtonBox::Ok)->setText(AText);
}

bool PasswordDialog::okButtonEnabled() const
{
	return dbbButtons->button(QDialogButtonBox::Ok)->isEnabled();
}

void PasswordDialog::setOkButtonEnabled(bool AEnabled)
{
	dbbButtons->button(QDialogButtonBox::Ok)->setEnabled(AEnabled);
}

QString PasswordDialog::cancelButtonText() const
{
	return dbbButtons->button(QDialogButtonBox::Cancel)->text();
}

void PasswordDialog::setCancelButtonText(const QString &AText)
{
	dbbButtons->button(QDialogButtonBox::Cancel)->setText(AText);
}

QLineEdit::EchoMode PasswordDialog::echoMode() const
{
	return lnePassword->echoMode();
}

void PasswordDialog::setEchoMode(QLineEdit::EchoMode AMode)
{
	lnePassword->setEchoMode(AMode);
}
