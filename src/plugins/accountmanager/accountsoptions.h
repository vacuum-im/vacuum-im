#ifndef ACCOUNTSOPTIONS_H
#define ACCOUNTSOPTIONS_H

#include <QMap>
#include <QWidget>
#include "../../interfaces/iaccountmanager.h"
#include "../../interfaces/isettings.h"
#include "ui_accountsoptions.h"
#include "accountoptions.h"
#include "accountmanager.h"

class AccountManager;

class AccountsOptions : 
  public QWidget
{
  Q_OBJECT;
public:
  AccountsOptions(AccountManager *AManager, QWidget *AParent = NULL);
  ~AccountsOptions();
  QWidget *accountOptions(const QUuid &AAccountId);
public slots:
  void apply();
  void reject();
signals:
  void optionsAccepted();
  void optionsRejected();
protected:
  QTreeWidgetItem *appendAccount(const QUuid &AAccountId, const QString &AName);
  void removeAccount(const QUuid &AAccountId);
protected slots:
  void onAccountAdd();
  void onAccountRemove();
  void onItemActivated(QTreeWidgetItem *AItem, int AColumn);
private:
  Ui::AccountsOptionsClass ui;
private:
  AccountManager *FManager;
private:
  QMap<QUuid, QTreeWidgetItem *> FAccountItems;
  QMap<QUuid, AccountOptions *> FAccountOptions;
};

#endif // ACCOUNTSOPTIONS_H
