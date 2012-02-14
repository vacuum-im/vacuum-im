#include "messageprocessor.h"

#include <QVariant>
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
	APluginInfo->name = tr("Message Manager");
	APluginInfo->description = tr("Allows other modules to send and receive messages");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(XMPPSTREAMS_UUID);
	APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool MessageProcessor::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
	IPlugin *plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
	if (plugin)
	{
		FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
		if (FXmppStreams)
		{
			connect(FXmppStreams->instance(),SIGNAL(opened(IXmppStream *)),SLOT(onStreamOpened(IXmppStream *)));
			connect(FXmppStreams->instance(),SIGNAL(closed(IXmppStream *)),SLOT(onStreamClosed(IXmppStream *)));
			connect(FXmppStreams->instance(),SIGNAL(removed(IXmppStream *)),SLOT(onStreamRemoved(IXmppStream *)));
		}
	}

	plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0,NULL);
	if (plugin)
		FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());

	plugin = APluginManager->pluginInterface("INotifications").value(0,NULL);
	if (plugin)
	{
		FNotifications = qobject_cast<INotifications *>(plugin->instance());
		if (FNotifications)
		{
			connect(FNotifications->instance(),SIGNAL(notificationActivated(int)), SLOT(onNotificationActivated(int)));
			connect(FNotifications->instance(),SIGNAL(notificationRemoved(int)), SLOT(onNotificationRemoved(int)));
		}
	}

	return FStanzaProcessor!=NULL && FXmppStreams!=NULL;
}

bool MessageProcessor::initObjects()
{
	insertMessageWriter(MWO_MESSAGEPROCESSOR,this);
	insertMessageWriter(MWO_MESSAGEPROCESSOR_ANCHORS,this);
	return true;
}

bool MessageProcessor::stanzaReadWrite(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept)
{
	if (FSHIMessages.value(AStreamJid) == AHandlerId)
	{
		Message message(AStanza);
		AAccept = sendMessage(AStreamJid,message,IMessageProcessor::MessageIn) || AAccept;
	}
	return false;
}

void MessageProcessor::writeTextToMessage(int AOrder, Message &AMessage, QTextDocument *ADocument, const QString &ALang)
{
	if (AOrder == MWO_MESSAGEPROCESSOR)
	{
		AMessage.setBody(prepareBodyForSend(ADocument->toPlainText()),ALang);
	}
}

void MessageProcessor::writeMessageToText(int AOrder, Message &AMessage, QTextDocument *ADocument, const QString &ALang)
{
	if (AOrder == MWO_MESSAGEPROCESSOR)
	{
		QTextCursor cursor(ADocument);
		cursor.insertHtml(prepareBodyForReceive(AMessage.body(ALang)));
	}
	else if (AOrder == MWO_MESSAGEPROCESSOR_ANCHORS)
	{
		QRegExp regexp("\\b((https?|ftp)://|www\\.|xmpp:|magnet:)\\S+");
		regexp.setCaseSensitivity(Qt::CaseInsensitive);
		for (QTextCursor cursor = ADocument->find(regexp); !cursor.isNull();  cursor = ADocument->find(regexp,cursor))
		{
#if QT_VERSION >= QT_VERSION_CHECK(4,6,0)
			QUrl link = QUrl::fromUserInput(cursor.selectedText());
#else
			QUrl link = cursor.selectedText();
			if (link.scheme().isEmpty())
				link = QString("http://")+cursor.selectedText();
#endif
			QTextCharFormat linkFormat = cursor.charFormat();
			linkFormat.setAnchor(true);
			linkFormat.setAnchorHref(link.toString());
			cursor.setCharFormat(linkFormat);
		}
	}
}

bool MessageProcessor::sendMessage(const Jid &AStreamJid, Message &AMessage, int ADirection)
{
	if (processMessage(AStreamJid,AMessage,ADirection))
	{
		if (ADirection == IMessageProcessor::MessageOut)
		{
			if (FStanzaProcessor && FStanzaProcessor->sendStanzaOut(AStreamJid,AMessage.stanza()))
			{
				displayMessage(AStreamJid,AMessage,ADirection);
				emit messageSent(AMessage);
				return true;
			}
		}
		else 
		{
			displayMessage(AStreamJid,AMessage,ADirection);
			emit messageReceived(AMessage);
			return true;
		}
	}
	return false;
}

