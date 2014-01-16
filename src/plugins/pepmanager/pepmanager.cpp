#include "pepmanager.h"

#include <definitions/namespaces.h>
#include <utils/logger.h>

#define SHC_PUBSUB_EVENT               "/message/event[@xmlns='" NS_PUBSUB_EVENT "']/items"

#define NODE_NOTIFY_SUFFIX             "+notify"

#define DIC_PUBSUB                     "pubsub"
#define DIT_PEP                        "pep"

PEPManager::PEPManager()
{
	FDiscovery = NULL;
	FXmppStreams = NULL;
	FStanzaProcessor = NULL;
}

PEPManager::~PEPManager()
{

}

void PEPManager::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("PEP Manager");
	APluginInfo->description = tr("Allows other plugins to receive and publish PEP events");
	APluginInfo->version = "0.9";
	APluginInfo->author = "Maxim Ignatenko";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(SERVICEDISCOVERY_UUID);
	APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
	APluginInfo->dependences.append(XMPPSTREAMS_UUID);
}

bool PEPManager::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
	IPlugin *plugin = APluginManager->pluginInterface("IServiceDiscovery").value(0,NULL);
	if (plugin)
	{
		FDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0,NULL);
	if (plugin)
	{
		FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
	if (plugin)
	{
		FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
		if (FXmppStreams)
		{
			connect(FXmppStreams->instance(),SIGNAL(opened(IXmppStream *)),SLOT(onStreamOpened(IXmppStream *)));
			connect(FXmppStreams->instance(),SIGNAL(closed(IXmppStream *)),SLOT(onStreamClosed(IXmppStream *)));
		}
	}
	return FDiscovery!=NULL && FStanzaProcessor!=NULL && FXmppStreams!=NULL;
}

bool PEPManager::stanzaReadWrite(int AHandleId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept)
{
	if (FStanzaHandles.value(AStreamJid) == AHandleId)
	{
		bool hooked = false;
		QString node = AStanza.firstElement("event",NS_PUBSUB_EVENT).firstChildElement("items").attribute("node",QString::null);

		foreach(int handlerId, FHandlersByNode.values(node))
		{
			if (FHandlersById.contains(handlerId))
				hooked = FHandlersById[handlerId]->processPEPEvent(AStreamJid,AStanza) || hooked;
		}
		AAccept = AAccept || hooked;
	}
	return false;
}

bool PEPManager::isSupported(const Jid &AStreamJid) const
{
	bool supported = false;
	IDiscoInfo dinfo = FDiscovery!=NULL ? FDiscovery->discoInfo(AStreamJid, AStreamJid.domain()) : IDiscoInfo();
	for (int i=0; !supported && i<dinfo.identity.count(); i++)
	{
		const IDiscoIdentity &ident = dinfo.identity.at(i);
		supported = ident.category==DIC_PUBSUB && ident.type==DIT_PEP;
	}
	return supported;
}

bool PEPManager::publishItem(const Jid &AStreamJid, const QString &ANode, const QDomElement &AItem)
{
	if (FStanzaProcessor && isSupported(AStreamJid))
	{
		Stanza iq("iq");
		iq.setType("set").setId(FStanzaProcessor->newId());
		QDomElement publish = iq.addElement("pubsub", NS_PUBSUB).appendChild(iq.createElement("publish")).toElement();
		publish.setAttribute("node", ANode);
		publish.appendChild(AItem.cloneNode(true));
		if (FStanzaProcessor->sendStanzaOut(AStreamJid,iq))
		{
			LOG_STRM_INFO(AStreamJid,QString("PEP item publish request sent, node=%1, id=%2").arg(ANode,iq.id()));
			return true;
		}
		else
		{
			LOG_STRM_WARNING(AStreamJid,QString("Failed to send PEP item publish request, node=%1").arg(ANode));
		}
	}
	else if (FStanzaProcessor)
	{
		LOG_STRM_ERROR(AStreamJid,QString("Failed to publish PEP item, node=%1: Not supported").arg(ANode));
	}
	return false;
}

IPEPHandler *PEPManager::nodeHandler(int AHandleId) const
{
	return FHandlersById.value(AHandleId, NULL);
}

int PEPManager::insertNodeHandler(const QString &ANode, IPEPHandler *AHandle)
{
	static int handleId = 0;

	handleId++;
	while(handleId <= 0 || FHandlersById.contains(handleId))
		handleId = (handleId > 0) ? handleId+1 : 1;

	FHandlersById.insert(handleId, AHandle);
	FHandlersByNode.insertMulti(ANode, handleId);
	connect(AHandle->instance(),SIGNAL(destroyed(QObject *)),SLOT(onPEPHandlerDestroyed(QObject *)));

	return handleId;
}

bool PEPManager::removeNodeHandler(int AHandleId)
{
	if (FHandlersById.contains(AHandleId))
	{
		QList<QString> nodes = FHandlersByNode.keys(AHandleId);
		foreach(const QString &node, nodes)
			FHandlersByNode.remove(node, AHandleId);
		FHandlersById.remove(AHandleId);
		return true;
	}
	return false;
}

void PEPManager::onStreamOpened(IXmppStream *AXmppStream)
{
	if (FStanzaProcessor)
	{
		IStanzaHandle shandle;
		shandle.handler = this;
		shandle.order = SHO_DEFAULT;
		shandle.direction = IStanzaHandle::DirectionIn;
		shandle.streamJid = AXmppStream->streamJid();
		shandle.conditions.append(SHC_PUBSUB_EVENT);
		FStanzaHandles.insert(AXmppStream->streamJid(), FStanzaProcessor->insertStanzaHandle(shandle));
	}

	if (FDiscovery)
		FDiscovery->requestDiscoInfo(AXmppStream->streamJid(), AXmppStream->streamJid().domain());
}

void PEPManager::onStreamClosed(IXmppStream *AXmppStream)
{
	if (FStanzaProcessor)
		FStanzaProcessor->removeStanzaHandle(FStanzaHandles.take(AXmppStream->streamJid()));
}

void PEPManager::onPEPHandlerDestroyed(QObject *AHandler)
{
	foreach(int id, FHandlersById.keys())
	{
		IPEPHandler *handler = FHandlersById.value(id);
		if (handler->instance() == AHandler)
		{
			removeNodeHandler(id);
			break;
		}
	}
}
