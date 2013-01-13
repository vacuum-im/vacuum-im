#include "logindialog.h"

#include <QMessageBox>

LoginDialog::LoginDialog(IOptionsManager *AOptionsManager, QWidget *AParent) : QDialog(AParent)
{
  ui.setupUi(this);
  setWindowModality(Qt::WindowModal);
  setAttribute(Qt::WA_DeleteOnClose, true);

  FManager = AOptionsManager;

  ui.cmbProfile->addItems(FManager->profiles());
  ui.cmbProfile->setCurrentIndex(ui.cmbProfile->findText(!FManager->currentProfile().isEmpty() ? FManager->currentProfile() : FManager->lastActiveProfile()));

  connect(FManager->instance(),SIGNAL(profileAdded(const QString &)),SLOT(onProfileAdded(const QString &)));
  connect(FManager->instance(),SIGNAL(profileRenamed(const QString &, const QString &)),
    SLOT(onProfileRenamed(const QString &, const QString &)));
  connect(FManager->instance(),SIGNAL(profileRemoved(const QString &)),SLOT(onProfileRemoved(const QString &)));

  connect(ui.pbtProfiles,SIGNAL(clicked(bool)),SLOT(onEditProfilesClicked(bool)));

  connect(ui.dbbButtons,SIGNAL(accepted()),SLOT(onDialogAccepted()));
  connect(ui.dbbButtons,SIGNAL(rejected()),SLOT(onDialogRejected()));
}

LoginDialog::~LoginDialog()
{

}

void LoginDialog::onProfileAdded(const QString &AProfile)
{
  ui.cmbProfile->addItem(AProfile);
}

void LoginDialog::onProfileRenamed(const QString &AProfile,const QString &ANewName)
{
  ui.cmbProfile->setItemText(ui.cmbProfile->findText(AProfile), ANewName);
}

void LoginDialog::onProfileRemoved(const QString &AProfile)
{
  ui.cmbProfile->removeItem(ui.cmbProfile->findText(AProfile));
}

void LoginDialog::onEditProfilesClicked(bool)
{
  FManager->showEditProfilesDialog(this);
}

void LoginDialog::onDialogAccepted()
{
  QString profile = ui.cmbProfile->currentText();
  QString password = ui.lnePassword->text();
  if (FManager->checkProfilePassword(profile, password))
  {
    if (FManager->setCurrentProfile(profile, password))
    {
      accept();
    }
    else
    {
      QMessageBox::warning(this,tr("Profile Blocked"), tr("This profile is now blocked by another program"));
    }
  }
  else
  {
    QMessageBox::warning(this,tr("Wrong Password"), tr("Entered profile password is not correct"));
  }
}

void LoginDialog::onDialogRejected()
{
  reject();
}
