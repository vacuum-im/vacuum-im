#include "messengeroptions.h"

#include <QKeyEvent>
#include <QFontDialog>

MessengerOptions::MessengerOptions(IMessageWidgets *AMessageWidgets, QWidget *AParent) : QWidget(AParent)
{
  ui.setupUi(this);
  FMessageWidgets = AMessageWidgets;

  ui.chbTabWindowsEnabled->setChecked(FMessageWidgets->tabWindowsEnabled());
  ui.chbChatWindowShowStatus->setChecked(FMessageWidgets->chatWindowShowStatus());
  ui.chbEditorAutoResize->setChecked(FMessageWidgets->editorAutoResize());
  ui.spbEditorMinimumLines->setValue(FMessageWidgets->editorMinimumLines());
  FSendKey = FMessageWidgets->editorSendKey();
  ui.lneEditorSendKey->setText(FSendKey.toString());
  ui.lneEditorSendKey->installEventFilter(this);
}

MessengerOptions::~MessengerOptions()
{

}

void MessengerOptions::apply()
{
  FMessageWidgets->setTabWindowsEnabled(ui.chbTabWindowsEnabled->isChecked());
  FMessageWidgets->setChatWindowShowStatus(ui.chbChatWindowShowStatus->isChecked());
  FMessageWidgets->setEditorAutoResize(ui.chbEditorAutoResize->isChecked());
  FMessageWidgets->setEditorMinimumLines(ui.spbEditorMinimumLines->value());
  FMessageWidgets->setEditorSendKey(FSendKey);
  emit optionsAccepted();
}

bool MessengerOptions::eventFilter(QObject *AWatched, QEvent *AEvent)
{
  static const QList<int> controlKeys =  QList<int>() << Qt::Key_unknown << Qt::Key_Control << Qt::Key_Meta << Qt::Key_Alt << Qt::Key_AltGr;

  if (AWatched==ui.lneEditorSendKey && AEvent->type()==QEvent::KeyPress)
  {
    QKeyEvent *keyEvent = static_cast<QKeyEvent *>(AEvent);
    if (!controlKeys.contains(keyEvent->key()))
    {
      FSendKey = QKeySequence(keyEvent->modifiers() | keyEvent->key());
      ui.lneEditorSendKey->setText(FSendKey.toString());
    }
    return true;
  }
  return QWidget::eventFilter(AWatched,AEvent);
}

