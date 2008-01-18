#include "multiuserchat.h"

#define SHC_PRESENCE        "/presence"
#define SHC_MESSAGE         "/message"

MultiUserChat::MultiUserChat(IMultiUserChatPlugin *AChatPlugin, IMessenger *AMessenger, const Jid &AStreamJid, const Jid &ARoomJid, 
                             const QString &ANickName, const QString &APassword, QObject *AParent) : QObject(AParent)
{
  FPresence = NULL;
  FXmppStream = NULL;
  FStanzaProcessor = NULL;
  FMainUser = NULL;

  FAutoPresence = false;
  FPresenceHandler = -1;
  FMessageHandler = -1;

  FMessenger = AMessenger;
  FChatPlugin = AChatPlugin;
  FRoomJid = ARoomJid;
  FStreamJid = AStreamJid;
  FNickName = ANickName;
  FPassword = APassword;
  FShow = IPresence::Offline;

  initialize(FChatPlugin->pluginManager());
}

MultiUserChat::~MultiUserChat()
{
  clearUsers();
  if (FStanzaProcessor)
  {
    FStanzaProcessor->removeHandler(FPresenceHandler);
    FStanzaProcessor->removeHandler(FMessageHandler);
  }
  emit chatDestroyed();
}

bool MultiUserChat::readStanza(int AHandlerId, const Jid &AStreamJid, const Stanza &AStanza, bool &AAccept)
{
  Jid fromJid = AStanza.from();
  Jid toJid = AStanza.to();
  if ( (fromJid && FRoomJid) && (AStreamJid == FStreamJid) )
  {
    AAccept = true;
    if (AHandlerId == FPresenceHandler)
    {
      return processPresence(AStanza);
    }
    else if (AHandlerId == FMessageHandler)
    {
      return processMessage(AStanza);
    }
  }
  return false;
}

bool MultiUserChat::isOpen() const
{
  return FMainUser!=NULL;
}

void MultiUserChat::setNickName(const QString &ANick)
{
  if (isOpen())
  {
    if (userByNick(ANick) == NULL)
    {
      Jid userJid = FRoomJid;
      userJid.setResource(ANick);
      Stanza presence("presence");
      presence.setTo(userJid.eFull());
      FStanzaProcessor->sendStanzaOut(FStreamJid,presence);
    }
  }
  else
    FNickName = ANick;
}

void MultiUserChat::setPassword(const QString &APassword)
{
  FPassword = APassword;
}

IMultiUser *MultiUserChat::userByNick(const QString &ANick) const
{
  return FUsers.value(ANick,NULL);
}

QList<IMultiUser *> MultiUserChat::allUsers() const
{
  QList<IMultiUser *> result;
  foreach(MultiUser *user, FUsers)
    result.append(user);
  return result;
}

void MultiUserChat::setAutoPresence(bool AAuto) 
{
  if (FAutoPresence != AAuto)
  {
    FAutoPresence = AAuto;
    if (FPresence && FAutoPresence)
      setPresence(FPresence->show(),FPresence->status());
  }
}

void MultiUserChat::setPresence(int AShow, const QString &AStatus)
{
  if (FStanzaProcessor && FXmppStream && FXmppStream->isOpen())
  {
    Jid userJid = FRoomJid;
    userJid.setResource(FNickName);

    Stanza presence("presence");
    presence.setTo(userJid.eFull());

    QString showText;
    switch (AShow)
    {
    case IPresence::Chat: showText = "chat"; break; 
    case IPresence::Away: showText = "away"; break;
    case IPresence::DoNotDistrib: showText = "dnd"; break; 
    case IPresence::ExtendedAway: showText = "xa"; break;
    }
    if (AShow == IPresence::Offline || AShow == IPresence::Error || AShow == IPresence::Invisible)
      presence.setType("unavailable");
    else if (!showText.isEmpty())
      presence.addElement("show").appendChild(presence.createTextNode(showText));
    if (!AStatus.isEmpty()) 
      presence.addElement("status").appendChild(presence.createTextNode(AStatus));
    
    if (!FPassword.isEmpty() && !isOpen() && FShow!=IPresence::Offline && FShow!=IPresence::Error)
    {
      QDomElement xelem = presence.addElement("x",NS_MUC);
      xelem.appendChild(presence.createElement("password")).appendChild(presence.createTextNode(FPassword));
    }

    FStanzaProcessor->sendStanzaOut(FStreamJid,presence);
  }
  else if (AShow == IPresence::Offline || AShow == IPresence::Error)
  {
    clearUsers();
    FShow = AShow;
    FStatus = AStatus;
    emit presenceChanged(FShow,FStatus);
  }
}

