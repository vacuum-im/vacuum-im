#ifndef MESSAGEWINDOW_H
#define MESSAGEWINDOW_H

#include "../../definations/messagedataroles.h"
#include "../../definations/rosterindextyperole.h"
#include "../../interfaces/imessenger.h"
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

  //IMessageWindow
  virtual QWidget *instance() { return this; }
  virtual const Jid &streamJid() const { return FStreamJid; }
  virtual const Jid &contactJid() const { return FContactJid; }
  virtual void addTabWidget(QWidget *AWidget);
  virtual void removeTabWidget(QWidget *AWidget);
  virtual IInfoWidget *infoWidget() const { return FInfoWidget; }
  virtual IViewWidget *viewWidget() const { return FViewWidget; }
  virtual IEditWidget *editWidget() const { return FEditWidget; }
  virtual IReceiversWidget *receiversWidget() const { return FReceiversWidget; }
  virtual IToolBarWidget *viewToolBarWidget() const { return FViewToolBarWidget; }
  virtual IToolBarWidget *editToolBarWidget() const { return FEditToolBarWidget; }
  virtual Mode mode() const { return FMode; }
  virtual void setMode(Mode AMode);
  virtual void showWindow();
  virtual void closeWindow();
signals:
  virtual void streamJidChanged(const Jid &ABefour);
  virtual void contactJidChanged(const Jid &ABefour);
  virtual void windowShow();
  virtual void windowClose();
  virtual void windowChanged();
  virtual void windowClosed();
  virtual void windowDestroyed();
protected:
  void initialize();
  void saveWindowState();
  void loadWindowState();
  void loadActiveMessages();
  void removeActiveMessage(int AMessageId);
  void updateWindow();
  void showMessage(const Message &AMessage);
  void showErrorMessage(const Message &AMessage);
  void showNextOrClose();
  void setContactJid(const Jid &AContactJid);
protected:
  virtual void showEvent(QShowEvent *AEvent);
  virtual void closeEvent(QCloseEvent *AEvent);
protected slots:
  void onMessageReceived(const Message &AMessage);
  void onStreamJidChanged(IXmppStream *AXmppStream, const Jid &ABefour);
  void onPresenceItem(IPresenceItem *APresenceItem);
  void onStatusIconsChanged();
  void onSendButtonClicked();
  void onReplyButtonClicked();
  void onForwardButtonClicked();
  void onChatButtonClicked();
  void onNextButtonClicked();
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
  IPresence *FPresence;
  IStatusIcons *FStatusIcons;
  ISettings *FSettings;
private:
  Jid FStreamJid;
  Jid FContactJid;
  Mode FMode;
  int FMessageId;
  Message FMessage;
  QString FCurrentThreadId;
  QString FSettingsValueNS;
  QList<int> FActiveMessages;
};

#endif // MESSAGEWINDOW_H
