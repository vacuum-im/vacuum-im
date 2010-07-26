#include "stanzaprocessor.h"

#include <QSet>

StanzaProcessor::StanzaProcessor()
{

}

StanzaProcessor::~StanzaProcessor()
{

}

void StanzaProcessor::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Stanza Manager");
	APluginInfo->description = tr("Allows other modules to send and receive XMPP stanzas");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(XMPPSTREAMS_UUID);
}

bool StanzaProcessor::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
	IPlugin *plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
	if (plugin)
	{
		FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
		if (FXmppStreams)
		{
			connect(FXmppStreams->instance(), SIGNAL(created(IXmppStream *)),
			        SLOT(onStreamCreated(IXmppStream *)));
			connect(FXmppStreams->instance(), SIGNAL(jidChanged(IXmppStream *, const Jid &)),
			        SLOT(onStreamJidChanged(IXmppStream *, const Jid &)));
			connect(FXmppStreams->instance(), SIGNAL(closed(IXmppStream *)),
			        SLOT(onStreamClosed(IXmppStream *)));
			connect(FXmppStreams->instance(), SIGNAL(streamDestroyed(IXmppStream *)),
			        SLOT(onStreamDestroyed(IXmppStream *)));
		}
	}
	return FXmppStreams!=NULL;
}

//IXmppStanzaHandler
bool StanzaProcessor::xmppStanzaIn(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder)
{
	if (AOrder == XSHO_STANZAPROCESSOR)
	{
		if (AStanza.from().isEmpty())
			AStanza.setFrom(AXmppStream->streamJid().eFull());
		AStanza.setTo(AXmppStream->streamJid().eFull());

		if (!sendStanzaIn(AXmppStream->streamJid(),AStanza))
		{
			if (AStanza.canReplyError())
			{
				Stanza reply = AStanza.replyError("service-unavailable");
				sendStanzaOut(AXmppStream->streamJid(), reply);
			}
		}
	}
	return false;
}

bool StanzaProcessor::xmppStanzaOut(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder)
{
	Q_UNUSED(AXmppStream);
	Q_UNUSED(AStanza);
	Q_UNUSED(AOrder);
	return false;
}

//IStanzaProcessor
QString StanzaProcessor::newId() const
{
	static unsigned long id = 1;
	static const QString mask = "sid_%1";
	return mask.arg(id++);
}

bool StanzaProcessor::sendStanzaIn(const Jid &AStreamJid, Stanza &AStanza)
{
	emit stanzaReceived(AStreamJid, AStanza);
	bool acceptedIn = processStanzaIn(AStreamJid,AStanza);
	bool acceptedIq = processStanzaRequest(AStreamJid,AStanza);
	return acceptedIn || acceptedIq;
}

bool StanzaProcessor::sendStanzaOut(const Jid &AStreamJid, Stanza &AStanza)
{
	if (!processStanzaOut(AStreamJid,AStanza))
	{
		IXmppStream *stream = FXmppStreams->xmppStream(AStreamJid);
		if (stream && stream->sendStanza(AStanza)>=0)
		{
			emit stanzaSent(AStreamJid, AStanza);
			return true;
		}
	}
	return false;
}

bool StanzaProcessor::sendStanzaRequest(IStanzaRequestOwner *AIqOwner, const Jid &AStreamJid, Stanza &AStanza, int ATimeout)
{
	if (AIqOwner && AStanza.tagName()=="iq" && !AStanza.id().isEmpty() && !FRequests.contains(AStanza.id()))
	{
		if ((AStanza.type() == "set" || AStanza.type() == "get") && sendStanzaOut(AStreamJid,AStanza))
		{
			StanzaRequest request;
			request.streamJid = AStreamJid;
			request.owner = AIqOwner;
			if (ATimeout > 0)
			{
				request.timer = new QTimer;
				request.timer->setSingleShot(true);
				connect(request.timer,SIGNAL(timeout()),SLOT(onStanzaRequestTimeout()));
				request.timer->start(ATimeout);
			}
			FRequests.insert(AStanza.id(),request);
			connect(AIqOwner->instance(),SIGNAL(destroyed(QObject *)),SLOT(onStanzaRequestOwnerDestroyed(QObject *)));
			return true;
		}
	}
	return false;
}

QList<int> StanzaProcessor::stanzaHandles() const
{
	return FHandles.keys();
}

IStanzaHandle StanzaProcessor::stanzaHandle(int AHandleId) const
{
	return FHandles.value(AHandleId);
}

