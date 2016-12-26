#include "messageprocessor.h"

#include <QVariant>
#include <QTextCursor>
#include <definitions/namespaces.h>
#include <definitions/messagedataroles.h>
#include <definitions/messagewriterorders.h>
#include <definitions/notificationdataroles.h>
#include <definitions/stanzahandlerorders.h>
#include <utils/logger.h>

#define SHC_MESSAGE         "/message"

MessageProcessor::MessageProcessor()
{
	FDiscovery = NULL;
	FNotifications = NULL;
	FStanzaProcessor = NULL;
	FXmppStreamManager = NULL;
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

bool MessageProcessor::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
	IPlugin *plugin = APluginManager->pluginInterface("IXmppStreamManager").value(0,NULL);
	if (plugin)
	{
		FXmppStreamManager = qobject_cast<IXmppStreamManager *>(plugin->instance());
		if (FXmppStreamManager)
		{
			connect(FXmppStreamManager->instance(),SIGNAL(streamActiveChanged(IXmppStream *, bool)),SLOT(onXmppStreamActiveChanged(IXmppStream *, bool)));
			connect(FXmppStreamManager->instance(),SIGNAL(streamJidChanged(IXmppStream *, const Jid &)),SLOT(onXmppStreamJidChanged(IXmppStream *, const Jid &)));
		}
	}

	plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0,NULL);
	if (plugin)
	{
		FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IServiceDiscovery").value(0,NULL);
	if (plugin)
	{
		FDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());
	}

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

	return FStanzaProcessor!=NULL && FXmppStreamManager!=NULL;
}

bool MessageProcessor::initObjects()
{
	insertMessageWriter(MWO_MESSAGEPROCESSOR,this);
	insertMessageWriter(MWO_MESSAGEPROCESSOR_ANCHORS,this);

	if (FDiscovery)
	{
		IDiscoFeature dfeature;
		dfeature.active = true;
		dfeature.var = NS_JABBER_X_OOB;
		dfeature.name = tr("Out of Band Data");
		dfeature.description = tr("Supports to communicate a URI to another user or application");
		FDiscovery->insertDiscoFeature(dfeature);
	}

	return true;
}

bool MessageProcessor::stanzaReadWrite(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept)
{
	if (FActiveStreams.value(AStreamJid) == AHandlerId)
	{
		Message message(AStanza);
		AAccept = sendMessage(AStreamJid,message,IMessageProcessor::DirectionIn) || AAccept;
	}
	return false;
}

bool MessageProcessor::writeMessageHasText(int AOrder, Message &AMessage, const QString &ALang)
{
	if (AOrder == MWO_MESSAGEPROCESSOR)
	{
		if (!AMessage.body(ALang).isEmpty())
			return true;

		for (QDomElement oobElem = AMessage.stanza().firstElement("x",NS_JABBER_X_OOB); !oobElem.isNull(); oobElem=oobElem.nextSiblingElement("x"))
			if (oobElem.namespaceURI()==NS_JABBER_X_OOB && !QUrl::fromUserInput(oobElem.firstChildElement("url").text()).isEmpty())
				return true;
	}
	return false;
}

bool MessageProcessor::writeMessageToText(int AOrder, Message &AMessage, QTextDocument *ADocument, const QString &ALang)
{
	bool changed = false;
	if (AOrder == MWO_MESSAGEPROCESSOR)
	{
		QTextCursor cursor(ADocument);

		QString html = convertBodyToHtml(AMessage.body(ALang));
		if (!html.isEmpty())
		{
			cursor.insertHtml(html);
			changed = true;
		}

		for (QDomElement oobElem = AMessage.stanza().firstElement("x",NS_JABBER_X_OOB); !oobElem.isNull(); oobElem=oobElem.nextSiblingElement("x"))
		{
			if (oobElem.namespaceURI() == NS_JABBER_X_OOB)
			{
				QString desc = oobElem.firstChildElement("desc").text().trimmed();
				QUrl url = QUrl::fromUserInput(oobElem.firstChildElement("url").text());
				if (!url.isEmpty() && url.toString() != html)
				{
					QTextCharFormat linkFormat;
					linkFormat.setAnchor(true);
					linkFormat.setToolTip(url.toString());
					linkFormat.setAnchorHref(url.toEncoded());
					if (!cursor.atStart())
						cursor.insertHtml("<br>");
					cursor.insertText(desc.isEmpty() ? url.toString() : desc, linkFormat);
					changed = true;
				}
			}
		}
	}
	else if (AOrder == MWO_MESSAGEPROCESSOR_ANCHORS)
	{
		QRegExp regexp("\\b((https?|ftp)://|www\\.|xmpp:|magnet:|mailto:)\\S+(/|#|~|@|&|=|-|\\+|\\*|\\$|\\b)");
		regexp.setCaseSensitivity(Qt::CaseInsensitive);
		for (QTextCursor cursor = ADocument->find(regexp); !cursor.isNull(); cursor = ADocument->find(regexp,cursor))
		{
			QTextCharFormat linkFormat = cursor.charFormat();
			if (!linkFormat.isAnchor())
			{
				QUrl url = QUrl::fromUserInput(cursor.selectedText());
				linkFormat.setAnchor(true);
				linkFormat.setAnchorHref(url.toEncoded());
				cursor.setCharFormat(linkFormat);
				changed = true;
			}
		}
	}
	return changed;
}

