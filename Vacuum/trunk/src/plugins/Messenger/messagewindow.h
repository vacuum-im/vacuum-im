#ifndef MESSAGEWINDOW_H
#define MESSAGEWINDOW_H

#include "../../definations/messagedataroles.h"
#include "../../definations/rosterindextyperole.h"
#include "../../interfaces/imessenger.h"
#include "../../interfaces/ixmppstreams.h"
#include "../../interfaces/ipresence.h"
#include "../../interfaces/istatusicons.h"
#include "../../interfaces/isettings.h"
#include "../../utils/message.h"
#include "../../utils/jid.h"
#include "../../utils/errorhandler.h"
#include "ui_messagewindow.h"

class MessageWindow : 
  public IMessageWindow
{
  Q_OBJECT;
  Q_INTERFACES(IMessageWindow);
public:
  MessageWindow(IMessenger *AMessenger, const Jid& AStreamJid, const Jid &AContactJid, Mode AMode);
  ~MessageWindow();
  //ITabWidget
  virtual QWidget *instance() { return this; }
  virtual void showWindow();
  virtual void closeWindow();
  //IMessageWindow
  virtual const Jid &streamJid() const { return FStreamJid; }
  virtual const Jid &contactJid() const { return FContactJid; }
  virtual void setContactJid(const Jid &AContactJid);
  virtual void addTabWidget(QWidget *AWidget);
  virtual void setCurrentTabWidget(QWidget *AWidget);
  virtual void removeTabWidget(QWidget *AWidget);
  virtual IInfoWidget *infoWidget() const { return FInfoWidget; }
  virtual IViewWidget *viewWidget() const { return FViewWidget; }
  virtual IEditWidget *editWidget() const { return FEditWidget; }
  virtual IReceiversWidget *receiversWidget() const { return FReceiversWidget; }
  virtual IToolBarWidget *viewToolBarWidget() const { return FViewToolBarWidget; }
  virtual IToolBarWidget *editToolBarWidget() const { return FEditToolBarWidget; }
  virtual Mode mode() const { return FMode; }
  virtual void setMode(Mode AMode);
  virtual Message currentMessage() const { return FMessage; }
  virtual QString subject() const { return ui.lneSubject->text(); }
  virtual void setSubject(const QString &ASubject);
  virtual QString threadId() const { return FCurrentThreadId; }
  virtual void setThreadId(const QString &AThreadId);
  virtual int nextCount() const { return FNextCount; }
  virtual void setNextCount(int ACount);
  virtual void showMessage(const Message &AMessage);
  virtual void updateWindow(const QIcon &AIcon, const QString &AIconText, const QString &ATitle);
signals:
  virtual void messageReady();
  virtual void showNextMessage();
  virtual void replyMessage();
  virtual void forwardMessage();
  virtual void showChatWindow();
  virtual void streamJidChanged(const Jid &ABefour);
  virtual void contactJidChanged(const Jid &ABefour);
  virtual void windowClosed();
  //ITabWidget
  virtual void windowShow();
  virtual void windowClose();
  virtual void windowChanged();
  virtual void windowDestroyed();
protected:
  void initialize();
  void saveWindowState();
  void loadWindowState();
  void showErrorMessage(const Message &AMessage);
protected:
  virtual void showEvent(QShowEvent *AEvent);
  virtual void closeEvent(QCloseEvent *AEvent);
protected slots:
  void onStreamJidChanged(IXmppStream *AXmppStream, const Jid &ABefour);
  void onSendButtonClicked();
  void onNextButtonClicked();
  void onReplyButtonClicked();
  void onForwardButtonClicked();
  void onChatButtonClicked();
  void onReceiversChanged(const Jid &AReceiver);
  void onDefaultMessageFontChanged(const QFont &AFont);
private:
  Ui::MessageWindowClass ui;
  IInfoWidget *FInfoWidget;
  IViewWidget *FViewWidget;
  IEditWidget *FEditWidget;
  IReceiversWidget *FReceiversWidget;
  IToolBarWidget *FViewToolBarWidget;
  IToolBarWidget *FEditToolBarWidget;
private:
  IMessenger *FMessenger;
  ISettings *FSettings;
private:
  int FNextCount;
  Jid FStreamJid;
  Jid FContactJid;
  Mode FMode;
  Message FMessage;
  QString FCurrentThreadId;
};

#endif // MESSAGEWINDOW_H
