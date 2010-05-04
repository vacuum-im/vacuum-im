#include "messengeroptions.h"

#include <QKeyEvent>
#include <QFontDialog>

MessengerOptions::MessengerOptions(IMessageWidgets *AMessageWidgets, QWidget *AParent) : QWidget(AParent)
{
  ui.setupUi(this);
  FMessageWidgets = AMessageWidgets;
  ui.lneEditorSendKey->installEventFilter(this);

  connect(ui.spbEditorMinimumLines,SIGNAL(valueChanged(int)),SIGNAL(modified()));
  connect(ui.lneEditorSendKey,SIGNAL(textChanged(const QString &)),SIGNAL(modified()));

  reset();
}

MessengerOptions::~MessengerOptions()
{

}

void MessengerOptions::apply()
{
  Options::node(OPV_MESSAGES_EDITORMINIMUMLINES).setValue(ui.spbEditorMinimumLines->value());
  Options::node(OPV_MESSAGES_EDITORSENDKEY).setValue(FSendKey);
  emit childApply();
}

void MessengerOptions::reset()
{
  ui.spbEditorMinimumLines->setValue(Options::node(OPV_MESSAGES_EDITORMINIMUMLINES).value().toInt());
  FSendKey = Options::node(OPV_MESSAGES_EDITORSENDKEY).value().value<QKeySequence>();
  ui.lneEditorSendKey->setText(FSendKey.toString());
  emit childReset();
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

