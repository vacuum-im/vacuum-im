#include "profiledialog.h"

#include <QInputDialog>
#include <QMessageBox>

ProfileDialog::ProfileDialog(ISettingsPlugin *ASettingsPlugin)
{
  ui.setupUi(this);
  setAttribute(Qt::WA_DeleteOnClose);
  connect(ui.pbtNew,SIGNAL(clicked()),SLOT(onNewProfileClicked()));
  connect(ui.pbtRename,SIGNAL(clicked()),SLOT(onRenameProfileClicked()));
  connect(ui.pbtDelete,SIGNAL(clicked()),SLOT(onRemoveProfileClicked()));
  connect(ui.btbButtons, SIGNAL(accepted()),SLOT(onAccepted()));

  FSettingsPlugin = ASettingsPlugin;
  connect(FSettingsPlugin->instance(),SIGNAL(profileAdded(const QString &)),SLOT(onProfileAdded(const QString &)));
  connect(FSettingsPlugin->instance(),SIGNAL(profileRenamed(const QString &, const QString &)),
    SLOT(onProfileRenamed(const QString &, const QString &)));
  connect(FSettingsPlugin->instance(),SIGNAL(profileRemoved(const QString &)),SLOT(onProfileRemoved(const QString &)));

  FOldProfiles = FSettingsPlugin->profiles().toSet();
  FNewProfiles = FOldProfiles;
  ui.cmbProfiles->addItems(FOldProfiles.toList());
  ui.lstProfiles->addItems(FOldProfiles.toList());
  ui.cmbProfiles->setCurrentIndex(ui.cmbProfiles->findText(FSettingsPlugin->profile()));
  updateDialog();
}

ProfileDialog::~ProfileDialog()
{

}

void ProfileDialog::addProfile(const QString &AProfile)
{
  FNewProfiles += AProfile;
  ui.cmbProfiles->addItem(AProfile);
  ui.lstProfiles->addItem(AProfile);
  updateDialog();
}

void ProfileDialog::renameProfile(const QString &AProfileFrom, const QString &AProfileTo)
{
  if (!FNewProfiles.contains(AProfileTo))
  {
    FNewProfiles -= AProfileFrom;
    FNewProfiles += AProfileTo;

    QListWidgetItem *listItem = ui.lstProfiles->findItems(AProfileFrom,Qt::MatchExactly).value(0);
    listItem->setText(AProfileTo);

    ui.cmbProfiles->setItemText(ui.cmbProfiles->findText(AProfileFrom),AProfileTo);
  }
  else
    removeProfile(AProfileFrom);
}

void ProfileDialog::removeProfile(const QString &AProfile)
{
  FNewProfiles -= AProfile;
  int cmbItem = ui.cmbProfiles->findText(AProfile);
  if (cmbItem != -1)
    ui.cmbProfiles->removeItem(cmbItem);
  qDeleteAll(ui.lstProfiles->findItems(AProfile,Qt::MatchExactly));
  updateDialog();
}

void ProfileDialog::updateDialog()
{
  ui.pbtDelete->setEnabled(FNewProfiles.count() > 1);
}

void ProfileDialog::onProfileAdded(const QString &AProfile)
{
  FOldProfiles += AProfile;
  FRenamedProfiles.remove(FRenamedProfiles.key(AProfile));
  addProfile(AProfile);
}

void ProfileDialog::onProfileRenamed(const QString &AProfileFrom, const QString &AProfileTo)
{
  FOldProfiles -= AProfileFrom;
  FOldProfiles += AProfileTo;
  if (FRenamedProfiles.contains(AProfileFrom))
  {
    if (FRenamedProfiles.value(AProfileFrom) != AProfileTo)
      FRenamedProfiles.insert(AProfileTo,FRenamedProfiles.value(AProfileFrom));
    FRenamedProfiles.remove(AProfileFrom);
  }
  renameProfile(AProfileFrom,AProfileTo);
}

void ProfileDialog::onProfileRemoved(const QString &AProfile)
{
  FOldProfiles -= AProfile;
  FRenamedProfiles.remove(AProfile);
  removeProfile(AProfile);
}

void ProfileDialog::onNewProfileClicked()
{
  QString profileName = QInputDialog::getText(this,tr("Creating new profile"),tr("Enter profile name:"));
  if (!profileName.isEmpty() && !FNewProfiles.contains(profileName))
     addProfile(profileName);
}

void ProfileDialog::onRenameProfileClicked()
{
  QListWidgetItem *listItem = ui.lstProfiles->selectedItems().value(0);
  if (listItem)
  {
    QString profileTo = QInputDialog::getText(this,tr("Renaming profile"),tr("Enter new name for profile:"),QLineEdit::Normal,listItem->text());
    if (!FNewProfiles.contains(profileTo))
    {
      if (!profileTo.isEmpty())
      {
        QString profileFrom = FRenamedProfiles.key(listItem->text(),listItem->text());
        if (FOldProfiles.contains(profileFrom))
        {
          if (profileFrom == profileTo)
            FRenamedProfiles.remove(profileFrom);
          else
            FRenamedProfiles.insert(profileFrom,profileTo);
        }
        renameProfile(listItem->text(),profileTo);
      }
    }
    else
      QMessageBox::information(this,tr("Renaming profile"),tr("Profile <b>%1</b> allready exists.").arg(profileTo));
  }
}

void ProfileDialog::onRemoveProfileClicked()
{
  QListWidgetItem *listItem = ui.lstProfiles->selectedItems().value(0);
  if (listItem)
  {
    QString oldProfile = listItem->text();
    QMessageBox::StandardButton button = QMessageBox::Yes;
    if (FOldProfiles.contains(oldProfile) || !FRenamedProfiles.key(oldProfile).isEmpty())
    {
      button = QMessageBox::question(this,tr("Removing profile"),
        tr("Are you sure that wish to remove profile <b>%1</b> with accounts?").arg(oldProfile),
        QMessageBox::Yes|QMessageBox::No);
    }
    if (button == QMessageBox::Yes)
    {
      FRenamedProfiles.remove(FRenamedProfiles.key(oldProfile));
      removeProfile(oldProfile);  
    }
  }
}

void ProfileDialog::onAccepted()
{
  QString newProfile = ui.cmbProfiles->currentText();
  QSet<QString> renProfiles = FRenamedProfiles.keys().toSet();
  QSet<QString> oldProfiles = FOldProfiles - FNewProfiles;
  QSet<QString> newProfiles = FNewProfiles - FOldProfiles;
  
  foreach (QString profileFrom, renProfiles)
  {
    QString profileTo = FRenamedProfiles.value(profileFrom);
    FSettingsPlugin->renameProfile(profileFrom,profileTo);
    oldProfiles -= profileFrom;
    newProfiles -= profileTo;
  }

  foreach (QString profile, newProfiles)
    FSettingsPlugin->addProfile(profile);

  if (newProfile != FSettingsPlugin->profile())
    FSettingsPlugin->setProfile(newProfile);

  foreach (QString profile, oldProfiles)
    FSettingsPlugin->removeProfile(profile);

  FSettingsPlugin->saveSettings();

  accept();
}
