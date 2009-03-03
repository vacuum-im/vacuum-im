#include "accountoptions.h"

AccountOptions::AccountOptions(const QString &AAccountId, QWidget *AParent)
  : QWidget(AParent)
{
  FAccountId = AAccountId;
  ui.setupUi(this);
}

AccountOptions::~AccountOptions()
{

}

QVariant AccountOptions::option(const Options &AOption) const
{
  switch(AOption)
  {
  case AO_Name:
    return ui.lneName->text();
  case AO_StreamJid:
    {
      Jid streamJid(ui.lneJabberId->text());
      streamJid.setResource(ui.lneResource->text());
      return streamJid.full();
    }
  case AO_Password:
    return ui.lnePassword->text();
  default:
    return QVariant();
  };
  return QVariant();
}

void AccountOptions::setOption(const Options &AOption, const QVariant &AValue)
{
  switch(AOption)
  {
  case AO_Name:
    {
      ui.lneName->setText(AValue.toString());
      break;
    }
  case AO_StreamJid:
    {
      Jid streamJid(AValue.toString());
      ui.lneJabberId->setText(streamJid.bare());
      ui.lneResource->setText(streamJid.resource());
      break;
    }
  case AO_Password:
    {
      ui.lnePassword->setText(AValue.toString());
      break;
    }
  default:
    break;
  };
}
