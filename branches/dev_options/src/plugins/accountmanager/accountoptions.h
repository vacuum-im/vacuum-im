#ifndef ACCOUNTOPTIONS_H
#define ACCOUNTOPTIONS_H

#include <QWidget>
#include <definations/version.h>
#include <interfaces/iaccountmanager.h>
#include <interfaces/ioptionsmanager.h>
#include <utils/jid.h>
#include "ui_accountoptions.h"

class AccountOptions : 
  public QWidget,
  public IOptionsWidget
{
  Q_OBJECT;
  Q_INTERFACES(IOptionsWidget);
public:
  AccountOptions(IAccountManager *AManager, const QUuid &AAccountId, QWidget *AParent);
  ~AccountOptions();
  virtual QWidget* instance() { return this; }
public slots:
  virtual void apply();
  virtual void reset();
signals:
  void modified();
  void childApply();
  void childReset();
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
