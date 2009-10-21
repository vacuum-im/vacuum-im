#ifndef ACCOUNTOPTIONSWIDGET_H
#define ACCOUNTOPTIONSWIDGET_H

#include <QWidget>
#include <definations/accountvaluenames.h>
#include <interfaces/iaccountmanager.h>
#include "ui_accountoptionswidget.h"

class AccountOptionsWidget : 
  public QWidget
{
  Q_OBJECT;
public:
  AccountOptionsWidget(IAccountManager *AAccountManager, const QUuid &AAccountId);
  ~AccountOptionsWidget();
public slots:
  void apply();
signals:
  void optionsAccepted();
private:
  Ui::AccountOptionsWidgetClass ui;
private:
  IAccountManager *FAccountManager;
private:
  QUuid FAccountId;
};

#endif // ACCOUNTOPTIONSWIDGET_H