bool MessageProcessor::processMessage(const Jid &AStreamJid, Message &AMessage, int ADirection)
{
	if (ADirection == IMessageProcessor::MessageIn)
		AMessage.setTo(AStreamJid.eFull());
	else
		AMessage.setFrom(AStreamJid.eFull());

	bool hooked = false;
	QMapIterator<int,IMessageEditor *> it(FMessageEditors);
	ADirection == MessageIn ? it.toFront() : it.toBack();
	while (!hooked && (ADirection == MessageIn ? it.hasNext() : it.hasPrevious()))
	{
		ADirection == MessageIn ? it.next() : it.previous();
		hooked = it.value()->messageReadWrite(it.key(), AStreamJid, AMessage, ADirection);
	}

	return !hooked;
}

bool MessageProcessor::displayMessage(const Jid &AStreamJid, Message &AMessage, int ADirection)
{
	Q_UNUSED(AStreamJid);
	IMessageHandler *handler = findMessageHandler(AMessage, ADirection);
	if (handler)
	{
		int messageId = AMessage.data(MDR_MESSAGE_ID).toInt();
		if (messageId <= 0)
		{
			messageId = newMessageId();
			AMessage.setData(MDR_MESSAGE_ID,messageId);
		}
		AMessage.setData(MDR_MESSAGE_DIRECTION,ADirection);

		if (handler->messageDisplay(AMessage,ADirection))
		{
			notifyMessage(handler,AMessage,ADirection);
			return true;
		}
	}
	return false;
}

QList<int> MessageProcessor::notifiedMessages() const
{
	return FNotifiedMessages.keys();
}

Message MessageProcessor::notifiedMessage(int AMesssageId) const
{
	return FNotifiedMessages.value(AMesssageId);
}

int MessageProcessor::notifyByMessage(int AMessageId) const
{
	return FNotifyId2MessageId.key(AMessageId,-1);
}

int MessageProcessor::messageByNotify(int ANotifyId) const
{
	return FNotifyId2MessageId.value(ANotifyId,-1);
}

void MessageProcessor::showNotifiedMessage(int AMessageId)
{
	IMessageHandler *handler = FHandlerForMessage.value(AMessageId,NULL);
	if (handler)
		handler->messageShowWindow(AMessageId);
}

void MessageProcessor::removeMessageNotify(int AMessageId)
{
	int notifyId = FNotifyId2MessageId.key(AMessageId);
	if (notifyId > 0)
	{
		FNotifiedMessages.remove(AMessageId);
		FNotifyId2MessageId.remove(notifyId);
		FHandlerForMessage.remove(AMessageId);
		FNotifications->removeNotification(notifyId);
		emit messageNotifyRemoved(AMessageId);
	}
}

void MessageProcessor::textToMessage(Message &AMessage, const QTextDocument *ADocument, const QString &ALang) const
{
	QTextDocument *documentCopy = ADocument->clone();
	QMapIterator<int,IMessageWriter *> it(FMessageWriters);
	it.toBack();
	while (it.hasPrevious())
	{
		it.previous();
		it.value()->writeTextToMessage(it.key(),AMessage,documentCopy,ALang);
	}
	delete documentCopy;
}

void MessageProcessor::messageToText(QTextDocument *ADocument, const Message &AMessage, const QString &ALang) const
{
	Message messageCopy = AMessage;
	QMapIterator<int,IMessageWriter *> it(FMessageWriters);
	it.toFront();
	while (it.hasNext())
	{
		it.next();
		it.value()->writeMessageToText(it.key(), messageCopy,ADocument,ALang);
	}
}

bool MessageProcessor::createMessageWindow(const Jid &AStreamJid, const Jid &AContactJid, Message::MessageType AType, int AShowMode) const
{
	for (QMultiMap<int, IMessageHandler *>::const_iterator it = FMessageHandlers.constBegin(); it!=FMessageHandlers.constEnd(); it++)
		if (it.value()->messageShowWindow(it.key(),AStreamJid,AContactJid,AType,AShowMode))
			return true;
	return false;
}

void MessageProcessor::insertMessageHandler(int AOrder, IMessageHandler *AHandler)
{
	if (!FMessageHandlers.contains(AOrder,AHandler))
	{
		FMessageHandlers.insertMulti(AOrder,AHandler);
		emit messageHandlerInserted(AOrder,AHandler);
	}
}

void MessageProcessor::removeMessageHandler(int AOrder, IMessageHandler *AHandler)
{
	if (FMessageHandlers.contains(AOrder,AHandler))
	{
		FMessageHandlers.remove(AOrder,AHandler);
		emit messageHandlerRemoved(AOrder,AHandler);
	}
}