void MultiUserChat::setSubject(const QString &ASubject)
{
  if (isOpen())
  {
    Message message;
    message.setSubject(ASubject);
    sendMessage(message);
  }
}

bool MultiUserChat::sendMessage(const Message &AMessage, const QString &AToNick)
{
  if (isOpen())
  {
    Message message = AMessage;
    Jid toJid = FRoomJid;
    toJid.setResource(AToNick);
    message.setTo(toJid.eFull());
    if (AToNick.isEmpty())
      message.setType(Message::GroupChat);
    
    if (FMessenger == NULL)
    {
      emit messageSend(message);
      if (FStanzaProcessor->sendStanzaOut(FStreamJid, message.stanza()))
      {
        emit messageSent(message);
        return true;
      }
    }
    else
      return FMessenger->sendMessage(message,FStreamJid);
  }
  return false;
}

bool MultiUserChat::processMessage(const Stanza &AStanza)
{
  Jid fromJid = AStanza.from();
  QString fromNick = fromJid.resource();

  Message message(AStanza);

  if (FMessenger == NULL)
    emit messageReceive(fromNick,message);

  if (AStanza.type() == "error")
  {
    ErrorHandler err(AStanza.element());
    emit chatError(fromNick,err.message());
  }
  else if (!message.subject().isEmpty() && message.type() == Message::GroupChat)
  {
    FTopic = message.subject();
    emit topicChanged(FTopic);
  }
  else
    emit messageReceived(fromNick,message);

  return true;
}

