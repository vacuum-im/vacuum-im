#ifndef EDITPROFILESDIALOG_H
#define EDITPROFILESDIALOG_H

#include <QSet>
#include <QDialog>
#include <interfaces/ioptionsmanager.h>
#include "ui_editprofilesdialog.h"

class EditProfilesDialog :
	public QDialog
{
	Q_OBJECT;
public:
	EditProfilesDialog(IOptionsManager *AOptionsManager, QWidget *AParent);
	~EditProfilesDialog();
protected slots:
	void onAddProfileClicked();
	void onPasswordProfileClicked();
	void onRenameProfileClicked();
	void onRemoveProfileClicked();
	void onProfileAdded(const QString &AProfile);
	void onProfileRenamed(const QString &AProfile,const QString &ANewName);
	void onProfileRemoved(const QString &AProfile);
private:
	Ui::EditProfilesDialogClass ui;
private:
	IOptionsManager *FManager;
};

#endif // EDITPROFILESDIALOG_H
