#ifndef EDITWIDGET_H
#define EDITWIDGET_H

#include <QShortcut>
#include <interfaces/imessagewidgets.h>
#include "ui_editwidget.h"

class EditWidget : 
  public QWidget,
  public IEditWidget
{
  Q_OBJECT;
  Q_INTERFACES(IEditWidget);
public:
  EditWidget(IMessageWidgets *AMessageWidgets, const Jid &AStreamJid, const Jid &AContactJid);
  ~EditWidget();
  virtual QWidget *instance() { return this; }
  virtual const Jid &streamJid() const { return FStreamJid; }
  virtual void setStreamJid(const Jid &AStreamJid);
  virtual const Jid &contactJid() const { return FContactJid; }
  virtual void setContactJid(const Jid &AContactJid);
  virtual QTextEdit *textEdit() const { return ui.medEditor; }
  virtual QTextDocument *document() const { return ui.medEditor->document(); }
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
  IMessageWidgets *FMessageWidgets;
private:
  Jid FStreamJid;
  Jid FContactJid;
  QShortcut *FSendShortcut;
};

#endif // EDITWIDGET_H
