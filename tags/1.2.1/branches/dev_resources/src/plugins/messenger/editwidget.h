#ifndef EDITWIDGET_H
#define EDITWIDGET_H

#include <QShortcut>
#include <QKeyEvent>
#include "../../interfaces/imessenger.h"
#include "ui_editwidget.h"

class EditWidget : 
  public IEditWidget
{
  Q_OBJECT;
  Q_INTERFACES(IEditWidget);
public:
  EditWidget(IMessenger *AMessenger, const Jid &AStreamJid, const Jid &AContactJid);
  ~EditWidget();
  virtual const Jid &streamJid() const { return FStreamJid; }
  virtual void setStreamJid(const Jid &AStreamJid);
  virtual const Jid &contactJid() const { return FContactJid; }
  virtual void setContactJid(const Jid &AContactJid);
  virtual QTextEdit *textEdit() const { return ui.tedEditor; }
  virtual QTextDocument *document() const { return ui.tedEditor->document(); }
  virtual void sendMessage();
  virtual QKeySequence sendMessageKey() const { return FSendShortcut->key(); }
  virtual void setSendMessageKey(const QKeySequence &AKey);
  virtual void clearEditor();
signals:
  virtual void keyEventReceived(QKeyEvent *AKeyEvent, bool &AHook);
  virtual void messageAboutToBeSend();
  virtual void messageReady();
  virtual void streamJidChanged(const Jid &ABefour);
  virtual void contactJidChanged(const Jid &ABefour);
  virtual void sendMessageKeyChanged(const QKeySequence &AKey);
  virtual void editorCleared();
protected:
  virtual bool eventFilter(QObject *AWatched, QEvent *AEvent);
protected slots:
  void onShortcutActivated();
  void onSendMessageKeyChanged(const QKeySequence &AKey);
private:
  Ui::EditWidgetClass ui;
private:
  IMessenger *FMessenger;
private:
  Jid FStreamJid;
  Jid FContactJid;
  QShortcut *FSendShortcut;
};

#endif // EDITWIDGET_H
