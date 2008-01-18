#ifndef MULTIUSERCHATWINDOW_H
#define MULTIUSERCHATWINDOW_H

#include "../../definations/messagehandlerorders.h"
#include "../../definations/multiuserdataroles.h"
#include "../../interfaces/imultiuserchat.h"
#include "../../interfaces/ixmppstreams.h"
#include "../../interfaces/irostersview.h"
#include "../../interfaces/isettings.h"
#include "../../interfaces/istatusicons.h"
#include "../../interfaces/iaccountmanager.h"
#include "../../utils/skin.h"
#include "../../utils/menu.h"
#include "ui_multiuserchatwindow.h"

class MultiUserChatWindow : 
  public IMultiUserChatWindow,
  public IMessageHandler
{
  Q_OBJECT;
  Q_INTERFACES(IMultiUserChatWindow ITabWidget IMessageHandler);
public:
  MultiUserChatWindow(IMessenger *AMessenger, IMultiUserChat *AMultiChat);
  ~MultiUserChatWindow();
  //ITabWidget
  virtual QWidget *instance() { return this; }
  virtual void showWindow();
  virtual void closeWindow();
  //IMessageHandler
  virtual bool openWindow(IRosterIndex *AIndex);
  virtual bool openWindow(const Jid &AStreamJid, const Jid &AContactJid, Message::MessageType AType);
  virtual bool checkMessage(const Message &AMessage);
  virtual bool notifyOptions(const Message &AMessage, QIcon &AIcon, QString &AToolTip, int &AFlags);
  virtual void receiveMessage(int AMessageId);
  virtual void showMessage(int AMessageId);
  //IMultiUserChatWindow
  virtual Jid streamJid() const { return FMultiChat->streamJid(); }
  virtual Jid roomJid() const { return FMultiChat->roomJid(); }
  virtual bool isActive() const { return isVisible() && isActiveWindow(); }
  virtual IViewWidget *viewWidget() const { return FViewWidget; }
  virtual IEditWidget *editWidget() const { return FEditWidget; }
  virtual IToolBarWidget *toolBarWidget() const { return FToolBarWidget; }
  virtual IMultiUserChat *multiUserChat() const { return FMultiChat; }
  virtual IChatWindow *openChatWindow(const Jid &AContactJid); 
  virtual IChatWindow *findChatWindow(const Jid &AContactJid) const;
  virtual void exitMultiUserChat();
signals:
  virtual void windowShow();
  virtual void windowClose();
  virtual void windowActivated();
  virtual void windowChanged();
  virtual void windowClosed();
  virtual void windowDestroyed();
  virtual void chatWindowCreated(IChatWindow *AWindow);
  virtual void chatWindowDestroyed(IChatWindow *AWindow);
  virtual void multiUserContextMenu(IMultiUser *AUser, Menu *AMenu);
protected:
  void initialize();
  void saveWindowState();
  void loadWindowState();
  void showServiceMessage(const QString &AMessage);
  void setViewColorForUser(IMultiUser *AUser);
  void setRoleColorForUser(IMultiUser *AUser);
  void setAffilationLineForUser(IMultiUser *AUser);
  void setToolTipForUser(IMultiUser *AUser);
protected:
  void updateWindow();
  void updateListItem(const Jid &AContactJid);
  void removeActiveMessages();
  IChatWindow *getChatWindow(const Jid &AContactJid);
  void showChatWindow(IChatWindow *AWindow);
  void removeActiveChatMessages(IChatWindow *AWindow);
  void updateChatWindow(IChatWindow *AWindow);
protected:
  virtual bool event(QEvent *AEvent);
  virtual void showEvent(QShowEvent *AEvent);
  virtual void closeEvent(QCloseEvent *AEvent);
  virtual bool eventFilter(QObject *AObject, QEvent *AEvent);
protected slots:
  void onChatOpened();
  void onUserPresence(IMultiUser *AUser, int AShow, const QString &AStatus);
  void onUserDataChanged(IMultiUser *AUser, int ARole, const QVariant &ABefour, const QVariant &AAfter);
  void onUserNickChanged(IMultiUser *AUser, const QString &AOldNick, const QString &ANewNick);
  void onPresenceChanged(int AShow, const QString &AStatus);
  void onTopicChanged(const QString &ATopic);
  void onMessageReceived(const QString &ANick, const Message &AMessage);
  void onChatError(const QString &ANick, const QString &AError);
  void onChatClosed();
protected slots:
  void onMessageSend();
  void onWindowActivated();
  void onChatMessageSend();
  void onChatWindowActivated();
  void onChatWindowClosed();
  void onChatWindowDestroyed();
protected slots:
  void onListItemActivated(QListWidgetItem *AItem);
  void onDefaultChatFontChanged(const QFont &AFont);
  void onStatusIconsChanged();
  void onAccountChanged(const QString &AName, const QVariant &AValue);  
  void onStreamJidChanged(const Jid &ABefour, const Jid &AAfter);
  void onExitMultiUserChat(bool);
private:
  Ui::MultiUserChatWindowClass ui;
private:
  IMessenger *FMessenger;
  ISettings *FSettings;
  IStatusIcons *FStatusIcons;
  IMultiUserChat *FMultiChat;
private:
  IViewWidget *FViewWidget;
  IEditWidget *FEditWidget;
  IToolBarWidget *FToolBarWidget;
private:
  bool FSplitterLoaded;
  bool FExitOnChatClosed;
  QList<int> FActiveMessages;
  QList<IChatWindow *> FChatWindows;
  QMultiHash<IChatWindow *,int> FActiveChatMessages;
  QHash<IMultiUser *, QListWidgetItem *> FUsers;
  QList<QColor> FColorQueue;
  QHash<QString,QString> FColorLastOwner;
};

#endif // MULTIUSERCHATWINDOW_H
