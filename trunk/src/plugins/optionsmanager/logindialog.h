#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <interfaces/ioptionsmanager.h>
#include "ui_logindialog.h"

class LoginDialog :
			public QDialog
{
	Q_OBJECT;
public:
	LoginDialog(IOptionsManager *AOptionsManager, QWidget *AParent = NULL);
	~LoginDialog();
protected slots:
	void onProfileAdded(const QString &AProfile);
	void onProfileRenamed(const QString &AProfile,const QString &ANewName);
	void onProfileRemoved(const QString &AProfile);
	void onEditProfilesClicked(bool);
	void onDialogAccepted();
	void onDialogRejected();
private:
	Ui::LoginDialogClass ui;
private:
	IOptionsManager *FManager;
};

#endif // LOGINDIALOG_H