bool MessageProcessor::writeTextToMessage(int AOrder, QTextDocument *ADocument, Message &AMessage, const QString &ALang)
{
	bool changed = false;
	if (AOrder == MWO_MESSAGEPROCESSOR)
	{
		QString body = convertTextToBody(ADocument->toPlainText());
		if (!body.isEmpty())
		{
			AMessage.setBody(body,ALang);
			changed = true;
		}
	}
	return changed;
}

QList<Jid> MessageProcessor::activeStreams() const
{
	return FActiveStreams.keys();
}

bool MessageProcessor::isActiveStream(const Jid &AStreamJid) const
{
	return FActiveStreams.contains(AStreamJid);
}

void MessageProcessor::appendActiveStream(const Jid &AStreamJid)
{
	if (FStanzaProcessor && AStreamJid.isValid() && !FActiveStreams.contains(AStreamJid))
	{
		IStanzaHandle shandle;
		shandle.handler = this;
		shandle.order = SHO_DEFAULT;
		shandle.direction = IStanzaHandle::DirectionIn;
		shandle.streamJid = AStreamJid;
		shandle.conditions.append(SHC_MESSAGE);
		FActiveStreams.insert(shandle.streamJid,FStanzaProcessor->insertStanzaHandle(shandle));
		emit activeStreamAppended(AStreamJid);
	}
}

void MessageProcessor::removeActiveStream(const Jid &AStreamJid)
{
	if (FStanzaProcessor && FActiveStreams.contains(AStreamJid))
	{
		FStanzaProcessor->removeStanzaHandle(FActiveStreams.take(AStreamJid));
		foreach(int notifyId, FNotifyId2MessageId.keys())
		{
			INotification notify = FNotifications->notificationById(notifyId);
			if (AStreamJid == notify.data.value(NDR_STREAM_JID).toString())
				removeMessageNotify(FNotifyId2MessageId.value(notifyId));
		}
		emit activeStreamRemoved(AStreamJid);
	}
}