bool MultiUserChat::processPresence(const Stanza &AStanza)
{
  Jid fromJid = AStanza.from();
  QString fromNick = fromJid.resource();
  if (AStanza.type().isEmpty())
  {
    QDomElement xelem = AStanza.firstElement("x",NS_MUC_USER);
    QDomElement itemElem = xelem.firstChildElement("item");
    if (!fromNick.isEmpty() && !itemElem.isNull())
    {
      int show;
      QString showText = AStanza.firstElement("show").text();
      if (showText.isEmpty())
        show = IPresence::Online;
      else if (showText == "chat")
        show = IPresence::Chat;
      else if (showText == "away")
        show = IPresence::Away;
      else if (showText == "dnd")
        show = IPresence::DoNotDistrib;
      else if (showText == "xa")
        show = IPresence::ExtendedAway;
      else 
        show = IPresence::Error;

      QString status = AStanza.firstElement("status").text();
      Jid realJid = itemElem.attribute("jid");
      QString role = itemElem.attribute("role");
      QString affiliation = itemElem.attribute("affiliation");

      MultiUser *user = FUsers.value(fromNick);
      if (!user)
      {
        user = new MultiUser(FRoomJid,fromNick,this);
        user->setData(MUDR_STREAMJID,FStreamJid.full());
        connect(user->instance(),SIGNAL(dataChanged(int,const QVariant &, const QVariant &)),
          SLOT(onUserDataChanged(int,const QVariant &, const QVariant &)));
        FUsers.insert(fromNick,user);
      }
      user->setData(MUDR_SHOW,show);
      user->setData(MUDR_STATUS,status);
      user->setData(MUDR_REALJID,realJid.full());
      user->setData(MUDR_ROLE,role);
      user->setData(MUDR_AFFILIATION,affiliation);

      if (!isOpen())
      {
        QList<int> statusCodes;
        QDomElement statusElem = xelem.firstChildElement("status");
        while(!statusElem.isNull())
        {
          statusCodes.append(statusElem.attribute("code").toInt());
          statusElem = statusElem.nextSiblingElement("status");
        }
        if (fromNick == FNickName || statusCodes.contains(210))
        {
          FNickName = fromNick;
          FMainUser = user;
          FStatusCodes = statusCodes;
          emit chatOpened();
        }
      }

      if (user == FMainUser)
      {
        FShow = show;
        FStatus = status;
        emit presenceChanged(FShow,FStatus);
      }

      emit userPresence(user,show,status);
      return true;
    }
  }
  else if (AStanza.type() == "unavailable")
  {
    MultiUser *user = FUsers.value(fromNick);
    if (user)
    {
      QDomElement xelem = AStanza.firstElement("x",NS_MUC_USER);
      QString newNick = xelem.firstChildElement("item").attribute("nick");
      
      QList<int> statusCodes;
      QDomElement statusElem = xelem.firstChildElement("status");
      while(!statusElem.isNull())
      {
        statusCodes.append(statusElem.attribute("code").toInt());
        statusElem = statusElem.nextSiblingElement("status");
      }

      if (newNick.isEmpty())
      {
        int show = IPresence::Offline;
        QString status = AStanza.firstElement("status").text();
        user->setData(MUDR_SHOW,show);
        user->setData(MUDR_STATUS,status);
        emit userPresence(user,show,status);
        FUsers.remove(fromNick);
        if (user == FMainUser)
        {
          clearUsers();
          FShow = show;
          FStatus = status;
          emit presenceChanged(FShow,FStatus);
          emit chatClosed();
        }
        else
          delete user;
      }
      else
      {
        FUsers.remove(fromNick);
        FUsers.insert(newNick,user);
        user->setNickName(newNick);
        emit userNickChanged(user,fromNick,newNick);
        
        if (user == FMainUser)
          FNickName = newNick;
      }
      return true;
    }
  }
  else if (AStanza.type() == "error")
  {
    ErrorHandler err(AStanza.element());
    MultiUser *user = FUsers.value(fromNick);
    if (user)
    {
      user->setData(MUDR_SHOW,IPresence::Error);
      user->setData(MUDR_STATUS,err.message());
      emit userPresence(user,IPresence::Error,err.message());
    }

    emit chatError(fromNick,err.message());
    
    if (fromNick == FNickName)
    {
      clearUsers();
      FShow = IPresence::Error;
      FStatus = err.message();
      emit presenceChanged(FShow,FStatus);
      emit chatClosed();
    }
    else
    {
      FUsers.remove(fromNick);
      delete user;
    }
  }
  return false;
}

