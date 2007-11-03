#ifndef EDITWIDGET_H
#define EDITWIDGET_H

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
  virtual ToolBarChanger *toolBarChanger() const { return FToolBarChanger; }
  virtual void addToolBar(QToolBar *AToolBar);
  virtual void removeToolBar(QToolBar *AToolBar);
  virtual QTextEdit *textEdit() const { return ui.tedEditor; }
  virtual QTextDocument *document() const { return ui.tedEditor->document(); }
  virtual void sendMessage();
  virtual int sendMessageKey() const { return FSendMessageKey; }
  virtual void setSendMessageKey(int AKey);
  virtual void clearEditor();
signals:
  virtual void messageAboutToBeSend();
  virtual void messageReady();
  virtual void streamJidChanged(const Jid &ABefour);
  virtual void contactJidChanged(const Jid &ABefour);
  virtual void sendMessageKeyChanged(int AKey);
  virtual void editorCleared();
protected:
  virtual bool eventFilter(QObject *AWatched, QEvent *AEvent);
private:
  Ui::EditWidgetClass ui;
  QVBoxLayout *FToolBarLayout;
  ToolBarChanger *FToolBarChanger;
private:
  IMessenger *FMessenger;
private:
  Jid FStreamJid;
  Jid FContactJid;
  int FSendMessageKey;
};

#endif // EDITWIDGET_H
