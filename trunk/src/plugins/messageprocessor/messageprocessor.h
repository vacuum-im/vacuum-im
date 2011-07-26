#ifndef MESSAGEPROCESSOR_H
#define MESSAGEPROCESSOR_H

#include <definitions/messagedataroles.h>
#include <definitions/messagewriterorders.h>
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
	virtual bool stanzaReadWrite(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept);
	//IMessageWriter
	virtual void writeMessage(int AOrder, Message &AMessage, QTextDocument *ADocument, const QString &ALang);
	virtual void writeText(int AOrder, Message &AMessage, QTextDocument *ADocument, const QString &ALang);
	//IMessageProcessor
	virtual int receiveMessage(const Message &AMessage);
	virtual bool sendMessage(const Jid &AStreamJid, const Message &AMessage);
	virtual void showMessage(int AMessageId);
	virtual void removeMessage(int AMessageId);
	virtual Message messageById(int AMessageId) const;
	virtual QList<int> messages(const Jid &AStreamJid, const Jid &AFromJid = Jid::null, int AMesTypes = Message::AnyType);
	virtual void textToMessage(Message &AMessage, const QTextDocument *ADocument, const QString &ALang = QString::null) const;
	virtual void messageToText(QTextDocument *ADocument, const Message &AMessage, const QString &ALang = QString::null) const;
	virtual bool createMessageWindow(const Jid &AStreamJid, const Jid &AContactJid, Message::MessageType AType, int AShowMode) const;
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
	void onStreamJidChanged(IXmppStream *AXmppStream, const Jid &ABefore);
	void onStreamClosed(IXmppStream *AXmppStream);
	void onStreamRemoved(IXmppStream *AXmppStream);
	void onNotificationActivated(int ANotifyId);
	void onNotificationRemoved(int ANotifyId);
private:
	IXmppStreams *FXmppStreams;
	INotifications *FNotifications;
	IStanzaProcessor *FStanzaProcessor;
private:
	QMap<Jid,int> FSHIMessages;
	QMap<int,Message> FMessages;
	QMap<int,int> FNotifyId2MessageId;
	QMap<int,IMessageHandler *> FHandlerForMessage;
	QMultiMap<int,IMessageHandler *> FMessageHandlers;
	QMultiMap<int,IMessageWriter *> FMessageWriters;
};

#endif // MESSAGEPROCESSOR_H
