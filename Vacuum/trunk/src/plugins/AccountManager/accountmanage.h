#ifndef ACCOUNTMANAGE_H
#define ACCOUNTMANAGE_H

#include <QWidget>
#include <QHash>
#include <QList>
#include "ui_accountmanage.h"


class AccountManage : 
  public QWidget
{
  Q_OBJECT;
public:
  AccountManage(QWidget *AParent = NULL);
  ~AccountManage();
  void setAccount(const QString &AId, const QString &AName, const QString &AStreamJid, bool AShown);
  bool isAccountActive(const QString &AId) const;
  QString accountName(const QString &AId) const;
  void removeAccount(const QString &AId);
signals:
  void accountAdded(const QString &AName);
  void accountShow(const QString &AAccountId);
  void accountRemoved(const QString &AAccountId);
protected slots:
  void onAccountAdd();
  void onAccountRemove();
  void onItemActivated(QTreeWidgetItem *AItem, int AColumn);
private:
  QHash<QString,QTreeWidgetItem *> FAccountItems;
private:
  Ui::AccountManageClass ui;
};

#endif // ACCOUNTMANAGE_H
