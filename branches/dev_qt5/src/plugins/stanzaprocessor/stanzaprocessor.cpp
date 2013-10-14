#include "stanzaprocessor.h"

#include <QSet>

StanzaProcessor::StanzaProcessor()
{
	FXmppStreams = NULL;
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

bool StanzaProcessor::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
	IPlugin *plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
	if (plugin)
	{
		FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
		if (FXmppStreams)
		{
			connect(FXmppStreams->instance(), SIGNAL(created(IXmppStream *)),SLOT(onStreamCreated(IXmppStream *)));
			connect(FXmppStreams->instance(), SIGNAL(jidChanged(IXmppStream *, const Jid &)),SLOT(onStreamJidChanged(IXmppStream *, const Jid &)));
			connect(FXmppStreams->instance(), SIGNAL(closed(IXmppStream *)),SLOT(onStreamClosed(IXmppStream *)));
			connect(FXmppStreams->instance(), SIGNAL(streamDestroyed(IXmppStream *)),SLOT(onStreamDestroyed(IXmppStream *)));
		}
	}
	return FXmppStreams!=NULL;
}

//IXmppStanzaHandler
bool StanzaProcessor::xmppStanzaIn(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder)
{
	if (AOrder == XSHO_STANZAPROCESSOR)
	{
		if (!sendStanzaIn(AXmppStream->streamJid(),AStanza))
		{
			if (AStanza.tagName()=="iq" && (AStanza.type()=="set" || AStanza.type()=="get"))
			{
				Stanza error = makeReplyError(AStanza,XmppStanzaError::EC_SERVICE_UNAVAILABLE);
				sendStanzaOut(AXmppStream->streamJid(), error);
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
	bool acceptedIn = processStanza(AStreamJid,AStanza,IStanzaHandle::DirectionIn);
	bool acceptedIq = processStanzaRequest(AStreamJid,AStanza);
	return acceptedIn || acceptedIq;
}

bool StanzaProcessor::sendStanzaOut(const Jid &AStreamJid, Stanza &AStanza)
{
	if (!processStanza(AStreamJid,AStanza,IStanzaHandle::DirectionOut))
	{
		IXmppStream *stream = FXmppStreams->xmppStream(AStreamJid);
		if (stream && stream->sendStanza(AStanza)>=0)
		{
			emit stanzaSent(AStreamJid, AStanza);
			return true;
		}
		return false;
	}
	return true;
}

bool StanzaProcessor::sendStanzaRequest(IStanzaRequestOwner *AIqOwner, const Jid &AStreamJid, Stanza &AStanza, int ATimeout)
{
	if (AIqOwner && AStanza.tagName()=="iq" && !AStanza.id().isEmpty() && !FRequests.contains(AStanza.id()))
	{
		if ((AStanza.type()=="set" || AStanza.type()=="get") && sendStanzaOut(AStreamJid,AStanza))
		{
			StanzaRequest request;
			request.owner = AIqOwner;
			request.streamJid = AStreamJid;
			request.contactJid = AStanza.to();
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

Stanza StanzaProcessor::makeReplyResult(const Stanza &AStanza) const
{
	Stanza result(AStanza.tagName());
	result.setType("result").setId(AStanza.id()).setTo(AStanza.from());
	return result;
}

Stanza StanzaProcessor::makeReplyError(const Stanza &AStanza, const XmppStanzaError &AError) const
{
	Stanza error(AStanza);
	error.setType("error").setId(AStanza.id()).setTo(AStanza.from()).setFrom(QString::null);
	insertErrorElement(error,AError);
	return error;
}

bool StanzaProcessor::checkStanza(const Stanza &AStanza, const QString &ACondition) const
{
	return checkCondition(AStanza.element(),ACondition);
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
		static int handleId = 0;
		handleId++;
		while(handleId <= 0 || FHandles.contains(handleId))
			handleId = (handleId > 0) ? handleId+1 : 1;
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

bool StanzaProcessor::checkCondition(const QDomElement &AElem, const QString &ACondition, int APos) const
{
	static const QSet<QChar> delimiters = QSet<QChar>()<<' '<<'/'<<'\\'<<'\t'<<'\n'<<'['<<']'<<'='<<'\''<<'"'<<'@';

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
			QString attrName;
			QString attrValue;
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
			bool attrBlankValue = attrValues.contains(QString::null);
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

bool StanzaProcessor::processStanza(const Jid &AStreamJid, Stanza &AStanza, int ADirection) const
{
	bool hooked = false;
	bool accepted = false;

	QMapIterator<int, int> it(FHandleIdByOrder);
	while (!hooked && it.hasNext())
	{
		it.next();
		const IStanzaHandle &shandle = FHandles.value(it.value());
		if (shandle.direction==ADirection && (shandle.streamJid.isEmpty() || shandle.streamJid==AStreamJid))
		{
			for (int i = 0; i<shandle.conditions.count(); i++)
			{
				if (checkCondition(AStanza.element(), shandle.conditions.at(i)))
				{
					hooked = shandle.handler->stanzaReadWrite(it.value(),AStreamJid,AStanza,accepted);
					break;
				}
			}
		}
	}

	return ADirection==IStanzaHandle::DirectionIn ? accepted : hooked;
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

void StanzaProcessor::processRequestTimeout(const QString &AStanzaId) const
{
	if (FRequests.contains(AStanzaId))
	{
		const StanzaRequest &request = FRequests.value(AStanzaId);

		Stanza timeout("iq");
		timeout.setType("error").setId(AStanzaId).setFrom(request.contactJid.full()).setTo(request.streamJid.full());
		insertErrorElement(timeout,XmppStanzaError(XmppStanzaError::EC_REMOTE_SERVER_TIMEOUT));

		request.owner->stanzaRequestResult(request.streamJid, timeout);
	}
}

void StanzaProcessor::removeStanzaRequest(const QString &AStanzaId)
{
	StanzaRequest request = FRequests.take(AStanzaId);
	delete request.timer;
}

void StanzaProcessor::insertErrorElement(Stanza &AStanza, const XmppStanzaError &AError) const
{
	QDomElement errElem = AStanza.addElement("error");
	if (AError.errorTypeCode() != XmppStanzaError::ET_UNKNOWN)
		errElem.setAttribute("type",AError.errorType());
	if (!AError.condition().isEmpty())
	{
		QDomNode condElem = errElem.appendChild(AStanza.createElement(AError.condition(),NS_XMPP_STANZA_ERROR));
		if (!AError.conditionText().isEmpty())
			condElem.appendChild(AStanza.createTextNode(AError.conditionText()));
	}
	if (!AError.errorText().isEmpty())
		errElem.appendChild(AStanza.createElement("text",NS_XMPP_STANZA_ERROR)).appendChild(AStanza.createTextNode(AError.errorText()));
	foreach(const QString &appCondNs, AError.appConditionNsList())
		errElem.appendChild(AStanza.createElement(AError.appCondition(appCondNs),appCondNs));
}

void StanzaProcessor::onStreamCreated(IXmppStream *AXmppStream)
{
	AXmppStream->insertXmppStanzaHandler(XSHO_STANZAPROCESSOR,this);
}

void StanzaProcessor::onStreamJidChanged(IXmppStream *AXmppStream, const Jid &ABefore)
{
	foreach(int shandleId, FHandles.keys())
		if (FHandles.value(shandleId).streamJid == ABefore)
			FHandles[shandleId].streamJid = AXmppStream->streamJid();
}

void StanzaProcessor::onStreamClosed(IXmppStream *AXmppStream)
{
	foreach(const QString &stanzaId, FRequests.keys())
	{
		const StanzaRequest &request = FRequests.value(stanzaId);
		if (request.streamJid == AXmppStream->streamJid())
		{
			processRequestTimeout(stanzaId);
			removeStanzaRequest(stanzaId);
		}
	}
}

void StanzaProcessor::onStreamDestroyed(IXmppStream *AXmppStream)
{
	AXmppStream->removeXmppStanzaHandler(XSHO_STANZAPROCESSOR,this);
}

void StanzaProcessor::onStanzaRequestTimeout()
{
	QTimer *timer = qobject_cast<QTimer *>(sender());
	if (timer != NULL)
	{
		for(QMap<QString,StanzaRequest>::const_iterator it=FRequests.constBegin(); it!=FRequests.constEnd(); ++it)
		{
			if (it->timer == timer)
			{
				processRequestTimeout(it.key());
				removeStanzaRequest(it.key());
				break;
			}
		}
	}
}

void StanzaProcessor::onStanzaRequestOwnerDestroyed(QObject *AOwner)
{
	foreach(const QString &stanzaId, FRequests.keys())
		if (FRequests.value(stanzaId).owner->instance() == AOwner)
			removeStanzaRequest(stanzaId);
}

void StanzaProcessor::onStanzaHandlerDestroyed(QObject *AHandler)
{
	foreach (int shandleId, FHandles.keys())
		if (FHandles.value(shandleId).handler->instance() == AHandler)
			removeStanzaHandle(shandleId);
}
