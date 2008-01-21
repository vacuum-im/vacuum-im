#ifndef MULTIUSERCHAT_H
#define MULTIUSERCHAT_H

#include "../../definations/multiuserdataroles.h"
#include "../../definations/namespaces.h"
#include "../../definations/stanzahandlerpriority.h"
#include "../../interfaces/imultiuserchat.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/ixmppstreams.h"
#include "../../interfaces/istanzaprocessor.h"
#include "../../interfaces/ipresence.h"
#include "multiuser.h"

class MultiUserChat : 
  public QObject,
  public IMultiUserChat,
  public IStanzaHandler
{
  Q_OBJECT;
  Q_INTERFACES(IMultiUserChat IStanzaHandler);
public:
  MultiUserChat(IMultiUserChatPlugin *AChatPlugin, IMessenger *AMessenger, const Jid &AStreamJid, const Jid &ARoomJid, 
    const QString &ANickName, const QString &APassword, QObject *AParent);
  ~MultiUserChat();
  virtual QObject *instance() { return this; }
  //IStanzaHandler
  virtual bool editStanza(int /*AHandlerId*/, const Jid &/*AStreamJid*/, Stanza * /*AStanza*/, bool &/*AAccept*/) { return false; }
  virtual bool readStanza(int AHandlerId, const Jid &AStreamJid, const Stanza &AStanza, bool &AAccept);
  //IMultiUserChar
  virtual Jid streamJid() const { return FStreamJid; }
  virtual Jid roomJid() const { return FRoomJid; }
  virtual bool isOpen() const;
  virtual QString nickName() const { return FNickName; }
  virtual void setNickName(const QString &ANick);
  virtual QString password() const { return FPassword; }
  virtual void setPassword(const QString &APassword);
  virtual bool autoPresence() const { return FAutoPresence; }
  virtual void setAutoPresence(bool AAuto);
  virtual int show() const { return FShow; }
  virtual QString status() const { return FStatus; }
  virtual void setPresence(int AShow, const QString &AStatus);
  virtual QString subject() const { return FTopic; }
  virtual void setSubject(const QString &ASubject);
  virtual bool sendMessage(const Message &AMessage, const QString &AToNick = "");
  virtual QList<int> statusCodes() const { return FStatusCodes; }
  virtual IMultiUser *mainUser() const { return FMainUser; }
  virtual IMultiUser *userByNick(const QString &ANick) const;
  virtual QList<IMultiUser *> allUsers() const;
signals:
  virtual void chatOpened();
  virtual void userPresence(IMultiUser *AUser, int AShow, const QString &AStatus);
  virtual void userDataChanged(IMultiUser *AUser, int ARole, const QVariant &ABefour, const QVariant &AAfter);
  virtual void userNickChanged(IMultiUser *AUser, const QString &AOldNick, const QString &ANewNick);
  virtual void presenceChanged(int AShow, const QString &AStatus);
  virtual void topicChanged(const QString &ATopic);
  virtual void messageReceive(const QString &ANick, Message &AMessage);
  virtual void messageReceived(const QString &ANick, const Message &AMessage);
  virtual void messageSend(Message &AMessage);
  virtual void messageSent(const Message &AMessage);
  virtual void chatError(const QString &ANick, const QString &AError);
  virtual void chatClosed();
  virtual void chatDestroyed();
  virtual void streamJidChanged(const Jid &ABefour, const Jid &AAfter);
protected:
  bool processMessage(const Stanza &AStanza);
  bool processPresence(const Stanza &AStanza);
  bool initialize(IPluginManager *APluginManager);
  void clearUsers();
protected slots:
  void onMessageReceive(Message &AMessage);
  void onMessageReceived(const Message &AMessage);
  void onMessageSend(Message &AMessage);
  void onMessageSent(const Message &AMessage);
  void onUserDataChanged(int ARole, const QVariant &ABefour, const QVariant &AAfter);
  void onSelfPresence(int AShow, const QString &AStatus, qint8 APriority, const Jid &AToJid);
  void onStreamJidChanged(IXmppStream *AXmppStream, const Jid &ABefour);
  void onStreamAboutToClose(IXmppStream *AXmppStream);
  void onStreamClosed(IXmppStream *AXmppStream);
private:
  IMessenger *FMessenger;
  IPresence *FPresence;
  IXmppStream *FXmppStream;
  IStanzaProcessor *FStanzaProcessor;
  IMultiUserChatPlugin *FChatPlugin;
private:
  int FPresenceHandler;
  int FMessageHandler;
  bool FAutoPresence;
  MultiUser *FMainUser;
  Jid FRoomJid;
  Jid FStreamJid;
  QString FNickName;
  QString FPassword;
  int FShow;
  QString FStatus;
  QList<int> FStatusCodes;
  QString FTopic;
  QHash<QString, MultiUser *> FUsers;
};

#endif // MULTIUSERCHAT_H
