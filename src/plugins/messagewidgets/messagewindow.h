#ifndef MESSAGEWINDOW_H
#define MESSAGEWINDOW_H

#include "../../definations/messagedataroles.h"
#include "../../definations/rosterindextyperole.h"
#include "../../interfaces/imessagewidgets.h"
#include "../../interfaces/imessageprocessor.h"
#include "../../interfaces/ixmppstreams.h"
#include "../../interfaces/ipresence.h"
#include "../../interfaces/istatusicons.h"
#include "../../interfaces/isettings.h"
#include "../../utils/errorhandler.h"
#include "ui_messagewindow.h"

class MessageWindow : 
  public QMainWindow,
  public IMessageWindow
{
  Q_OBJECT;
  Q_INTERFACES(IMessageWindow ITabWidget);
public:
  MessageWindow(IMessageWidgets *AMessageWidgets, const Jid& AStreamJid, const Jid &AContactJid, Mode AMode);
  virtual ~MessageWindow();
  virtual QMainWindow *instance() { return this; }
  //ITabWidget
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
  virtual QString subject() const { return ui.lneSubject->text(); }
  virtual void setSubject(const QString &ASubject);
  virtual QString threadId() const { return FCurrentThreadId; }
  virtual void setThreadId(const QString &AThreadId);
  virtual int nextCount() const { return FNextCount; }
  virtual void setNextCount(int ACount);
  virtual void updateWindow(const QIcon &AIcon, const QString &AIconText, const QString &ATitle);
signals:
  //ITabWidget
  virtual void windowShow();
  virtual void windowClose();
  virtual void windowChanged();
  virtual void windowDestroyed();
  //IMessageWindow
  virtual void messageReady();
  virtual void showNextMessage();
  virtual void replyMessage();
  virtual void forwardMessage();
  virtual void showChatWindow();
  virtual void streamJidChanged(const Jid &ABefour);
  virtual void contactJidChanged(const Jid &ABefour);
  virtual void windowClosed();
protected:
  void initialize();
  void saveWindowState();
  void loadWindowState();
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
private:
  Ui::MessageWindowClass ui;
  IInfoWidget *FInfoWidget;
  IViewWidget *FViewWidget;
  IEditWidget *FEditWidget;
  IReceiversWidget *FReceiversWidget;
  IToolBarWidget *FViewToolBarWidget;
  IToolBarWidget *FEditToolBarWidget;
private:
  IMessageWidgets *FMessageWidgets;
  ISettings *FSettings;
private:
  Mode FMode;
  int FNextCount;
  Jid FStreamJid;
  Jid FContactJid;
  QString FCurrentThreadId;
};

#endif // MESSAGEWINDOW_H
