#ifndef EDITWIDGET_H
#define EDITWIDGET_H

#include <QShortcut>
#include <definations/optionvalues.h>
#include <definations/resources.h>
#include <definations/menuicons.h>
#include <interfaces/imessagewidgets.h>
#include <utils/options.h>
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
  virtual bool sendButtonVisible() const;
  virtual void setSendButtonVisible(bool AVisible);
signals:
  void keyEventReceived(QKeyEvent *AKeyEvent, bool &AHook);
  void messageAboutToBeSend();
  void messageReady();
  void editorCleared();
  void streamJidChanged(const Jid &ABefour);
  void contactJidChanged(const Jid &ABefour);
  void autoResizeChanged(bool AResize);
  void minimumLinesChanged(int ALines);
  void sendKeyChanged(const QKeySequence &AKey);
protected:
  virtual bool eventFilter(QObject *AWatched, QEvent *AEvent);
protected:
  void appendMessageToBuffer();
  void showBufferedMessage();
  void showNextBufferedMessage();
  void showPrevBufferedMessage();
protected slots:
  void onShortcutActivated();
  void onSendButtonCliked(bool);
  void onOptionsChanged(const OptionsNode &ANode);
private:
  Ui::EditWidgetClass ui;
private:
  IMessageWidgets *FMessageWidgets;
private:
  int FBufferPos;
  Jid FStreamJid;
  Jid FContactJid;
  QShortcut *FSendShortcut;
  QList<QString> FBuffer;
};

#endif // EDITWIDGET_H
