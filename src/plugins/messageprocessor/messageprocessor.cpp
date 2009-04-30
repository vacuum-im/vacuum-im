#include "messageprocessor.h"

#include <QTextCursor>

#define SHC_MESSAGE         "/message"

MessageProcessor::MessageProcessor()
{
  FXmppStreams = NULL;
  FStanzaProcessor = NULL;
  FNotifications = NULL;
}

MessageProcessor::~MessageProcessor()
{

}

void MessageProcessor::pluginInfo(IPluginInfo *APluginInfo)
{
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->description = tr("Managing message stanzas");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = tr("Message Processor"); 
  APluginInfo->uid = MESSAGEPROCESSOR_UUID;
  APluginInfo->version = "0.1";
  APluginInfo->conflicts.append("{153A4638-B468-496f-B57C-9F30CEDFCC2E}");
}

bool MessageProcessor::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *plugin = APluginManager->getPlugins("IXmppStreams").value(0,NULL);
  if (plugin) 
  {
    FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
    if (FXmppStreams)
    {
      connect(FXmppStreams->instance(),SIGNAL(opened(IXmppStream *)),SLOT(onStreamOpened(IXmppStream *)));
      connect(FXmppStreams->instance(),SIGNAL(jidAboutToBeChanged(IXmppStream *, const Jid &)),
        SLOT(onStreamJidAboutToBeChanged(IXmppStream *, const Jid &)));
      connect(FXmppStreams->instance(),SIGNAL(jidChanged(IXmppStream *, const Jid &)),
        SLOT(onStreamJidChanged(IXmppStream *, const Jid &)));
      connect(FXmppStreams->instance(),SIGNAL(closed(IXmppStream *)),SLOT(onStreamClosed(IXmppStream *)));
      connect(FXmppStreams->instance(),SIGNAL(removed(IXmppStream *)),SLOT(onStreamRemoved(IXmppStream *)));
    }
  }

  plugin = APluginManager->getPlugins("IStanzaProcessor").value(0,NULL);
  if (plugin) 
    FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());

  plugin = APluginManager->getPlugins("INotifications").value(0,NULL);
  if (plugin) 
  {
    FNotifications = qobject_cast<INotifications *>(plugin->instance());
    if (FNotifications)
    {
      connect(FNotifications->instance(),SIGNAL(notificationActivated(int)), SLOT(onNotificationActivated(int)));
    }
  }

  return true;
}

bool MessageProcessor::initObjects()
{
  insertMessageWriter(this,MWO_MESSAGEPROCESSOR);
  insertMessageWriter(this,MWO_MESSAGEPROCESSOR_ANCHORS);
  return true;
}

bool MessageProcessor::readStanza(int AHandlerId, const Jid &AStreamJid, const Stanza &AStanza, bool &AAccept)
{
  if (FSHIMessages.value(AStreamJid) == AHandlerId)
  {
    Message message(AStanza);
    bool received = receiveMessage(message) > 0;
    AAccept = AAccept || received;
  }
  return false;
}

void MessageProcessor::writeMessage(Message &AMessage, QTextDocument *ADocument, const QString &ALang, int AOrder)
{
  if (AOrder == MWO_MESSAGEPROCESSOR)
  {
    AMessage.setBody(prepareBodyForSend(ADocument->toPlainText()),ALang);
  }
}

void MessageProcessor::writeText(Message &AMessage, QTextDocument *ADocument, const QString &ALang, int AOrder)
{
  if (AOrder == MWO_MESSAGEPROCESSOR)
  {
    QTextCursor cursor(ADocument);
    cursor.insertHtml(prepareBodyForReceive(AMessage.body(ALang)));
  }
  else if (AOrder == MWO_MESSAGEPROCESSOR_ANCHORS)
  {
    QRegExp regexp("\\b((https?|ftp)://|www.)[a-z0-9/\\?.=:@&%#_;\\+\\-]+");
    regexp.setCaseSensitivity(Qt::CaseInsensitive);
    for (QTextCursor cursor = ADocument->find(regexp); !cursor.isNull();  cursor = ADocument->find(regexp,cursor))
    {
      QTextCharFormat linkFormat = cursor.charFormat();
      linkFormat.setAnchor(true);
      linkFormat.setAnchorHref(cursor.selectedText());
      cursor.setCharFormat(linkFormat);
    }
  }
}

