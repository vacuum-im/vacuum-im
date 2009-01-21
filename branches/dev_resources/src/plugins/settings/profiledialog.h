#ifndef PROFILEDIALOG_H
#define PROFILEDIALOG_H

#include <QDialog>
#include <QSet>
#include "../../interfaces/isettings.h"
#include "ui_profiledialog.h"

class ProfileDialog : 
  public QDialog
{
  Q_OBJECT;

public:
  ProfileDialog(ISettingsPlugin *ASettingsPlugin);
  ~ProfileDialog();
protected:
  void addProfile(const QString &AProfile);
  void renameProfile(const QString &AProfileFrom, const QString &AProfileTo);
  void removeProfile(const QString &AProfile);
  void updateDialog();
protected slots:
  void onProfileAdded(const QString &AProfile);
  void onProfileRenamed(const QString &AProfileFrom, const QString &AProfileTo);
  void onProfileRemoved(const QString &AProfile);
  void onNewProfileClicked();
  void onRenameProfileClicked();
  void onRemoveProfileClicked();
  void onAccepted();
private:
  ISettingsPlugin *FSettingsPlugin;
private:
  Ui::ProfileDialogClass ui;
  QSet<QString> FOldProfiles;
  QSet<QString> FNewProfiles;
  QHash<QString,QString> FRenamedProfiles;
};

#endif // PROFILEDIALOG_H
