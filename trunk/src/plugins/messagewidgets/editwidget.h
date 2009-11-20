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
  virtual const Jid &streamJid() const;
  virtual void setStreamJid(const Jid &AStreamJid);
  virtual const Jid &contactJid() const;
  virtual void setContactJid(const Jid &AContactJid);
  virtual QTextEdit *textEdit() const;
  virtual QTextDocument *document() const;
  virtual void sendMessage();
  virtual void clearEditor();
  virtual bool autoResize() const;
  virtual void setAutoResize(bool AResize);
  virtual int minimumLines() const;
  virtual void setMinimumLines(int ALines);
  virtual QKeySequence sendKey() const;
  virtual void setSendKey(const QKeySequence &AKey);
signals:
  virtual void keyEventReceived(QKeyEvent *AKeyEvent, bool &AHook);
  virtual void messageAboutToBeSend();
  virtual void messageReady();
  virtual void editorCleared();
  virtual void streamJidChanged(const Jid &ABefour);
  virtual void contactJidChanged(const Jid &ABefour);
  virtual void autoResizeChanged(bool AResize);
  virtual void minimumLinesChanged(int ALines);
  virtual void sendKeyChanged(const QKeySequence &AKey);
protected:
  virtual bool eventFilter(QObject *AWatched, QEvent *AEvent);
protected slots:
  void onShortcutActivated();
  void onEditorAutoResizeChanged(bool AResize);
  void onEditorMinimumLinesChanged(int ALines);
  void onEditorSendKeyChanged(const QKeySequence &AKey);
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
