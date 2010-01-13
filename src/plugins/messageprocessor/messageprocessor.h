#ifndef MESSAGEPROCESSOR_H
#define MESSAGEPROCESSOR_H

#include <definations/messagedataroles.h>
#include <definations/messagewriterorders.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imessageprocessor.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/inotifications.h>
#include <interfaces/istanzaprocessor.h>

class MessageProcessor : 
  public QObject,
  public IPlugin,
  public IMessageProcessor,
  public IMessageWriter,
  public IStanzaHandler
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IMessageProcessor IMessageWriter IStanzaHandler);
public:
  MessageProcessor();
  ~MessageProcessor();
  //IPlugin
  virtual QObject *instance() { return this; }
  virtual QUuid pluginUuid() const { return MESSAGEPROCESSOR_UUID; }
  virtual void pluginInfo(IPluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects();
  virtual bool initSettings() { return true; }
  virtual bool startPlugin() { return true; }
  //IStanzaHandler
  virtual bool stanzaEdit(int /*AHandlerId*/, const Jid &/*AStreamJid*/, Stanza &/*AStanza*/, bool &/*AAccept*/) { return false; }
  virtual bool stanzaRead(int AHandlerId, const Jid &AStreamJid, const Stanza &AStanza, bool &AAccept);
  //IMessageWriter
  virtual void writeMessage(Message &AMessage, QTextDocument *ADocument, const QString &ALang, int AOrder);
  virtual void writeText(Message &AMessage, QTextDocument *ADocument, const QString &ALang, int AOrder);
  //IMessageProcessor
  virtual int receiveMessage(const Message &AMessage);
  virtual bool sendMessage(const Jid &AStreamJid, const Message &AMessage);
  virtual void showMessage(int AMessageId);
  virtual void removeMessage(int AMessageId);
  virtual Message messageById(int AMessageId) const;
  virtual QList<int> messages(const Jid &AStreamJid, const Jid &AFromJid = Jid(), int AMesTypes = Message::AnyType);
  virtual void textToMessage(Message &AMessage, const QTextDocument *ADocument, const QString &ALang = "") const;
  virtual void messageToText(QTextDocument *ADocument, const Message &AMessage, const QString &ALang = "") const;
  virtual bool openWindow(const Jid &AStreamJid, const Jid &AContactJid, Message::MessageType AType) const;
  virtual void insertMessageHandler(IMessageHandler *AHandler, int AOrder);
  virtual void removeMessageHandler(IMessageHandler *AHandler, int AOrder);
  virtual void insertMessageWriter(IMessageWriter *AWriter, int AOrder);
  virtual void removeMessageWriter(IMessageWriter *AWriter, int AOrder);
signals:
  void messageReceive(Message &AMessage);
  void messageReceived(const Message &AMessage);
  void messageSend(Message &AMessage);
  void messageSent(const Message &AMessage);
  void messageNotified(int AMessageId);
  void messageUnNotified(int AMessageId);
  void messageRemoved(const Message &AMessage);
  void messageHandlerInserted(IMessageHandler *AHandler, int AOrder);
  void messageHandlerRemoved(IMessageHandler *AHandler, int AOrder);
  void messageWriterInserted(IMessageWriter *AWriter, int AOrder);
  void messageWriterRemoved(IMessageWriter *AWriter, int AOrder);
protected:
  int newMessageId();
  IMessageHandler *getMessageHandler(const Message &AMessage);
  void notifyMessage(int AMessageId);
  void unNotifyMessage(int AMessageId);
  void removeStreamMessages(const Jid &AStreamJid);
  QString prepareBodyForSend(const QString &AString) const;
  QString prepareBodyForReceive(const QString &AString) const;
protected slots:
  void onStreamOpened(IXmppStream *AXmppStream);
  void onStreamJidAboutToBeChanged(IXmppStream *AXmppStream, const Jid &AAfter);
  void onStreamJidChanged(IXmppStream *AXmppStream, const Jid &ABefour);
  void onStreamClosed(IXmppStream *AXmppStream);
  void onStreamRemoved(IXmppStream *AXmppStream);
  void onNotificationActivated(int ANotifyId);
private:
  IXmppStreams *FXmppStreams;
  INotifications *FNotifications;
  IStanzaProcessor *FStanzaProcessor;
private:
  QHash<Jid,int> FSHIMessages;
  QMap<int,Message> FMessages;
  QHash<int,int> FNotifyId2MessageId;
  QHash<int,IMessageHandler *> FHandlerForMessage;
  QMultiMap<int,IMessageHandler *> FMessageHandlers;
  QMultiMap<int,IMessageWriter *> FMessageWriters;
};

#endif // MESSAGEPROCESSOR_H
