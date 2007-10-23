#include "messengeroptions.h"

MessengerOptions::MessengerOptions(QWidget *AParent)
    : QWidget(AParent)
{
  ui.setupUi(this);
  FOptions = 0;
}

MessengerOptions::~MessengerOptions()
{

}

bool MessengerOptions::checkOption(IMessenger::Option AOption) const
{
  switch(AOption)
  {
  case IMessenger::OpenChatInTabWindow:
    return ui.chbOpenChatInTab->isChecked();
  case IMessenger::OpenMessageWindow:
    return ui.chbOpenMessageWindow->isChecked();
  case IMessenger::OpenChatWindow:
    return ui.chbOpenChatWindow->isChecked();
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
  case IMessenger::OpenChatInTabWindow:
    ui.chbOpenChatInTab->setChecked(AValue);
    break;
  case IMessenger::OpenMessageWindow:
    ui.chbOpenMessageWindow->setChecked(AValue);
    break;
  case IMessenger::OpenChatWindow:
    ui.chbOpenChatWindow->setChecked(AValue);
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