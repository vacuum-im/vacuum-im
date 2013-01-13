#ifndef EDITUSERSLISTDIALOG_H
#define EDITUSERSLISTDIALOG_H

#include <QDialog>
#include "../../definations/resources.h"
#include "../../definations/menuicons.h"
#include "../../interfaces/imultiuserchat.h"
#include "../../utils/iconstorage.h"
#include "ui_edituserslistdialog.h"

struct UsersListItem 
{
  Jid userJid;
  QString reason;
};

class EditUsersListDialog : 
  public QDialog
{
  Q_OBJECT;
public:
  EditUsersListDialog(const QString &AAffiliation, const QList<IMultiUserListItem> &AList, QWidget *AParent = NULL);
  ~EditUsersListDialog();
  QString affiliation() const { return FAffiliation; }
  QList<IMultiUserListItem> deltaList() const;
  void setTitle(const QString &ATitle);
protected slots:
  void onAddClicked();
  void onDeleteClicked();
private:
  Ui::EditUsersListDialogClass ui;
private:
  QString FAffiliation;
  QHash<Jid,QTableWidgetItem *> FCurrentItems;
  QHash<Jid,QTableWidgetItem *> FAddedItems;
  QList<Jid> FDeletedItems;
};

#endif // EDITUSERSLISTDIALOG_H