int StanzaProcessor::insertStanzaHandle(const IStanzaHandle &AHandle)
{
	if (AHandle.handler!=NULL && !AHandle.conditions.isEmpty())
	{
		int handleId = qrand();
		while(handleId<=0 || FHandles.contains(handleId))
			handleId++;
		FHandles.insert(handleId,AHandle);
		FHandleIdByOrder.insertMulti(AHandle.order,handleId);
		connect(AHandle.handler->instance(),SIGNAL(destroyed(QObject *)),SLOT(onStanzaHandlerDestroyed(QObject *)));
		emit stanzaHandleInserted(handleId,AHandle);
		return handleId;
	}
	return -1;
}

void StanzaProcessor::removeStanzaHandle(int AHandleId)
{
	if (FHandles.contains(AHandleId))
	{
		IStanzaHandle shandle = FHandles.take(AHandleId);
		FHandleIdByOrder.remove(shandle.order,AHandleId);
		emit stanzaHandleRemoved(AHandleId, shandle);
	}
}

bool StanzaProcessor::checkStanza(const Stanza &AStanza, const QString &ACondition) const
{
	return checkCondition(AStanza.element(),ACondition);
}

bool StanzaProcessor::checkCondition(const QDomElement &AElem, const QString &ACondition, const int APos) const
{
	static QSet<QChar> delimiters = QSet<QChar>()<<' '<<'/'<<'\\'<<'\t'<<'\n'<<'['<<']'<<'='<<'\''<<'"'<<'@';

	QDomElement elem = AElem;

	int pos = APos;
	if (pos<ACondition.count() && ACondition[pos] == '/')
		pos++;

	QString tagName;
	while (pos<ACondition.count() && !delimiters.contains(ACondition[pos]))
		tagName.append(ACondition[pos++]);

	if (!tagName.isEmpty() &&  elem.tagName() != tagName)
		elem = elem.nextSiblingElement(tagName);

	if (elem.isNull())
		return false;

	QMultiHash<QString,QString> attributes;
	while (pos<ACondition.count() && ACondition[pos] != '/')
	{
		if (ACondition[pos] == '[')
		{
			pos++;
			QString attrName = "";
			QString attrValue = "";
			while (pos<ACondition.count() && ACondition[pos] != ']')
			{
				if (ACondition[pos] == '@')
				{
					pos++;
					while (pos<ACondition.count() && !delimiters.contains(ACondition[pos]))
						attrName.append(ACondition[pos++]);
				}
				else if (ACondition[pos] == '"' || ACondition[pos] == '\'')
				{
					QChar end = ACondition[pos++];
					while (pos<ACondition.count() && ACondition[pos] != end)
						attrValue.append(ACondition[pos++]);
					pos++;
				}
				else pos++;
			}
			if (!attrName.isEmpty())
				attributes.insertMulti(attrName,attrValue);
			pos++;
		}
		else pos++;
	}

	if (pos < ACondition.count() && !elem.hasChildNodes())
		return false;

	while (!elem.isNull())
	{
		int attr = 0;
		QList<QString> attrNames = attributes.keys();
		while (attr<attrNames.count() && !elem.isNull())
		{
			QString attrName = attrNames.at(attr);
			QList<QString> attrValues = attributes.values(attrName);
			bool attrBlankValue = attrValues.contains("");
			bool elemHasAttr;
			QString elemAttrValue;
			if (elem.hasAttribute(attrName))
			{
				elemHasAttr = true;
				elemAttrValue = elem.attribute(attrName);
			}
			else if (attrName == "xmlns")
			{
				elemHasAttr = true;
				elemAttrValue = elem.namespaceURI();
			}
			else
				elemHasAttr = false;

			if (!elemHasAttr || (!attrValues.contains(elemAttrValue) && !attrBlankValue))
			{
				elem = elem.nextSiblingElement(tagName);
				attr = 0;
			}
			else attr++;
		}

		if (!elem.isNull() && pos < ACondition.count())
		{
			if (checkCondition(elem.firstChildElement(),ACondition,pos))
				return true;
			else
				elem = elem.nextSiblingElement(tagName);
		}
		else if (!elem.isNull())
			return true;
	}

	return false;
}

bool StanzaProcessor::processStanzaIn(const Jid &AStreamJid, Stanza &AStanza) const
{
	bool hooked = false;
	bool accepted = false;
	QList<int> checkedHandles;

	QMapIterator<int, int> it(FHandleIdByOrder);
	while (!hooked && it.hasNext())
	{
		it.next();
		const IStanzaHandle &shandle = FHandles.value(it.value());
		if (shandle.direction==IStanzaHandle::DirectionIn && (shandle.streamJid.isEmpty() || shandle.streamJid==AStreamJid))
		{
			for (int i = 0; i<shandle.conditions.count(); i++)
			{
				if (checkCondition(AStanza.element(), shandle.conditions.at(i)))
				{
					hooked = shandle.handler->stanzaEdit(it.value(),AStreamJid,AStanza,accepted);
					checkedHandles.append(it.value());
					break;
				}
			}
		}
	}

	for (int i = 0; !hooked && i<checkedHandles.count(); i++)
	{
		int shandleId = checkedHandles.at(i);
		const IStanzaHandle &shandle = FHandles.value(shandleId);
		hooked = shandle.handler->stanzaRead(shandleId,AStreamJid,AStanza,accepted);
	}

	return accepted;
}

