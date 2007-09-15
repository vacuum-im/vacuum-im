#ifndef MESSAGEWINDOW_H
#define MESSAGEWINDOW_H

#include "../../interfaces/imessenger.h"
#include "../../utils/jid.h"
#include "../../utils/message.h"
#include "ui_messagewindow.h"

using namespace Ui;

class MessageWindow : 
  virtual public QMainWindow, 
  public MessageWindowClass/*,
  public IMessageWindow*/
{
  Q_OBJECT;
  //Q_INTERFACES(IMessageWindow);

public:
  MessageWindow(QWidget *AParent = NULL);
  ~MessageWindow();

  //IMessageWindow
  virtual const Jid &streamJid() const { return FStreamJid; }
  virtual void setStreamJid(const Jid &AStreamJid);
  virtual const Jid &contactJid() const { return FContactJid; }
  virtual void setContactJid(const Jid &AContactJid);
  virtual Message *message();
  virtual QMenuBar *menuBar() const { return mnbMenu; }
  virtual QLineEdit *accountEdit() const { return lneAccount; }
  virtual QLineEdit *contactEdit() const { return lneContact; }
  virtual QLineEdit *dateTimeEdit() const { return lneDateTime; }
  virtual QLineEdit *subjectEdit() const { return lneSubject; }
  virtual QToolBar *toolBar() const{ return FToolBar; }
  virtual QTextEdit *messageEdit() const { return tedMessage; }
  virtual QButtonGroup *buttonGroup() const { return FButtonGroup; }
  virtual QWidget *accountWidget() const { return wdtAccount; }
  virtual QWidget *contactWidget() const { return wdtContact; }
  virtual QWidget *toolBarsWidget() const { return wdtToolbars; }
  virtual QWidget *subjectWidget() const { return wdtSubject; }
  virtual QWidget *messageWidget() const { return wdtMessage; }
signals:
  virtual void send(IMessageWindow *AWindow);
  virtual void reply(IMessageWindow *AWindow);
  virtual void quote(IMessageWindow *AWindow);
  virtual void chat(IMessageWindow *AWindow);
  virtual void next(IMessageWindow *AWindow);
  virtual void close(IMessageWindow *AWindow);
private:
  Jid FStreamJid;
  Jid FContactJid;
  Message *FMessage;
  QToolBar *FToolBar;
  QButtonGroup *FButtonGroup;
};

#endif // MESSAGEWINDOW_H
