#include "messengeroptions.h"

#include <QFontDialog>

MessengerOptions::MessengerOptions(QWidget *AParent)
    : QWidget(AParent)
{
  ui.setupUi(this);
  FOptions = 0;

  connect(ui.pbtChatFont,SIGNAL(clicked()),SLOT(onChangeChatFont()));
  connect(ui.pbtMessageFont,SIGNAL(clicked()),SLOT(onChangeMessageFont()));
}

MessengerOptions::~MessengerOptions()
{

}

void MessengerOptions::setChatFont(const QFont &AFont)
{
  FChatFont = AFont;
  ui.lblChatFont->setFont(AFont);
  ui.lblChatFont->setText(QString("%1 %2").arg(AFont.family()).arg(AFont.pointSize()));
}

void MessengerOptions::setMessageFont(const QFont &AFont)
{
  FMessageFont = AFont;
  ui.lblMessageFont->setFont(AFont);
  ui.lblMessageFont->setText(QString("%1 %2").arg(AFont.family()).arg(AFont.pointSize()));
}

bool MessengerOptions::checkOption(IMessenger::Option AOption) const
{
  switch(AOption)
  {
  case IMessenger::UseTabWindow:
    return ui.chbUseTabWindow->isChecked();
  case IMessenger::ShowHTML:
  	return ui.chbViewShowHtml->isChecked();
  case IMessenger::ShowDateTime:
    return ui.chbViewShowDateTime->isChecked();
  case IMessenger::ShowStatus:
    return ui.chbChatShowStatus->isChecked();
  default:
    return false;
  }
}

void MessengerOptions::setOption(IMessenger::Option AOption, bool AValue)
{
  switch(AOption)
  {
  case IMessenger::UseTabWindow:
    ui.chbUseTabWindow->setChecked(AValue);
    break;
  case IMessenger::ShowHTML:
    ui.chbViewShowHtml->setChecked(AValue);
    break;
  case IMessenger::ShowDateTime:
    ui.chbViewShowDateTime->setChecked(AValue);
    break;
  case IMessenger::ShowStatus:
    ui.chbChatShowStatus->setChecked(AValue);
    break;
  }
}

void MessengerOptions::onChangeChatFont()
{
  bool ok = false;
  QFont font = QFontDialog::getFont(&ok,FChatFont,this,tr("Select font for chat window"));
  if (ok)
    setChatFont(font);
}

void MessengerOptions::onChangeMessageFont()
{
  bool ok = false;
  QFont font = QFontDialog::getFont(&ok,FMessageFont,this,tr("Select font for message window"));
  if (ok)
    setMessageFont(font);
}