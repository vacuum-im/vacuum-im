#include "editwidget.h"

#include <QKeyEvent>

#define MAX_BUFFERED_MESSAGES     10

EditWidget::EditWidget(IMessageWidgets *AMessageWidgets, const Jid& AStreamJid, const Jid &AContactJid)
{
  ui.setupUi(this);
  ui.medEditor->setAcceptRichText(false);
  ui.medEditor->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);

  FMessageWidgets = AMessageWidgets;
  FStreamJid = AStreamJid;
  FContactJid = AContactJid;
  FBufferPos = -1;

  FSendShortcut = new QShortcut(ui.medEditor);
  FSendShortcut->setContext(Qt::WidgetShortcut);
  connect(FSendShortcut,SIGNAL(activated()),SLOT(onShortcutActivated()));

  IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(ui.tlbSend,MNI_MESSAGEWIDGETS_SEND);
  connect(ui.tlbSend,SIGNAL(clicked(bool)),SLOT(onSendButtonCliked(bool)));

  ui.medEditor->installEventFilter(this);

  setAutoResize(FMessageWidgets->editorAutoResize());
  setMinimumLines(FMessageWidgets->editorMinimumLines());
  setSendKey(FMessageWidgets->editorSendKey());

  connect(FMessageWidgets->instance(),SIGNAL(editorAutoResizeChanged(bool)), SLOT(onEditorAutoResizeChanged(bool)));
  connect(FMessageWidgets->instance(),SIGNAL(editorMinimumLinesChanged(int)), SLOT(onEditorMinimumLinesChanged(int)));
  connect(FMessageWidgets->instance(),SIGNAL(editorSendKeyChanged(const QKeySequence &)), SLOT(onEditorSendKeyChanged(const QKeySequence &)));
}

EditWidget::~EditWidget()
{

}

const Jid &EditWidget::streamJid() const
{
  return FStreamJid;
}

void EditWidget::setStreamJid(const Jid &AStreamJid)
{
  Jid befour = FStreamJid;
  FStreamJid = AStreamJid;
  emit streamJidChanged(befour);
}

const Jid & EditWidget::contactJid() const
{
   return FContactJid;
}

void EditWidget::setContactJid(const Jid &AContactJid)
{
  Jid befour = FContactJid;
  FContactJid = AContactJid;
  emit contactJidChanged(AContactJid);
}

QTextEdit *EditWidget::textEdit() const
{
  return ui.medEditor;
}

QTextDocument *EditWidget::document() const
{
  return ui.medEditor->document();
}

void EditWidget::sendMessage()
{
  emit messageAboutToBeSend();
  appendMessageToBuffer();
  emit messageReady();
}

void EditWidget::clearEditor()
{
  ui.medEditor->clear();
  emit editorCleared();
}

bool EditWidget::autoResize() const
{
  return ui.medEditor->autoResize();
}

void EditWidget::setAutoResize(bool AResize)
{
  ui.medEditor->setAutoResize(AResize);
  emit autoResizeChanged(ui.medEditor->autoResize());
}

int EditWidget::minimumLines() const
{
  return ui.medEditor->minimumLines();
}

void EditWidget::setMinimumLines(int ALines)
{
  ui.medEditor->setMinimumLines(ALines);
  emit minimumLinesChanged(ui.medEditor->minimumLines());
}

QKeySequence EditWidget::sendKey() const
{
  return FSendShortcut->key();
}

void EditWidget::setSendKey(const QKeySequence &AKey)
{
  FSendShortcut->setKey(AKey);
  emit sendKeyChanged(AKey);
}

bool EditWidget::sendButtonVisible() const
{
  return ui.tlbSend->isVisible();
}

void EditWidget::setSendButtonVisible(bool AVisible)
{
  ui.tlbSend->setVisible(AVisible);
}

bool EditWidget::eventFilter(QObject *AWatched, QEvent *AEvent)
{
  bool hooked = false;
  if (AWatched==ui.medEditor && AEvent->type()==QEvent::KeyPress)
  {
    QKeyEvent *keyEvent = static_cast<QKeyEvent *>(AEvent);
    emit keyEventReceived(keyEvent,hooked);

    if (!hooked && keyEvent->modifiers()==Qt::CTRL && keyEvent->key()==Qt::Key_Up)
    {
      hooked = true;
      showNextBufferedMessage();
    }
    else if (!hooked && keyEvent->modifiers()==Qt::CTRL && keyEvent->key() == Qt::Key_Down)
    {
      hooked = true;
      showPrevBufferedMessage();
    }
  }
  else if (AWatched==ui.medEditor && AEvent->type()==QEvent::ShortcutOverride)
  {
    hooked = true;
  }
  return hooked || QWidget::eventFilter(AWatched,AEvent);
}

void EditWidget::appendMessageToBuffer()
{
  QString message = ui.medEditor->toPlainText();
  if (!message.isEmpty())
  {
    FBufferPos = -1;
    int index = FBuffer.indexOf(message);
    if (index >= 0)
      FBuffer.removeAt(index);
    FBuffer.prepend(message);
    if (FBuffer.count() > MAX_BUFFERED_MESSAGES)
      FBuffer.removeLast();
  }
}

void EditWidget::showBufferedMessage()
{
  ui.medEditor->setPlainText(FBuffer.value(FBufferPos));
}

void EditWidget::showNextBufferedMessage()
{
  if (FBufferPos < FBuffer.count()-1)
  {
    if (FBufferPos<0 && !ui.medEditor->toPlainText().isEmpty())
    {
      appendMessageToBuffer();
      FBufferPos++;
    }
    FBufferPos++;
    showBufferedMessage();
  }
}

void EditWidget::showPrevBufferedMessage()
{
  if (FBufferPos > 0)
  {
    FBufferPos--;
    showBufferedMessage();
  }
}

void EditWidget::onShortcutActivated()
{
  QShortcut *shortcut = qobject_cast<QShortcut *>(sender());
  if (shortcut == FSendShortcut)
  {
    sendMessage();
  }
}

void EditWidget::onSendButtonCliked(bool)
{
  sendMessage();
}

void EditWidget::onEditorAutoResizeChanged(bool AResize)
{
  setAutoResize(AResize);
}

void EditWidget::onEditorMinimumLinesChanged(int ALines)
{
  setMinimumLines(ALines);
}

void EditWidget::onEditorSendKeyChanged(const QKeySequence &AKey)
{
  setSendKey(AKey);
}
