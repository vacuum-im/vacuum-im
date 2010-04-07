#ifndef ACCOUNTSOPTIONS_H
#define ACCOUNTSOPTIONS_H

#include <QMap>
#include <QWidget>
#include <interfaces/iaccountmanager.h>
#include <interfaces/ioptionsmanager.h>
#include "ui_accountsoptions.h"
#include "accountoptions.h"
#include "accountmanager.h"

class AccountManager;

class AccountsOptions : 
  public QWidget,
  public IOptionsWidget
{
  Q_OBJECT;
  Q_INTERFACES(IOptionsWidget);
public:
  AccountsOptions(AccountManager *AManager, QWidget *AParent);
  ~AccountsOptions();
  virtual QWidget* instance() { return this; }
public slots:
  virtual void apply();
  virtual void reset();
signals:
  void modified();
  void childApply();
  void childReset();
public:
  AccountOptions *accountOptions(const QUuid &AAccountId, QWidget *AParent);
protected:
  QTreeWidgetItem *appendAccount(const QUuid &AAccountId, const QString &AName);
  void removeAccount(const QUuid &AAccountId);
protected slots:
  void onAddButtonClicked(bool);
  void onRemoveButtonClicked(bool);
  void onItemActivated(QTreeWidgetItem *AItem, int AColumn);
  void onAccountOptionsDestroyed(QObject *AObject);
private:
  Ui::AccountsOptionsClass ui;
private:
  AccountManager *FManager;
private:
  QMap<QUuid, QTreeWidgetItem *> FAccountItems;
  QMap<QUuid, AccountOptions *> FAccountOptions;
};

#endif // ACCOUNTSOPTIONS_H
