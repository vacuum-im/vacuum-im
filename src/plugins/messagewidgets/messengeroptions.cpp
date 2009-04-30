#include "messengeroptions.h"

#include <QKeyEvent>
#include <QFontDialog>

MessengerOptions::MessengerOptions(IMessageWidgets *AMessageWidgets, QWidget *AParent) : QWidget(AParent)
{
  ui.setupUi(this);
  FMessageWidgets = AMessageWidgets;

  ui.chbUseTabWindow->setChecked(FMessageWidgets->checkOption(IMessageWidgets::UseTabWindow));
  ui.chbChatShowStatus->setChecked(FMessageWidgets->checkOption(IMessageWidgets::ShowStatus));
  setChatFont(FMessageWidgets->defaultChatFont());
  setMessageFont(FMessageWidgets->defaultMessageFont());

  connect(ui.pbtChatFont,SIGNAL(clicked()),SLOT(onChangeChatFont()));
  connect(ui.pbtMessageFont,SIGNAL(clicked()),SLOT(onChangeMessageFont()));

  FSendKey = FMessageWidgets->sendMessageKey();
  ui.lneSendKey->setText(FSendKey.toString());
  ui.lneSendKey->installEventFilter(this);
}

void MessengerOptions::apply()
{
  FMessageWidgets->setOption(IMessageWidgets::UseTabWindow,ui.chbUseTabWindow->isChecked());
  FMessageWidgets->setOption(IMessageWidgets::ShowStatus,ui.chbChatShowStatus->isChecked());
  FMessageWidgets->setDefaultChatFont(ui.lblChatFont->font());
  FMessageWidgets->setDefaultMessageFont(ui.lblMessageFont->font());
  FMessageWidgets->setSendMessageKey(FSendKey);
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

