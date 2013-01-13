#include "messengeroptions.h"

#include <QKeyEvent>
#include <QFontDialog>

MessengerOptions::MessengerOptions(IMessenger *AMessenger, QWidget *AParent) : QWidget(AParent)
{
  ui.setupUi(this);
  FMessenger = AMessenger;

  ui.chbUseTabWindow->setChecked(FMessenger->checkOption(IMessenger::UseTabWindow));
  ui.chbViewShowHtml->setChecked(FMessenger->checkOption(IMessenger::ShowHTML));
  ui.chbViewShowDateTime->setChecked(FMessenger->checkOption(IMessenger::ShowDateTime));
  ui.chbChatShowStatus->setChecked(FMessenger->checkOption(IMessenger::ShowStatus));
  setChatFont(FMessenger->defaultChatFont());
  setMessageFont(FMessenger->defaultMessageFont());

  connect(ui.pbtChatFont,SIGNAL(clicked()),SLOT(onChangeChatFont()));
  connect(ui.pbtMessageFont,SIGNAL(clicked()),SLOT(onChangeMessageFont()));

  FSendKey = FMessenger->sendMessageKey();
  ui.lneSendKey->setText(FSendKey.toString());
  ui.lneSendKey->installEventFilter(this);
}

void MessengerOptions::apply()
{
  FMessenger->setOption(IMessenger::UseTabWindow,ui.chbUseTabWindow->isChecked());
  FMessenger->setOption(IMessenger::ShowHTML,ui.chbViewShowHtml->isChecked());
  FMessenger->setOption(IMessenger::ShowDateTime,ui.chbViewShowDateTime->isChecked());
  FMessenger->setOption(IMessenger::ShowStatus,ui.chbChatShowStatus->isChecked());
  FMessenger->setDefaultChatFont(ui.lblChatFont->font());
  FMessenger->setDefaultMessageFont(ui.lblMessageFont->font());
  FMessenger->setSendMessageKey(FSendKey);
  emit optionsApplied();
}

MessengerOptions::~MessengerOptions()
{

}

void MessengerOptions::setChatFont(const QFont &AFont)
{
  ui.lblChatFont->setFont(AFont);
  ui.lblChatFont->setText(QString("%1 %2").arg(AFont.family()).arg(AFont.pointSize()));
}

void MessengerOptions::setMessageFont(const QFont &AFont)
{
  ui.lblMessageFont->setFont(AFont);
  ui.lblMessageFont->setText(QString("%1 %2").arg(AFont.family()).arg(AFont.pointSize()));
}

bool MessengerOptions::eventFilter(QObject *AWatched, QEvent *AEvent)
{
  static const QList<int> controlKeys =  QList<int>() << Qt::Key_unknown << Qt::Key_Control << Qt::Key_Meta << Qt::Key_Alt << Qt::Key_AltGr;

  if (AWatched==ui.lneSendKey && AEvent->type()==QEvent::KeyPress)
  {
    QKeyEvent *keyEvent = static_cast<QKeyEvent *>(AEvent);
    if (!controlKeys.contains(keyEvent->key()))
    {
      FSendKey = QKeySequence(keyEvent->modifiers() | keyEvent->key());
      ui.lneSendKey->setText(FSendKey.toString());
    }
    return true;
  }
  return QWidget::eventFilter(AWatched,AEvent);
}

void MessengerOptions::onChangeChatFont()
{
  bool ok = false;
  QFont font = QFontDialog::getFont(&ok,ui.lblChatFont->font(),this,tr("Select font for chat window"));
  if (ok)
    setChatFont(font);
}

void MessengerOptions::onChangeMessageFont()
{
  bool ok = false;
  QFont font = QFontDialog::getFont(&ok,ui.lblMessageFont->font(),this,tr("Select font for message window"));
  if (ok)
    setMessageFont(font);
}