bool StanzaProcessor::processStanzaOut(const Jid &AStreamJid, Stanza &AStanza) const
{
	bool hooked = false;
	bool accepted = false;
	QList<int> checkedHandlers;

	QMapIterator<int, int> it(FHandleIdByOrder);
	while (!hooked && it.hasNext())
	{
		it.next();
		const IStanzaHandle &shandle = FHandles.value(it.value());
		if (shandle.direction==IStanzaHandle::DirectionOut && (shandle.streamJid.isEmpty() || shandle.streamJid==AStreamJid))
		{
			for (int i = 0; i<shandle.conditions.count(); i++)
			{
				if (checkCondition(AStanza.element(), shandle.conditions.at(i)))
				{
					hooked = shandle.handler->stanzaEdit(it.value(),AStreamJid,AStanza,accepted);
					checkedHandlers.append(it.value());
					break;
				}
			}
		}
	}

	for (int i = 0; !hooked && i<checkedHandlers.count(); i++)
	{
		int shandleId = checkedHandlers.at(i);
		const IStanzaHandle &shandle = FHandles.value(shandleId);
		hooked = shandle.handler->stanzaRead(shandleId,AStreamJid,AStanza,accepted);
	}

	return hooked;
}

bool StanzaProcessor::processStanzaRequest(const Jid &AStreamJid, const Stanza &AStanza)
{
	if (AStanza.tagName()=="iq" && FRequests.contains(AStanza.id()) && (AStanza.type()=="result" || AStanza.type()=="error"))
	{
		const StanzaRequest &request = FRequests.value(AStanza.id());
		request.owner->stanzaRequestResult(AStreamJid,AStanza);
		removeStanzaRequest(AStanza.id());
		return true;
	}
	return false;
}

void StanzaProcessor::removeStanzaRequest(const QString &AStanzaId)
{
	StanzaRequest request = FRequests.take(AStanzaId);
	delete request.timer;
}

void StanzaProcessor::onStreamCreated(IXmppStream *AXmppStream)
{
	AXmppStream->insertXmppStanzaHandler(this, XSHO_STANZAPROCESSOR);
}

void StanzaProcessor::onStreamJidChanged(IXmppStream *AXmppStream, const Jid &ABefour)
{
	foreach(int shandleId, FHandles.keys())
		if (FHandles.value(shandleId).streamJid == ABefour)
			FHandles[shandleId].streamJid = AXmppStream->streamJid();
}

void StanzaProcessor::onStreamClosed(IXmppStream *AXmppStream)
{
	foreach(QString stanzaId, FRequests.keys())
	{
		const StanzaRequest &request = FRequests.value(stanzaId);
		if (request.streamJid == AXmppStream->streamJid())
		{
			request.owner->stanzaRequestTimeout(request.streamJid, stanzaId);
			removeStanzaRequest(stanzaId);
		}
	}
}

void StanzaProcessor::onStreamDestroyed(IXmppStream *AXmppStream)
{
	AXmppStream->removeXmppStanzaHandler(this, XSHO_STANZAPROCESSOR);
}

void StanzaProcessor::onStanzaRequestTimeout()
{
	QTimer *timer = qobject_cast<QTimer *>(sender());
	if (timer != NULL)
	{
		foreach(QString stanzaId, FRequests.keys())
		{
			const StanzaRequest &request = FRequests.value(stanzaId);
			if (request.timer == timer)
			{
				request.owner->stanzaRequestTimeout(request.streamJid, stanzaId);
				removeStanzaRequest(stanzaId);
				break;
			}
		}
	}
}

void StanzaProcessor::onStanzaRequestOwnerDestroyed(QObject *AOwner)
{
	foreach(QString stanzaId, FRequests.keys())
		if (FRequests.value(stanzaId).owner->instance() == AOwner)
			removeStanzaRequest(stanzaId);
}

void StanzaProcessor::onStanzaHandlerDestroyed(QObject *AHandler)
{
	foreach (int shandleId, FHandles.keys())
		if (FHandles.value(shandleId).handler->instance() == AHandler)
			removeStanzaHandle(shandleId);
}

Q_EXPORT_PLUGIN2(plg_stanzaprocessor, StanzaProcessor)
