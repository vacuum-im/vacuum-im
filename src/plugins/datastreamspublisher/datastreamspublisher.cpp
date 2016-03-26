#include "datastreamspublisher.h"

#include <QUuid>
#include <definitions/namespaces.h>
#include <definitions/stanzahandlerorders.h>
#include <utils/logger.h>

#define STREAM_START_TIMEOUT   30000

#define SHC_STREAM_START       "/iq[@type='get']/start[@xmlns='" NS_STREAM_PUBLICATION "']"

DataStreamsPublisher::DataStreamsPublisher()
{

}

DataStreamsPublisher::~DataStreamsPublisher()
{

}

void DataStreamsPublisher::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Data Streams Publisher");
	APluginInfo->description = tr("Allows to publish available data streams");
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->version = "1.0";
	APluginInfo->homePage = "http://www.vacuum-im.org";
}

bool DataStreamsPublisher::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(APluginManager); Q_UNUSED(AInitOrder);
	return true;
}

bool DataStreamsPublisher::initObjects()
{
	if (FDiscovery)
	{
		IDiscoFeature sipub;
		sipub.active = true;
		sipub.var = NS_STREAM_PUBLICATION;
		sipub.name = tr("Data Streams Publication");
		sipub.description = tr("Supports the publication of the data streams");
		FDiscovery->insertDiscoFeature(sipub);
	}
	
	if (FStanzaProcessor)
	{
		IStanzaHandle handle;
		handle.handler = this;
		handle.order = SHO_DEFAULT;
		handle.direction = IStanzaHandle::DirectionIn;
		handle.conditions.append(SHC_STREAM_START);
		FSHIStreamStart = FStanzaProcessor->insertStanzaHandle(handle);
	}

	return true;
}

bool DataStreamsPublisher::stanzaReadWrite(int AHandleId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept)
{
	if (AHandleId == FSHIStreamStart)
	{
		AAccept = true;
		QString streamId = AStanza.firstElement("start", NS_STREAM_PUBLICATION).attribute("id");
		const IPublicDataStream stream = FStreams.value(streamId);
		if (!stream.isNull())
		{
			foreach(IPublicDataStreamHandler *handler, FHandlers)
			{
				if (handler->publicDataStreamCanStart(stream))
				{
					QString sessionId = QUuid::createUuid().toString();
					LOG_STRM_INFO(AStreamJid,QString("Start published data stream request accepted, from=%1, sid=%2, id=%3").arg(AStanza.from(),sessionId,streamId));

					Stanza reply = FStanzaProcessor->makeReplyResult(AStanza);
					reply.addElement("starting", NS_STREAM_PUBLICATION).setAttribute("sid",sessionId);
					FStanzaProcessor->sendStanzaOut(AStreamJid,reply);

					if (!handler->publicDataStreamStart(AStreamJid,AStanza.from(),sessionId,stream))
						LOG_STRM_ERROR(AStreamJid,QString("Failed to start accepted published data stream request, from=%1, id=%2, profile=%3").arg(AStanza.from(),streamId,stream.profile));

					return true;
				}
			}

			LOG_STRM_INFO(AStreamJid,QString("Failed to accept start published data stream request, from=%1, id=%2: Stream not started").arg(AStanza.from(),streamId));
			Stanza err = FStanzaProcessor->makeReplyError(AStanza, XmppStanzaError::EC_RESOURCE_CONSTRAINT);
			FStanzaProcessor->sendStanzaOut(AStreamJid,err);
		}
		else
		{
			LOG_STRM_INFO(AStreamJid,QString("Failed to accept start published data stream request, from=%1, id=%2: Stream not found").arg(AStanza.from(),streamId));
			Stanza err = FStanzaProcessor->makeReplyError(AStanza, XmppStanzaError::EC_NOT_ACCEPTABLE);
			FStanzaProcessor->sendStanzaOut(AStreamJid,err);
		}
		return true;
	}
	return false;
}

void DataStreamsPublisher::stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza)
{
	if (FStartRequest.contains(AStanza.id()))
	{
		QString sessionId = FStartRequest.take(AStanza.id());
		if (AStanza.isResult())
		{
			LOG_STRM_INFO(AStreamJid,QString("Start public data stream request accepted, sid=%1, id=%2").arg(sessionId,AStanza.id()));
			QDomElement startElem = AStanza.firstElement("starting", NS_STREAM_PUBLICATION);
			emit streamStartAccepted(AStanza.id(),startElem.attribute("sid"));
		}
		else
		{
			XmppStanzaError err(AStanza);
			LOG_STRM_INFO(AStreamJid,QString("Start public data stream request rejected, sid=%1, id=%2: %3").arg(sessionId,AStanza.id(),err.condition()));
			emit streamStartRejected(AStanza.id(),err);
		}
	}
}

QList<QString> DataStreamsPublisher::streams() const
{
	return FStreams.keys();
}

IPublicDataStream DataStreamsPublisher::findStream(const QString &AStreamId) const
{
	return FStreams.value(AStreamId);
}

bool DataStreamsPublisher::publishStream(const IPublicDataStream &AStream)
{
	if (AStream.isValid() && !FStreams.contains(AStream.id))
	{
		FStreams.insert(AStream.id,AStream);
		LOG_INFO(QString("Registered public data stream, owner=%1, id=%2, profile=%3").arg(AStream.ownerJid.full(),AStream.id,AStream.profile));
		emit streamPublished(AStream);
		return true;
	}
	return false;
}

