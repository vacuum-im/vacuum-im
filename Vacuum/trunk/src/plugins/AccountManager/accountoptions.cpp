#include <QtDebug>
#include "accountoptions.h"

#include "../../utils/jid.h"

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
    return ui.lneName->text(); break;
  case AO_StreamJid:
    {
      Jid streamJid(ui.lneJabberId->text());
      streamJid.setResource(ui.lneResource->text());
      return streamJid.full();
    }; break;
  case AO_Password:
    return ui.lnePassword->text(); break;
  case AO_ManualHostPort:
    return ui.chbManualHostPort->checkState() == Qt::Checked; break;
  case AO_Host:
    return ui.lneHost->text(); break;
  case AO_Port:
    return ui.spbPort->value(); break;
  case AO_ProxyType:
    return ui.cmbProxyType->currentIndex(); break;
  case AO_ProxyHost:
    return ui.lneProxyHost->text(); break;
  case AO_ProxyPort:
    return ui.spbProxyPort->value(); break;
  case AO_ProxyUser:
    return ui.lneProxyUser->text(); break;
  case AO_ProxyPassword:
    return ui.lneProxyPassword->text(); break;
  //case AO_PollServer:
  case AO_AutoConnect:
    return ui.chbAutoConnect->checkState() == Qt::Checked; break;
  case AO_AutoReconnect:
    return ui.chbAutoReconnect->checkState() == Qt::Checked; break;
  }
  return QVariant();
}

void AccountOptions::setOption(const Options &AOption, const QVariant &AValue)
{
  switch(AOption)
  {
  case AO_Name:
    ui.lneName->setText(AValue.toString()); break;
  case AO_StreamJid:
    {
      Jid streamJid(AValue.toString());
      ui.lneJabberId->setText(streamJid.bare());
      ui.lneResource->setText(streamJid.resource());
    }; break;
  case AO_Password:
    ui.lnePassword->setText(AValue.toString()); break;
  case AO_ManualHostPort:
    ui.chbManualHostPort->setCheckState(AValue.toBool() ? Qt::Checked : Qt::Unchecked); break;
  case AO_Host:
    ui.lneHost->setText(AValue.toString()); break;
  case AO_Port:
    ui.spbPort->setValue(AValue.toInt()); break;
  case AO_ProxyTypes:
    ui.cmbProxyType->addItems(AValue.toStringList()); break;
  case AO_ProxyType:
    ui.cmbProxyType->setCurrentIndex(AValue.toInt()); break;
  case AO_ProxyHost:
    ui.lneProxyHost->setText(AValue.toString()); break;
  case AO_ProxyPort:
    ui.spbProxyPort->setValue(AValue.toInt()); break;
  case AO_ProxyUser:
    ui.lneProxyUser->setText(AValue.toString()); break;
  case AO_ProxyPassword:
    ui.lneProxyPassword->setText(AValue.toString()); break;
  //case AO_PollServer:
  case AO_AutoConnect:
    ui.chbAutoConnect->setCheckState(AValue.toBool() ? Qt::Checked : Qt::Unchecked); break;
  case AO_AutoReconnect:
    ui.chbAutoReconnect->setCheckState(AValue.toBool() ? Qt::Checked : Qt::Unchecked); break;
  }

}