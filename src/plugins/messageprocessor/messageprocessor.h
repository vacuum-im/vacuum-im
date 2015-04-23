#ifndef MESSAGEPROCESSOR_H
#define MESSAGEPROCESSOR_H

#include <interfaces/ipluginmanager.h>
#include <interfaces/imessageprocessor.h>
#include <interfaces/ixmppstreammanager.h>
#include <interfaces/inotifications.h>
#include <interfaces/istanzaprocessor.h>

class MessageProcessor :
	public QObject,
	public IPlugin,
	public IMessageProcessor,
	public IMessageWriter,
	public IStanzaHandler
{
	Q_OBJECT
	Q_INTERFACES(IPlugin IMessageProcessor IMessageWriter IStanzaHandler)
	Q_PLUGIN_METADATA(IID "org.jrudevels.vacuum.IMessageProcessor");
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
	virtual void writeTextToMessage(int AOrder, Message &AMessage, QTextDocument *ADocument, const QString &ALang);
	virtual void writeMessageToText(int AOrder, Message &AMessage, QTextDocument *ADocument, const QString &ALang);
	//IMessageProcessor
	virtual QList<Jid> activeStreams() const;
	virtual bool isActiveStream(const Jid &AStreamJid) const;
	virtual void appendActiveStream(const Jid &AStreamJid);
	virtual void removeActiveStream(const Jid &AStreamJid);
	virtual bool sendMessage(const Jid &AStreamJid, Message &AMessage, int ADirection);
	virtual bool processMessage(const Jid &AStreamJid, Message &AMessage, int ADirection);
	virtual bool displayMessage(const Jid &AStreamJid, Message &AMessage, int ADirection);
	virtual QList<int> notifiedMessages() const;
	virtual Message notifiedMessage(int AMesssageId) const;
	virtual int notifyByMessage(int AMessageId) const;
	virtual int messageByNotify(int ANotifyId) const;
	virtual void showNotifiedMessage(int AMessageId);
	virtual void removeMessageNotify(int AMessageId);
	virtual void textToMessage(Message &AMessage, const QTextDocument *ADocument, const QString &ALang = QString::null) const;
	virtual void messageToText(QTextDocument *ADocument, const Message &AMessage, const QString &ALang = QString::null) const;
	virtual bool createMessageWindow(const Jid &AStreamJid, const Jid &AContactJid, Message::MessageType AType, int AShowMode) const;
	virtual QMultiMap<int, IMessageHandler *> messageHandlers() const;
	virtual void insertMessageHandler(int AOrder, IMessageHandler *AHandler);
	virtual void removeMessageHandler(int AOrder, IMessageHandler *AHandler);
	virtual QMultiMap<int, IMessageWriter *> messageWriters() const;
	virtual void insertMessageWriter(int AOrder, IMessageWriter *AWriter);
	virtual void removeMessageWriter(int AOrder, IMessageWriter *AWriter);
	virtual QMultiMap<int, IMessageEditor *> messageEditors() const;
	virtual void insertMessageEditor(int AOrder, IMessageEditor *AEditor);
	virtual void removeMessageEditor(int AOrder, IMessageEditor *AEditor);
signals:
	void messageSent(const Message &AMessage);
	void messageReceived(const Message &AMessage);
	void messageNotifyInserted(int AMessageId);
	void messageNotifyRemoved(int AMessageid);
	void activeStreamAppended(const Jid &AStreamJid);
	void activeStreamRemoved(const Jid &AStreamJid);
protected:
	int newMessageId();
	IMessageHandler *findMessageHandler(const Message &AMessage, int ADirection);
	void notifyMessage(IMessageHandler *AHandler, const Message &AMessage, int ADirection);
	QString prepareBodyForSend(const QString &AString) const;
	QString prepareBodyForReceive(const QString &AString) const;
protected slots:
	void onNotificationActivated(int ANotifyId);
	void onNotificationRemoved(int ANotifyId);
	void onXmppStreamActiveChanged(IXmppStream *AXmppStream, bool AActive);
	void onXmppStreamJidChanged(IXmppStream *AXmppStream, const Jid &ABefore);
private:
	IXmppStreamManager *FXmppStreamManager;
	INotifications *FNotifications;
	IStanzaProcessor *FStanzaProcessor;
private:
	QMap<int, int> FNotifyId2MessageId;
	QMap<int, Message> FNotifiedMessages;
private:
	QMap<Jid, int> FActiveStreams;
	QMap<int, IMessageHandler *> FHandlerForMessage;
	QMultiMap<int, IMessageHandler *> FMessageHandlers;
	QMultiMap<int, IMessageWriter *> FMessageWriters;
	QMultiMap<int, IMessageEditor *> FMessageEditors;
};

#endif // MESSAGEPROCESSOR_H
