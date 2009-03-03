#ifndef ACCOUNTOPTIONS_H
#define ACCOUNTOPTIONS_H

#include <QWidget>
#include <QVariant>
#include "../../utils/jid.h"
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
    AO_DefLang,
  };
public:
  AccountOptions(const QString &AAccountId, QWidget *AParent = NULL);
  ~AccountOptions();

  const QString &accountId() const { return FAccountId; }
  QVariant option(const Options &AOption) const;
  void setOption(const Options &AOption, const QVariant &AValue);
private:
  Ui::AccountOptionsClass ui;
private:
  QString FAccountId;
};

#endif // ACCOUNTOPTIONS_H
