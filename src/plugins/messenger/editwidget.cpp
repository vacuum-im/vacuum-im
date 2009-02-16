#include "editwidget.h"

EditWidget::EditWidget(IMessenger *AMessenger, const Jid& AStreamJid, const Jid &AContactJid)
{
  ui.setupUi(this);
  ui.tedEditor->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);

  FMessenger = AMessenger;
  FStreamJid = AStreamJid;
  FContactJid = AContactJid;

  FSendShortcut = new QShortcut(FMessenger->sendMessageKey(),ui.tedEditor);
  FSendShortcut->setContext(Qt::WidgetShortcut);
  connect(FSendShortcut,SIGNAL(activated()),SLOT(onShortcutActivated()));
  connect(FMessenger->instance(),SIGNAL(sendMessageKeyChanged(const QKeySequence &)),
    SLOT(onSendMessageKeyChanged(const QKeySequence &)));
  ui.tedEditor->installEventFilter(this);
}

EditWidget::~EditWidget()
{

}

void EditWidget::sendMessage()
{
  emit messageAboutToBeSend();
  emit messageReady();
}

void EditWidget::setSendMessageKey(const QKeySequence &AKey)
{
  FSendShortcut->setKey(AKey);
  emit sendMessageKeyChanged(AKey);
}

void EditWidget::setStreamJid(const Jid &AStreamJid)
{
  Jid befour = FStreamJid;
  FStreamJid = AStreamJid;
  emit streamJidChanged(befour);
}

void EditWidget::setContactJid(const Jid &AContactJid)
{
  Jid befour = FContactJid;
  FContactJid = AContactJid;
  emit contactJidChanged(AContactJid);
}

void EditWidget::clearEditor()
{
  ui.tedEditor->clear();
  emit editorCleared();
}

bool EditWidget::eventFilter(QObject *AWatched, QEvent *AEvent)
{
  bool hooked = false;
  if (AWatched==ui.tedEditor && AEvent->type()==QEvent::KeyPress)
  {
    QKeyEvent *keyEvent = static_cast<QKeyEvent *>(AEvent);
    emit keyEventReceived(keyEvent,hooked);
    hooked = hooked || (keyEvent->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier)) > 0;
  }
  else if (AWatched==ui.tedEditor && AEvent->type()==QEvent::ShortcutOverride)
  {
    hooked = true;
  }
  return hooked || QWidget::eventFilter(AWatched,AEvent);
}

void EditWidget::onShortcutActivated()
{
  QShortcut *shortcut = qobject_cast<QShortcut *>(sender());
  if (shortcut == FSendShortcut)
  {
    sendMessage();
  }
}

void EditWidget::onSendMessageKeyChanged(const QKeySequence &AKey)
{
  if (!FSendShortcut->key().isEmpty() && !AKey.isEmpty())
    setSendMessageKey(AKey);
}