#ifndef MESSAGEHANDLER_H
#define MESSAGEHANDLER_H

#include "../../definations/rosterindextyperole.h"
#include "../../interfaces/imessenger.h"
#include "../../interfaces/istatusicons.h"
#include "../../interfaces/ipresence.h"
#include "../../interfaces/irostersview.h"
#include "../../interfaces/imessagearchiver.h"
#include "../../utils/skin.h"

class MessageHandler : 
  public QObject,
  public IMessageHandler
{
  Q_OBJECT;
  Q_INTERFACES(IMessageHandler);
public:
  MessageHandler(IMessenger *FMessenger, QObject *AParent);
  ~MessageHandler();
  //IMessageHandler
  virtual bool openWindow(IRosterIndex *AIndex);
  virtual bool openWindow(const Jid &AStreamJid, const Jid &AContactJid, Message::MessageType AType);
  virtual bool checkMessage(const Message &AMessage);
  virtual bool notifyOptions(const Message &AMessage, QIcon &AIcon, QString &AToolTip, int &AFlags);
  virtual void receiveMessage(int AMessageId);
  virtual void showMessage(int AMessageId);
protected:
  void initialize();
  IMessageWindow *getMessageWindow(const Jid &AStreamJid, const Jid &AContactJid, IMessageWindow::Mode AMode);
  IMessageWindow *findMessageWindow(const Jid &AStreamJid, const Jid &AContactJid);
  void showMessageWindow(IMessageWindow *AWindow);
  void showNextNormalMessage(IMessageWindow *AWindow);
  void loadActiveNormalMessages(IMessageWindow *AWindow);
  void updateMessageWindow(IMessageWindow *AWindow);
  IChatWindow *getChatWindow(const Jid &AStreamJid, const Jid &AContactJid);
  IChatWindow *findChatWindow(const Jid &AStreamJid, const Jid &AContactJid);
  void showChatHistory(IChatWindow *AWindow);
  void showChatWindow(IChatWindow *AWindow);
  void removeActiveChatMessages(IChatWindow *AWindow);
  void updateChatWindow(IChatWindow *AWindow);
protected slots:
  void onChatMessageSend();
  void onChatWindowActivated();
  void onChatInfoFieldChanged(IInfoWidget::InfoField AField, const QVariant &AValue);
  void onChatWindowDestroyed();
  void onMessageWindowSend();
  void onMessageWindowShowNext();
  void onMessageWindowReply();
  void onMessageWindowForward();
  void onMessageWindowShowChat();
  void onMessageWindowDestroyed();
  void onPresenceReceived(IPresence *APresence, const IPresenceItem &APresenceItem);
  void onStatusIconsChanged();
private:
  IMessenger *FMessenger;
  IStatusIcons *FStatusIcons;
  IPresencePlugin *FPresencePlugin;
  IMessageArchiver *FMessageArchiver;
private:
  QList<IMessageWindow *> FMessageWindows;
  QList<IChatWindow *> FChatWindows;
  QMultiHash<IMessageWindow *, int> FActiveNormalMessages;
  QMultiHash<IChatWindow *,int> FActiveChatMessages;
};

#endif // MESSAGEHANDLER_H