int MessageProcessor::receiveMessage(const Message &AMessage)
{
  int messageId = -1;
  IMessageHandler *handler = getMessageHandler(AMessage);
  if (handler)
  {
    Message message = AMessage;
    messageId = newMessageId();
    message.setData(MDR_MESSAGE_ID,messageId);
    FMessages.insert(messageId,message);
    FHandlerForMessage.insert(messageId,handler);

    emit messageReceive(message);
    notifyMessage(messageId);
    handler->receiveMessage(messageId);
    emit messageReceived(message);
  }
  return messageId;
}

bool MessageProcessor::sendMessage(const Jid &AStreamJid, const Message &AMessage)
{
  Message message = AMessage;

  emit messageSend(message);
  if (FStanzaProcessor->sendStanzaOut(AStreamJid,message.stanza()))
  {
    emit messageSent(message);
    return true;
  }
  return false;
}

void MessageProcessor::showMessage(int AMessageId)
{
  IMessageHandler *handler = FHandlerForMessage.value(AMessageId,NULL);
  if (handler)
    handler->showMessage(AMessageId);
}

void MessageProcessor::removeMessage(int AMessageId)
{
  if (FMessages.contains(AMessageId))
  {
    unNotifyMessage(AMessageId);
    FHandlerForMessage.remove(AMessageId);
    Message message = FMessages.take(AMessageId);
    emit messageRemoved(message);
  }
}

Message MessageProcessor::messageById(int AMessageId) const
{
  return FMessages.value(AMessageId);
}

QList<int> MessageProcessor::messages(const Jid &AStreamJid, const Jid &AFromJid, int AMesTypes)
{
  QList<int> mIds;
  for (QMap<int,Message>::const_iterator it = FMessages.constBegin(); it != FMessages.constEnd(); it++)
  {
    if (AStreamJid == it.value().to() && 
      (!AFromJid.isValid() || AFromJid==it.value().from()) && 
      (AMesTypes==Message::AnyType || (AMesTypes & it.value().type())>0))
    {
      mIds.append(it.key());
    }
  }
  return mIds;
}

void MessageProcessor::textToMessage(Message &AMessage, const QTextDocument *ADocument, const QString &ALang) const
{
  QTextDocument *documentCopy = ADocument->clone();
  QMapIterator<int,IMessageWriter *> it(FMessageWriters);
  it.toBack();
  while(it.hasPrevious())
  {
    it.previous();
    it.value()->writeMessage(AMessage,documentCopy,ALang,it.key());
  }
  delete documentCopy;
}

void MessageProcessor::messageToText(QTextDocument *ADocument, const Message &AMessage, const QString &ALang) const
{
  Message messageCopy = AMessage;
  QMapIterator<int,IMessageWriter *> it(FMessageWriters);
  it.toFront();
  while(it.hasNext())
  {
    it.next();
    it.value()->writeText(messageCopy,ADocument,ALang,it.key());
  }
}

bool MessageProcessor::openWindow(const Jid &AStreamJid, const Jid &AContactJid, Message::MessageType AType) const
{
  foreach(IMessageHandler *handler, FMessageHandlers)
    if (handler->openWindow(AStreamJid,AContactJid,AType))
      return true;
  return false;
}

void MessageProcessor::insertMessageHandler(IMessageHandler *AHandler, int AOrder)
{
  if (!FMessageHandlers.values(AOrder).contains(AHandler))  
  {
    FMessageHandlers.insert(AOrder,AHandler);
    emit messageHandlerInserted(AHandler,AOrder);
  }
}

void MessageProcessor::removeMessageHandler(IMessageHandler *AHandler, int AOrder)
{
  if (FMessageHandlers.values(AOrder).contains(AHandler))  
  {
    FMessageHandlers.remove(AOrder,AHandler);
    emit messageHandlerRemoved(AHandler,AOrder);
  }
}

void MessageProcessor::insertMessageWriter(IMessageWriter *AWriter, int AOrder)
{
  if (!FMessageWriters.values(AOrder).contains(AWriter))  
  {
    FMessageWriters.insert(AOrder,AWriter);
    emit messageWriterInserted(AWriter,AOrder);
  }
}