bool MultiUserChat::initialize(IPluginManager *APluginManager)
{
  IPlugin *plugin = APluginManager->getPlugins("IStanzaProcessor").value(0,NULL);
  if (plugin) 
  {
    FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());
    if (FStanzaProcessor)
    {
      FPresenceHandler = FStanzaProcessor->insertHandler(this,SHC_PRESENCE,IStanzaProcessor::DirectionIn,SHP_MULTIUSERCHAT,FStreamJid);
      if (FMessenger == NULL)
        FMessageHandler = FStanzaProcessor->insertHandler(this,SHC_MESSAGE,IStanzaProcessor::DirectionIn,SHP_MULTIUSERCHAT,FStreamJid);
    }
  }

  plugin = APluginManager->getPlugins("IPresencePlugin").value(0,NULL);
  if (plugin) 
  {
    IPresencePlugin *presencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance());
    if (presencePlugin)
    {
      FPresence = presencePlugin->getPresence(FStreamJid);
      if (FPresence)
      {
        connect(FPresence->instance(),SIGNAL(selfPresence(IPresence::Show, const QString &, qint8, const Jid &)),
          SLOT(onSelfPresence(IPresence::Show, const QString &, qint8, const Jid &)));
      }
    }
  }

  plugin = APluginManager->getPlugins("IXmppStreams").value(0,NULL);
  if (plugin) 
  {
    IXmppStreams *xmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
    if (xmppStreams)
    {
      FXmppStream = xmppStreams->getStream(FStreamJid);
      if (FXmppStream)
      {
        connect(FXmppStream->instance(),SIGNAL(jidChanged(IXmppStream *, const Jid &)),
          SLOT(onStreamJidChanged(IXmppStream *, const Jid &)));
        connect(FXmppStream->instance(),SIGNAL(aboutToClose(IXmppStream *)),SLOT(onStreamAboutToClose(IXmppStream *)));
        connect(FXmppStream->instance(),SIGNAL(closed(IXmppStream *)),SLOT(onStreamClosed(IXmppStream *)));
      }
    }
  }

  if (FMessenger)
  {
    connect(FMessenger->instance(),SIGNAL(messageReceive(Message &)),SLOT(onMessageReceive(Message &)));
    connect(FMessenger->instance(),SIGNAL(messageReceived(const Message &)),SLOT(onMessageReceived(const Message &)));
    connect(FMessenger->instance(),SIGNAL(messageSend(Message &)),SLOT(onMessageSend(Message &)));
    connect(FMessenger->instance(),SIGNAL(messageSent(const Message &)),SLOT(onMessageSent(const Message &)));
  }
  
  return FStanzaProcessor!=NULL && FXmppStream != NULL;
}

void MultiUserChat::clearUsers()
{
  foreach(MultiUser *user, FUsers)
  {
    user->setData(MUDR_SHOW,IPresence::Offline);
    user->setData(MUDR_STATUS,tr("Disconnected"));
    emit userPresence(user,IPresence::Offline,tr("Disconnected"));
    delete user;
  }
  FMainUser = NULL;
  FUsers.clear();
}

void MultiUserChat::onMessageReceive(Message &AMessage)
{
  Jid fromJid = AMessage.from();
  if (FRoomJid && fromJid)
    emit messageReceive(fromJid.resource(), AMessage);
}

void MultiUserChat::onMessageReceived(const Message &AMessage)
{
  if ( (FRoomJid && AMessage.from()) && (FStreamJid == AMessage.to()) )
    processMessage(AMessage.stanza());
}

void MultiUserChat::onMessageSend(Message &AMessage)
{
  if (FRoomJid && AMessage.to())
    emit messageSend(AMessage);
}

void MultiUserChat::onMessageSent(const Message &AMessage)
{
  if (FRoomJid && AMessage.to())
    emit messageSent(AMessage);
}

void MultiUserChat::onUserDataChanged(int ARole, const QVariant &ABefour, const QVariant &AAfter)
{
  IMultiUser *user = qobject_cast<IMultiUser *>(sender());
  if (user)
    emit userDataChanged(user,ARole,ABefour,AAfter);
}

void MultiUserChat::onSelfPresence(IPresence::Show AShow, const QString &AStatus, qint8 /*APriority*/, const Jid &AToJid)
{
  if (FAutoPresence && !AToJid.isValid())
    setPresence(AShow,AStatus);
}

void MultiUserChat::onStreamJidChanged(IXmppStream *AXmppStream, const Jid &ABefour)
{
  FStreamJid = AXmppStream->jid();
  foreach(MultiUser *user, FUsers)
    user->setData(MUDR_STREAMJID,FStreamJid.full());
  emit streamJidChanged(ABefour,FStreamJid);
}

void MultiUserChat::onStreamAboutToClose(IXmppStream * /*AXmppStream*/)
{
  if (isOpen())
    setPresence(IPresence::Offline,tr("Disconnected"));
}

void MultiUserChat::onStreamClosed(IXmppStream * /*AXmppStream*/)
{
  if (isOpen())
  {
    clearUsers();
    FShow = IPresence::Offline;
    FStatus = tr("Disconnected");
    emit presenceChanged(FShow,FStatus);
    emit chatClosed();
  }
}
