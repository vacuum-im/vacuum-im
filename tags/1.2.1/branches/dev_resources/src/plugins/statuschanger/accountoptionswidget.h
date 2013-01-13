#ifndef ACCOUNTOPTIONSWIDGET_H
#define ACCOUNTOPTIONSWIDGET_H

#include <QWidget>
#include "ui_accountoptionswidget.h"

class AccountOptionsWidget : 
  public QWidget
{
  Q_OBJECT;

public:
  AccountOptionsWidget(const QString &AAccountId);
  ~AccountOptionsWidget();
  
  QString accountId() const { return FAccountId; }
  bool autoConnect() const;
  void setAutoConnect(bool AAutoConnect);
  bool autoReconnect() const;
  void setAutoReconnect(bool AAutoReconnect);
private:
  Ui::AccountOptionsWidgetClass ui;
private:
  QString FAccountId;
};

#endif // ACCOUNTOPTIONSWIDGET_H
