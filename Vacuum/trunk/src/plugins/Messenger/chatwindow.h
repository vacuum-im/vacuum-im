#ifndef CHATWINDOW_H
#define CHATWINDOW_H

#include "../../definations/messagedataroles.h"
#include "../../interfaces/imessenger.h"
#include "../../interfaces/ipresence.h"
#include "../../interfaces/istatusicons.h"
#include "../../interfaces/isettings.h"
#include "../../utils/message.h"
#include "../../utils/skin.h"
#include "ui_chatwindow.h"

class ChatWindow : 
  public IChatWindow
{
  Q_OBJECT;
  Q_INTERFACES(IChatWindow ITabWidget);
public:
  ChatWindow(IMessenger *AMessenger, const Jid& AStreamJid, const Jid &AContactJid);
  ~ChatWindow();
  virtual QWidget *instance() { return this; }
  virtual const Jid &streamJid() const { return FStreamJid; }
  virtual const Jid &contactJid() const { return FContactJid; }
  virtual IInfoWidget *infoWidget() const { return FInfoWidget; }
  virtual IViewWidget *viewWidget() const { return FViewWidget; }
  virtual IEditWidget *editWidget() const { return FEditWidget; }
  virtual void showWindow();
  virtual void closeWindow();
signals:
  virtual void windowShow();
  virtual void windowClose();
  virtual void streamJidChanged(const Jid &ABefour);
  virtual void contactJidChanged(const Jid &ABefour);
  virtual void windowChanged();
  virtual void windowClosed();
  virtual void windowDestroyed();
protected:
  void initialize();
  void saveWindowState();
  void loadWindowState();
  void loadActiveMessages();
  void removeActiveMessages();
  void updateWindow();
protected:
  virtual bool event(QEvent *AEvent);
  virtual void showEvent(QShowEvent *AEvent);
  virtual void closeEvent(QCloseEvent *AEvent);
protected slots:
  void onMessageReady();
  void onMessageReceived(const Message &AMessage);
  void onStreamJidChanged(IXmppStream *AXmppStream, const Jid &ABefour);
  void onPresenceItem(IPresenceItem *APresenceItem);
  void onStatusIconsChanged();
  void onInfoFieldChanged(IInfoWidget::InfoField AField, const QVariant &AValue);
  void onDefaultChatFontChanged(const QFont &AFont);
private:
  Ui::ChatWindowClass ui;
private:
  IMessenger *FMessenger;
  IPresence *FPresence;
  IStatusIcons *FStatusIcons;
  ISettings *FSettings;
private:
  IInfoWidget *FInfoWidget;
  IViewWidget *FViewWidget;
  IEditWidget *FEditWidget;
private:
  int FOptions;
  bool FSplitterLoaded;
  Jid FStreamJid;
  Jid FContactJid;
  QString FLastStatusShow;
  QString FSettingsValueNS;
  QList<int> FActiveMessages;
};

#endif // CHATWINDOW_H
