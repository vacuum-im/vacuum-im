#ifndef ACCOUNTOPTIONS_H
#define ACCOUNTOPTIONS_H

#include <QWidget>
#include <definations/version.h>
#include <definations/optionnodes.h>
#include <interfaces/iaccountmanager.h>
#include <utils/jid.h>
#include "ui_accountoptions.h"

class AccountOptions : 
  public QWidget
{
  Q_OBJECT;
public:
  AccountOptions(IAccountManager *AManager, const QUuid &AAccountId, QWidget *AParent = NULL);
  ~AccountOptions();
public:
  void apply();
public:
  QString name() const;
  void setName(const QString &AName);
private:
  Ui::AccountOptionsClass ui;
private:
  IAccountManager *FManager;
private:
  QUuid FAccountId;
};

#endif // ACCOUNTOPTIONS_H
