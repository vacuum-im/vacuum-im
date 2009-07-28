#include "messengeroptions.h"

#include <QKeyEvent>
#include <QFontDialog>

MessengerOptions::MessengerOptions(IMessageWidgets *AMessageWidgets, QWidget *AParent) : QWidget(AParent)
{
  ui.setupUi(this);
  FMessageWidgets = AMessageWidgets;

  ui.chbUseTabWindow->setChecked(FMessageWidgets->checkOption(IMessageWidgets::UseTabWindow));
  ui.chbChatShowStatus->setChecked(FMessageWidgets->checkOption(IMessageWidgets::ShowStatus));
  FSendKey = FMessageWidgets->sendMessageKey();
  ui.lneSendKey->setText(FSendKey.toString());
  ui.lneSendKey->installEventFilter(this);
}

void MessengerOptions::apply()
{
  FMessageWidgets->setOption(IMessageWidgets::UseTabWindow,ui.chbUseTabWindow->isChecked());
  FMessageWidgets->setOption(IMessageWidgets::ShowStatus,ui.chbChatShowStatus->isChecked());
  FMessageWidgets->setSendMessageKey(FSendKey);
  emit optionsAccepted();
}

MessengerOptions::~MessengerOptions()
{

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

