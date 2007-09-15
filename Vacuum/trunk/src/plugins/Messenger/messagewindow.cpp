#include "messagewindow.h"

#include <QVBoxLayout>

MessageWindow::MessageWindow(QWidget *AParent)
  : QMainWindow(AParent)
{
  setupUi(this);
  setAttribute(Qt::WA_DeleteOnClose,true);

  FToolBar = new QToolBar(wdtToolbars);
  FToolBar->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
  QVBoxLayout *toolBarsLayout = new QVBoxLayout(wdtToolbars);
  toolBarsLayout->setMargin(0);
  toolBarsLayout->addWidget(FToolBar);  

  //FButtonGroup = new QButtonGroup(this);
  //FButtonGroup->addButton(pbtSend,SendButton);
  //FButtonGroup->addButton(pbtReply,ReplyButton);
  //FButtonGroup->addButton(pbtQuote,QuoteButton);
  //FButtonGroup->addButton(pbtChat,ChatButton);
  //FButtonGroup->addButton(pbtNext,NextButton);
  //FButtonGroup->addButton(pbtClose,CloseButton);
}

MessageWindow::~MessageWindow()
{

}

void MessageWindow::setStreamJid(const Jid &AStreamJid)
{
  FStreamJid = AStreamJid;
  accountEdit()->setText(AStreamJid.full());
}

void MessageWindow::setContactJid(const Jid &AContactJid)
{
  FContactJid = AContactJid;
  contactEdit()->setText(AContactJid.full());
}

Message *MessageWindow::message()
{
  return FMessage;
}