void MessageProcessor::insertMessageWriter(int AOrder, IMessageWriter *AWriter)
{
	if (!FMessageWriters.contains(AOrder,AWriter))
	{
		FMessageWriters.insertMulti(AOrder,AWriter);
		emit messageWriterInserted(AOrder,AWriter);
	}
}

void MessageProcessor::removeMessageWriter(int AOrder, IMessageWriter *AWriter)
{
	if (FMessageWriters.contains(AOrder,AWriter))
	{
		FMessageWriters.remove(AOrder,AWriter);
		emit messageWriterRemoved(AOrder,AWriter);
	}
}

void MessageProcessor::insertMessageEditor(int AOrder, IMessageEditor *AEditor)
{
	if (!FMessageEditors.contains(AOrder,AEditor))
	{
		FMessageEditors.insertMulti(AOrder,AEditor);
		emit messageEditorInserted(AOrder,AEditor);
	}
}

void MessageProcessor::removeMessageEditor(int AOrder, IMessageEditor *AEditor)
{
	if (FMessageEditors.contains(AOrder,AEditor))
	{
		FMessageEditors.remove(AOrder,AEditor);
		emit messageEditorRemoved(AOrder,AEditor);
	}
}

int MessageProcessor::newMessageId()
{
	static int messageId = 1;
	return messageId++;
}

IMessageHandler *MessageProcessor::findMessageHandler(const Message &AMessage, int ADirection)
{
	for (QMultiMap<int, IMessageHandler *>::const_iterator it = FMessageHandlers.constBegin(); it!=FMessageHandlers.constEnd(); it++)
		if (it.value()->messageCheck(it.key(),AMessage,ADirection))
			return it.value();
	return NULL;
}

void MessageProcessor::notifyMessage(IMessageHandler *AHandler, const Message &AMessage, int ADirection)
{
	if (FNotifications && AHandler)
	{
		INotification notify = AHandler->messageNotify(FNotifications, AMessage, ADirection);
		if (notify.kinds > 0)
		{
			int notifyId = FNotifications->appendNotification(notify);
			int messageId = AMessage.data(MDR_MESSAGE_ID).toInt();
			FNotifiedMessages.insert(messageId,AMessage);
			FNotifyId2MessageId.insert(notifyId,messageId);
			FHandlerForMessage.insert(messageId,AHandler);
			emit messageNotifyInserted(messageId);
		}
	}
}

QString MessageProcessor::prepareBodyForSend(const QString &AString) const
{
	QString result = AString;
	result.remove(QChar::Null);
	result.remove(QChar::ObjectReplacementCharacter);
	return result;
}

QString MessageProcessor::prepareBodyForReceive(const QString &AString) const
{
	QString result = Qt::escape(AString);
	result.replace('\n',"<br>");
	result.replace("  ","&nbsp; ");
	result.replace('\t',"&nbsp; &nbsp; ");
	return result;
}

void MessageProcessor::onStreamOpened(IXmppStream *AXmppStream)
{
	if (FStanzaProcessor)
	{
		IStanzaHandle shandle;
		shandle.handler = this;
		shandle.order = SHO_DEFAULT;
		shandle.direction = IStanzaHandle::DirectionIn;
		shandle.streamJid = AXmppStream->streamJid();
		shandle.conditions.append(SHC_MESSAGE);
		FSHIMessages.insert(shandle.streamJid,FStanzaProcessor->insertStanzaHandle(shandle));
	}
}

void MessageProcessor::onStreamClosed(IXmppStream *AXmppStream)
{
	if (FStanzaProcessor)
	{
		FStanzaProcessor->removeStanzaHandle(FSHIMessages.take(AXmppStream->streamJid()));
	}
}

void MessageProcessor::onStreamRemoved(IXmppStream *AXmppStream)
{
	foreach(int notifyId, FNotifyId2MessageId.keys())
	{
		INotification notify = FNotifications->notificationById(notifyId);
		if (AXmppStream->streamJid() == notify.data.value(NDR_STREAM_JID).toString())
			removeMessageNotify(FNotifyId2MessageId.value(notifyId));
	}
}

void MessageProcessor::onNotificationActivated(int ANotifyId)
{
	if (FNotifyId2MessageId.contains(ANotifyId))
		showNotifiedMessage(FNotifyId2MessageId.value(ANotifyId));
}

void MessageProcessor::onNotificationRemoved(int ANotifyId)
{
	if (FNotifyId2MessageId.contains(ANotifyId))
		removeMessageNotify(FNotifyId2MessageId.value(ANotifyId));
}

Q_EXPORT_PLUGIN2(plg_messageprocessor, MessageProcessor)