void DataStreamsPublisher::removeStream(const QString &AStreamId)
{
	if (FStreams.contains(AStreamId))
	{
		IPublicDataStream stream = FStreams.take(AStreamId);
		LOG_INFO(QString("Removed public data stream, owner=%1, id=%2, profile=%3").arg(stream.ownerJid.full(),stream.id,stream.profile));
		emit streamRemoved(stream);
	}
}

QList<IPublicDataStream> DataStreamsPublisher::readStreams(const QDomElement &AParent) const
{
	QList<IPublicDataStream> streamList;
	if (!AParent.isNull())
	{
		QDomElement sipubElem = AParent.firstChildElement("sipub");
		while (!sipubElem.isNull())
		{
			if (sipubElem.namespaceURI() == NS_STREAM_PUBLICATION)
			{
				IPublicDataStream stream;
				stream.id = sipubElem.attribute("id");
				stream.ownerJid = sipubElem.attribute("from");
				stream.profile = sipubElem.attribute("profile");
				stream.mimeType = sipubElem.attribute("mime-type");

				if (stream.isValid())
				{
					foreach(IPublicDataStreamHandler *handler, FHandlers)
					{
						if (handler->publicDataStreamRead(stream,sipubElem))
						{
							streamList.append(stream);
							break;
						}
					}
				}
			}
			sipubElem = sipubElem.nextSiblingElement("sipub");
		}
	}
	else
	{
		REPORT_ERROR("Failed to read public data streams: Invalid parameters");
	}
	return streamList;
}

bool DataStreamsPublisher::writeStream(const QString &AStreamId, QDomElement &AParent) const
{
	const IPublicDataStream stream = findStream(AStreamId);
	if (stream.isValid() && !AParent.isNull())
	{
		QDomElement sipubElem = AParent.ownerDocument().createElementNS(NS_STREAM_PUBLICATION,"sipub");
		sipubElem.setAttribute("id",stream.id);
		sipubElem.setAttribute("from",stream.ownerJid.full());
		sipubElem.setAttribute("profile",stream.profile);
		
		if (!stream.mimeType.isEmpty())
			sipubElem.setAttribute("mime-type", stream.profile);

		foreach(IPublicDataStreamHandler *handler, FHandlers)
		{
			if (handler->publicDataStreamWrite(stream,sipubElem))
			{
				AParent.appendChild(sipubElem);
				return true;
			}
		}

		LOG_WARNING(QString("Failed to write public data stream, id=%1: Handler not found").arg(AStreamId));
	}
	else if (stream.isValid())
	{
		REPORT_ERROR("Failed to write public data stream: Invalid parameters");
	}
	return false;
}

bool DataStreamsPublisher::isSupported(const Jid &AStreamJid, const Jid &AContactJid) const
{
	return FDiscovery==NULL || FDiscovery->discoInfo(AStreamJid,AContactJid).features.contains(NS_STREAM_PUBLICATION);
}

QString DataStreamsPublisher::startStream(const Jid &AStreamJid, const Jid &AContactJid, const QString &AStreamId)
{
	if (FStanzaProcessor && AStreamJid.isValid() && AContactJid.isValid() && !AStreamId.isEmpty())
	{
		Stanza request(STANZA_KIND_IQ);
		request.setType(STANZA_TYPE_GET).setTo(AContactJid.full()).setUniqueId();

		QDomElement startElem = request.addElement("start",NS_STREAM_PUBLICATION);
		startElem.setAttribute("id",AStreamId);

		if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,request,STREAM_START_TIMEOUT))
		{
			LOG_STRM_INFO(AStreamJid,QString("Start public data stream request sent, to=%1, sid=%2, id=%3").arg(AContactJid.full(),AStreamId,request.id()));
			FStartRequest.insert(request.id(),AStreamId);
			return request.id();
		}
		else
		{
			LOG_STRM_WARNING(AStreamJid,QString("Failed to send start public data stream request, to=%1, sid=%2").arg(AContactJid.full(),AStreamId));
		}
	}
	else if (FStanzaProcessor)
	{
		REPORT_ERROR("Failed to send start public data stream request: Invalid parameters");
	}
	return QString::null;
}

QList<IPublicDataStreamHandler *> DataStreamsPublisher::streamHandlers() const
{
	return FHandlers.values();
}

void DataStreamsPublisher::insertStreamHandler(int AOrder, IPublicDataStreamHandler *AHandler)
{
	if (!FHandlers.contains(AOrder,AHandler))
	{
		FHandlers.insertMulti(AOrder,AHandler);
		emit streamHandlerInserted(AOrder,AHandler);
	}
}

void DataStreamsPublisher::removeStreamHandler(int AOrder, IPublicDataStreamHandler *AHandler)
{
	if (FHandlers.contains(AOrder,AHandler))
	{
		FHandlers.remove(AOrder,AHandler);
		emit streamHandlerRemoved(AOrder,AHandler);
	}
}

Q_EXPORT_PLUGIN2(plg_datastreamspublisher, DataStreamsPublisher);