void MessageProcessor::removeMessageWriter(IMessageWriter *AWriter, int AOrder)
{
  if (FMessageWriters.values(AOrder).contains(AWriter))  
  {
    FMessageWriters.remove(AOrder,AWriter);
    emit messageWriterRemoved(AWriter,AOrder);
  }
}

int MessageProcessor::newMessageId()
{
  static int messageId = 1;
  return messageId++;
}

IMessageHandler *MessageProcessor::getMessageHandler(const Message &AMessage)
{
  foreach(IMessageHandler *handler, FMessageHandlers)
    if (handler->checkMessage(AMessage))
      return handler;
  return NULL;
}

void MessageProcessor::notifyMessage(int AMessageId)
{
  if (FMessages.contains(AMessageId))
  {
    if (FNotifications)
    {
      Message &message = FMessages[AMessageId];
      IMessageHandler *handler = FHandlerForMessage.value(AMessageId);
      INotification notify = handler->notification(FNotifications, message);
      if (notify.kinds > 0)
      {
        int notifyId = FNotifications->appendNotification(notify);
        FNotifyId2MessageId.insert(notifyId,AMessageId);
      }
    }
    emit messageNotified(AMessageId);
  }
}

void MessageProcessor::unNotifyMessage(int AMessageId)
{
  if (FMessages.contains(AMessageId))
  {
    if (FNotifications)
    {
      int notifyId = FNotifyId2MessageId.key(AMessageId);
      FNotifications->removeNotification(notifyId);
      FNotifyId2MessageId.remove(notifyId);
    }
    emit messageUnNotified(AMessageId);
  }
}

void MessageProcessor::removeStreamMessages(const Jid &AStreamJid)
{
  foreach (int messageId, messages(AStreamJid))
    removeMessage(messageId);
}

QString MessageProcessor::prepareBodyForSend(const QString &AString) const
{
  QString result = AString;
  result.remove(QChar::ObjectReplacementCharacter);
  return result;
}

QString MessageProcessor::prepareBodyForReceive(const QString &AString) const
{
  QString result = Qt::escape(AString);
  result.replace('\n',"<br>");
  result.replace("  " ,"&nbsp; ");
  return result;
}

void MessageProcessor::onStreamOpened(IXmppStream *AXmppStream)
{
  if (FStanzaProcessor && !FSHIMessages.contains(AXmppStream->jid()))
  {
    int handler = FStanzaProcessor->insertHandler(this,SHC_MESSAGE,IStanzaProcessor::DirectionIn,SHP_DEFAULT,AXmppStream->jid());
    FSHIMessages.insert(AXmppStream->jid(),handler);
  }
}

void MessageProcessor::onStreamJidAboutToBeChanged(IXmppStream *AXmppStream, const Jid &AAfter)
{
  if (AAfter && AXmppStream->jid())
  {
    QMap<int,Message>::iterator it = FMessages.begin();
    while (it != FMessages.end())
    {
      if (AXmppStream->jid() == it.value().to())
      {
        unNotifyMessage(it.key());
        it.value().setTo(AAfter.eFull());
      }
      it++;
    }
  }
  else
    removeStreamMessages(AXmppStream->jid());
}

void MessageProcessor::onStreamJidChanged(IXmppStream *AXmppStream, const Jid &/*ABefour*/)
{
  QMap<int,Message>::iterator it = FMessages.begin();
  while (it != FMessages.end())
  {
    if (AXmppStream->jid() == it.value().to())
      notifyMessage(it.key());
    it++;
  }
}

void MessageProcessor::onStreamClosed(IXmppStream *AXmppStream)
{
  if (FStanzaProcessor && FSHIMessages.contains(AXmppStream->jid()))
  {
    FStanzaProcessor->removeHandler(FSHIMessages.take(AXmppStream->jid()));
  }
}

void MessageProcessor::onStreamRemoved(IXmppStream *AXmppStream)
{
  removeStreamMessages(AXmppStream->jid());
}

void MessageProcessor::onNotificationActivated(int ANotifyId)
{
  if (FNotifyId2MessageId.contains(ANotifyId))
    showMessage(FNotifyId2MessageId.value(ANotifyId));
}

Q_EXPORT_PLUGIN2(MessageProcessorPlugin, MessageProcessor)