bool MessageProcessor::sendMessage(const Jid &AStreamJid, Message &AMessage, int ADirection)
{
	if (processMessage(AStreamJid,AMessage,ADirection))
	{
		if (ADirection == IMessageProcessor::DirectionOut)
		{
			Stanza stanza = AMessage.stanza(); // Ignore changes in StanzaProcessor
			if (FStanzaProcessor && FStanzaProcessor->sendStanzaOut(AStreamJid,stanza))
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
	if (ADirection == IMessageProcessor::DirectionIn)
		AMessage.setTo(AStreamJid.full());
	else
		AMessage.setFrom(AStreamJid.full());

	if (AMessage.data(MDR_MESSAGE_ID).isNull())
		AMessage.setData(MDR_MESSAGE_ID,newMessageId());
	AMessage.setData(MDR_MESSAGE_DIRECTION,ADirection);

	bool hooked = false;
	QMapIterator<int,IMessageEditor *> it(FMessageEditors);
	ADirection == DirectionIn ? it.toFront() : it.toBack();
	while (!hooked && (ADirection==DirectionIn ? it.hasNext() : it.hasPrevious()))
	{
		ADirection == DirectionIn ? it.next() : it.previous();
		hooked = it.value()->messageReadWrite(it.key(), AStreamJid, AMessage, ADirection);
	}

	return !hooked;
}

bool MessageProcessor::displayMessage(const Jid &AStreamJid, Message &AMessage, int ADirection)
{
	Q_UNUSED(AStreamJid);
	IMessageHandler *handler = findMessageHandler(AMessage,ADirection);
	if (handler && handler->messageDisplay(AMessage,ADirection))
	{
		notifyMessage(handler,AMessage,ADirection);
		return true;
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
	IMessageHandler *handler = FHandlerForMessage.value(AMessageId);
	if (handler)
		handler->messageShowNotified(AMessageId);
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


bool MessageProcessor::messageHasText(const Message &AMessage, const QString &ALang) const
{
	Message messageCopy = AMessage;
	QMapIterator<int,IMessageWriter *> it(FMessageWriters); it.toFront();
	while (it.hasNext())
	{
		if (it.next().value()->writeMessageHasText(it.key(),messageCopy,ALang))
			return true;
	}
	return false;
}

bool MessageProcessor::messageToText(const Message &AMessage, QTextDocument *ADocument, const QString &ALang) const
{
	bool changed = false;

	Message messageCopy = AMessage;
	QMapIterator<int,IMessageWriter *> it(FMessageWriters); it.toFront();
	while (it.hasNext())
		changed = it.next().value()->writeMessageToText(it.key(),messageCopy,ADocument,ALang) || changed;

	return changed;
}

bool MessageProcessor::textToMessage(const QTextDocument *ADocument, Message &AMessage, const QString &ALang) const
{
	bool changed = false;
	
	QTextDocument *documentCopy = ADocument->clone();
	QMapIterator<int,IMessageWriter *> it(FMessageWriters); it.toBack();
	while (it.hasPrevious())
		changed = it.previous().value()->writeTextToMessage(it.key(),documentCopy,AMessage,ALang) || changed;
	delete documentCopy;

	return changed;
}

IMessageWindow *MessageProcessor::getMessageWindow(const Jid &AStreamJid, const Jid &AContactJid, Message::MessageType AType, int AAction) const
{
	for (QMultiMap<int, IMessageHandler *>::const_iterator it = FMessageHandlers.constBegin(); it!=FMessageHandlers.constEnd(); ++it)
	{
		IMessageWindow *window = it.value()->messageGetWindow(AStreamJid,AContactJid,AType);
		if (window)
		{
			if (AAction == ActionAssign)
				window->assignTabPage();
			else if (AAction == ActionShowNormal)
				window->showTabPage();
			else if (AAction == ActionShowMinimized)
				window->showMinimizedTabPage();
			return window;
		}
	}
	return NULL;
}

QMultiMap<int, IMessageHandler *> MessageProcessor::messageHandlers() const
{
	return FMessageHandlers;
}

void MessageProcessor::insertMessageHandler(int AOrder, IMessageHandler *AHandler)
{
	if (AHandler && !FMessageHandlers.contains(AOrder,AHandler))
		FMessageHandlers.insertMulti(AOrder,AHandler);
}

void MessageProcessor::removeMessageHandler(int AOrder, IMessageHandler *AHandler)
{
	if (FMessageHandlers.contains(AOrder,AHandler))
		FMessageHandlers.remove(AOrder,AHandler);
}

QMultiMap<int, IMessageWriter *> MessageProcessor::messageWriters() const
{
	return FMessageWriters;
}

void MessageProcessor::insertMessageWriter(int AOrder, IMessageWriter *AWriter)
{
	if (AWriter && !FMessageWriters.contains(AOrder,AWriter))
		FMessageWriters.insertMulti(AOrder,AWriter);
}

void MessageProcessor::removeMessageWriter(int AOrder, IMessageWriter *AWriter)
{
	if (FMessageWriters.contains(AOrder,AWriter))
		FMessageWriters.remove(AOrder,AWriter);
}

QMultiMap<int, IMessageEditor *> MessageProcessor::messageEditors() const
{
	return FMessageEditors;
}

void MessageProcessor::insertMessageEditor(int AOrder, IMessageEditor *AEditor)
{
	if (AEditor && !FMessageEditors.contains(AOrder,AEditor))
		FMessageEditors.insertMulti(AOrder,AEditor);
}

void MessageProcessor::removeMessageEditor(int AOrder, IMessageEditor *AEditor)
{
	if (FMessageEditors.contains(AOrder,AEditor))
		FMessageEditors.remove(AOrder,AEditor);
}

int MessageProcessor::newMessageId()
{
	static int messageId = 1;
	return messageId++;
}

IMessageHandler *MessageProcessor::findMessageHandler(const Message &AMessage, int ADirection)
{
	for (QMultiMap<int, IMessageHandler *>::const_iterator it = FMessageHandlers.constBegin(); it!=FMessageHandlers.constEnd(); ++it)
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

QString MessageProcessor::convertTextToBody(const QString &AString) const
{
	QString result = AString;
	result.remove(QChar::Null);
	result.remove(QChar::ObjectReplacementCharacter);
	return result;
}

QString MessageProcessor::convertBodyToHtml(const QString &AString) const
{
	QString result = AString.toHtmlEscaped();
	result.replace('\n',"<br>");
	result.replace("  ","&nbsp; ");
	result.replace('\t',"&nbsp; &nbsp; ");
	return result;
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

void MessageProcessor::onXmppStreamActiveChanged(IXmppStream *AXmppStream, bool AActive)
{
	if (AActive)
		appendActiveStream(AXmppStream->streamJid());
	else
		removeActiveStream(AXmppStream->streamJid());
}

void MessageProcessor::onXmppStreamJidChanged(IXmppStream *AXmppStream, const Jid &ABefore)
{
	if (FActiveStreams.contains(ABefore))
	{
		int handleId = FActiveStreams.take(ABefore);
		FActiveStreams.insert(AXmppStream->streamJid(),handleId);
	}
}
