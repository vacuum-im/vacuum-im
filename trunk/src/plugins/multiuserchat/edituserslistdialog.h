#ifndef EDITUSERSLISTDIALOG_H
#define EDITUSERSLISTDIALOG_H

#include <QDialog>
#include <definations/resources.h>
#include <definations/menuicons.h>
#include <interfaces/imultiuserchat.h>
#include <utils/iconstorage.h>
#include "ui_edituserslistdialog.h"

class EditUsersListDialog : 
  public QDialog
{
  Q_OBJECT;
public:
  EditUsersListDialog(const QString &AAffiliation, const QList<IMultiUserListItem> &AList, QWidget *AParent = NULL);
  ~EditUsersListDialog();
  QString affiliation() const;
  QList<IMultiUserListItem> deltaList() const;
  void setTitle(const QString &ATitle);
protected slots:
  void onAddClicked();
  void onDeleteClicked();
private:
  Ui::EditUsersListDialogClass ui;
private:
  QString FAffiliation;
  QList<QString> FDeletedItems;
  QMap<QString,QTableWidgetItem *> FAddedItems;
  QMap<QString,QTableWidgetItem *> FCurrentItems;
};

#endif // EDITUSERSLISTDIALOG_H
