#ifndef IMESSAGEPROCESSOR_H
#define IMESSAGEPROCESSOR_H

#include <QTextDocument>
#include <interfaces/irostersmodel.h>
#include <interfaces/inotifications.h>
#include <utils/jid.h>
#include <utils/message.h>

#define MESSAGEPROCESSOR_UUID "{1282bedb-f58f-48e8-96f6-62abb15dc6e1}"

class IMessageHandler
{
public:
	virtual bool checkMessage(int AOrder, const Message &AMessage) =0;
	virtual void showMessage(int AMessageId) =0;
	virtual void receiveMessage(int AMessageId) =0;
	virtual INotification notification(INotifications *ANotifications, const Message &AMessage) =0;
	virtual bool openWindow(int AOrder, const Jid &AStreamJid, const Jid &AContactJid, Message::MessageType AType) =0;
};

class IMessageWriter
{
public:
	virtual void writeText(int AOrder, Message &AMessage, QTextDocument *ADocument, const QString &ALang) =0;
	virtual void writeMessage(int AOrder, Message &AMessage, QTextDocument *ADocument, const QString &ALang) =0;
};

class IMessageProcessor
{
public:
	virtual QObject *instance() = 0;
	virtual int receiveMessage(const Message &AMessage) =0;
	virtual bool sendMessage(const Jid &AStreamJid, const Message &AMessage) =0;
	virtual void showMessage(int AMessageId) =0;
	virtual void removeMessage(int AMessageId) =0;
	virtual Message messageById(int AMessageId) const =0;
	virtual QList<int> messages(const Jid &AStreamJid, const Jid &AFromJid = Jid(), int AMesTypes = Message::AnyType) =0;
	virtual void textToMessage(Message &AMessage, const QTextDocument *ADocument, const QString &ALang = "") const =0;
	virtual void messageToText(QTextDocument *ADocument, const Message &AMessage, const QString &ALang = "") const =0;
	virtual bool openWindow(const Jid &AStreamJid, const Jid &AContactJid, Message::MessageType AType) const =0;
	virtual void insertMessageHandler(IMessageHandler *AHandler, int AOrder) =0;
	virtual void removeMessageHandler(IMessageHandler *AHandler, int AOrder) =0;
	virtual void insertMessageWriter(IMessageWriter *AWriter, int AOrder) =0;
	virtual void removeMessageWriter(IMessageWriter *AWriter, int AOrder) =0;
protected:
	virtual void messageReceive(Message &AMessage) =0;
	virtual void messageReceived(const Message &AMessage) =0;
	virtual void messageSend(Message &AMessage) =0;
	virtual void messageSent(const Message &AMessage) =0;
	virtual void messageNotified(int AMessageId) =0;
	virtual void messageUnNotified(int AMessageId) =0;
	virtual void messageRemoved(const Message &AMessage) =0;
	virtual void messageHandlerInserted(IMessageHandler *AHandler, int AOrder) =0;
	virtual void messageHandlerRemoved(IMessageHandler *AHandler, int AOrder) =0;
	virtual void messageWriterInserted(IMessageWriter *AWriter, int AOrder) =0;
	virtual void messageWriterRemoved(IMessageWriter *AWriter, int AOrder) =0;
};

Q_DECLARE_INTERFACE(IMessageHandler,"Vacuum.Plugin.IMessageHandler/1.0")
Q_DECLARE_INTERFACE(IMessageWriter,"Vacuum.Plugin.IMessageWriter/1.0")
Q_DECLARE_INTERFACE(IMessageProcessor,"Vacuum.Plugin.IMessageProcessor/1.0")

#endif
