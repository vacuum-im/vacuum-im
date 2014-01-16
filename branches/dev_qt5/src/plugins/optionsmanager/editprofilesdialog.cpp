#include "editprofilesdialog.h"

#include <QMessageBox>
#include <QInputDialog>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <utils/iconstorage.h>
#include <utils/logger.h>

EditProfilesDialog::EditProfilesDialog(IOptionsManager *AOptionsManager, QWidget *AParent) : QDialog(AParent)
{
	REPORT_VIEW;
	ui.setupUi(this);
	setWindowModality(Qt::WindowModal);
	setAttribute(Qt::WA_DeleteOnClose,true);
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this,MNI_OPTIONS_EDIT_PROFILES,0,0,"windowIcon");

	FManager = AOptionsManager;
	ui.lstProfiles->addItems(FManager->profiles());

	connect(FManager->instance(),SIGNAL(profileAdded(const QString &)),SLOT(onProfileAdded(const QString &)));
	connect(FManager->instance(),SIGNAL(profileRenamed(const QString &, const QString &)),SLOT(onProfileRenamed(const QString &, const QString &)));
	connect(FManager->instance(),SIGNAL(profileRemoved(const QString &)),SLOT(onProfileRemoved(const QString &)));

	connect(ui.pbtAdd,SIGNAL(clicked()),SLOT(onAddProfileClicked()));
	connect(ui.pbtPassword,SIGNAL(clicked()),SLOT(onPasswordProfileClicked()));
	connect(ui.pbtRename,SIGNAL(clicked()),SLOT(onRenameProfileClicked()));
	connect(ui.pbtDelete,SIGNAL(clicked()),SLOT(onRemoveProfileClicked()));

	connect(ui.pbtClose, SIGNAL(clicked()),SLOT(accept()));
}

EditProfilesDialog::~EditProfilesDialog()
{

}

void EditProfilesDialog::onAddProfileClicked()
{
	bool ok;
	QString profile = QInputDialog::getText(this,tr("New Profile"),tr("Enter profile name:"),QLineEdit::Normal,QString::null,&ok);
	if (ok && !profile.isEmpty())
	{
		QString password = QInputDialog::getText(this,tr("Profile Password"),tr("Enter profile password:"),QLineEdit::Password,QString::null,&ok);
		if (ok && password==QInputDialog::getText(this,tr("Confirm Password"),tr("Reenter password:"),QLineEdit::Password,QString::null,&ok))
		{
			if (!FManager->addProfile(profile,password))
				QMessageBox::warning(this,tr("Error"),tr("Could not create profile, maybe this profile already exists"));
		}
		else if (ok)
		{
			QMessageBox::warning(this,tr("Error"),tr("Passwords did not match"));
		}
	}
}

void EditProfilesDialog::onPasswordProfileClicked()
{
	QListWidgetItem *listItem = ui.lstProfiles->selectedItems().value(0);
	if (listItem)
	{
		bool ok;
		QString profile = listItem->text();
		QString oldPassword = QInputDialog::getText(this,tr("Profile Password"),tr("Enter current profile password:"),QLineEdit::Password,QString::null,&ok);
		if (ok && FManager->checkProfilePassword(profile,oldPassword))
		{
			QString newPassword = QInputDialog::getText(this,tr("Profile Password"),tr("Enter new profile password:"),QLineEdit::Password,QString::null,&ok);
			if (ok && newPassword==QInputDialog::getText(this,tr("Confirm Password"),tr("Reenter password:"),QLineEdit::Password,QString::null,&ok))
			{
				if (!FManager->changeProfilePassword(profile,oldPassword,newPassword))
					QMessageBox::warning(this,tr("Error"),tr("Failed to change profile password"));
			}
			else if (ok)
			{
				QMessageBox::warning(this,tr("Error"),tr("Passwords did not match"));
			}
		}
		else if (ok)
		{
			QMessageBox::warning(this,tr("Error"),tr("Entered password is not valid"));
		}
	}
}

void EditProfilesDialog::onRenameProfileClicked()
{
	QListWidgetItem *listItem = ui.lstProfiles->selectedItems().value(0);
	if (listItem)
	{
		bool ok;
		QString profile = listItem->text();
		QString newname = QInputDialog::getText(this,tr("Rename Profile"),tr("Enter new name for profile:"),QLineEdit::Normal,QString::null,&ok);
		if (ok && !newname.isEmpty())
		{
			if (!FManager->renameProfile(profile, newname))
				QMessageBox::warning(this,tr("Error"),tr("Failed to rename profile"));
		}
	}
}

void EditProfilesDialog::onRemoveProfileClicked()
{
	QListWidgetItem *listItem = ui.lstProfiles->selectedItems().value(0);
	if (listItem)
	{
		QString profile = listItem->text();
		if (QMessageBox::question(this,tr("Remove Profile"),tr("Are you sure you want to delete profile '<b>%1</b>'?").arg(profile.toHtmlEscaped()),QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes)
		{
			if (!FManager->removeProfile(profile))
				QMessageBox::warning(this,tr("Error"),tr("Failed to remove profile"));
		}
	}
}

void EditProfilesDialog::onProfileAdded(const QString &AProfile)
{
	ui.lstProfiles->addItem(AProfile);
}

void EditProfilesDialog::onProfileRenamed( const QString &AProfile,const QString &ANewName )
{
	QListWidgetItem *listItem = ui.lstProfiles->findItems(AProfile,Qt::MatchExactly).value(0);
	if (listItem)
		listItem->setText(ANewName);
}

void EditProfilesDialog::onProfileRemoved(const QString &AProfile)
{
	qDeleteAll(ui.lstProfiles->findItems(AProfile,Qt::MatchExactly));
}
