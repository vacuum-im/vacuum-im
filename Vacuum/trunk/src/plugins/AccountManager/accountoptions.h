#ifndef ACCOUNTOPTIONS_H
#define ACCOUNTOPTIONS_H

#include <QWidget>
#include <QVariant>
#include "ui_accountoptions.h"

class AccountOptions : 
  public QWidget
{
  Q_OBJECT;

public:
  enum Options {
    AO_Id,
    AO_Name,
    AO_StreamJid,
    AO_Password,
    AO_ManualHostPort,
    AO_Host,
    AO_Port,
    AO_DefLang,
    AO_ProxyTypes,
    AO_ProxyType,
    AO_ProxyHost,
    AO_ProxyPort,
    AO_ProxyUser,
    AO_ProxyPassword,
    AO_PollServer,
    AO_AutoConnect,
    AO_AutoReconnect
  };
public:
  AccountOptions(const QString &AAccountId, QWidget *AParent = NULL);
  ~AccountOptions();
  const QString &accountId() const { return FAccountId; }
  QVariant option(const Options &AOption) const;
  void setOption(const Options &AOption, const QVariant &AValue);
private:
  QString FAccountId;
private:
  Ui::AccountOptionsClass ui;
};

#endif // ACCOUNTOPTIONS_H
