#include "pepmanager.h"

#include <definitions/namespaces.h>

#define SHC_PUBSUB_EVENT               "/message/event[@xmlns='" NS_PUBSUB_EVENT "']/items"

#define NODE_NOTIFY_SUFFIX             "+notify"

PEPManager::PEPManager()
{
	FDisco = NULL;
	FStanzaProcessor = NULL;
}

PEPManager::~PEPManager()
{
}

void PEPManager::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("PEP Manager");
	APluginInfo->description = tr("Allows other plugins to recieve and publish PEP events");
	APluginInfo->version = "0.9";
	APluginInfo->author = "Maxim Ignatenko";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
	APluginInfo->dependences.append(SERVICEDISCOVERY_UUID);
	APluginInfo->dependences.append(XMPPSTREAMS_UUID);
}

bool PEPManager::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
	IPlugin *plugin = APluginManager->pluginInterface("IServiceDiscovery").value(0,NULL);
	if (plugin)
	{
		FDisco = qobject_cast<IServiceDiscovery *>(plugin->instance());
		if (FDisco)
			connect(FDisco->instance(), SIGNAL(discoInfoReceived(const IDiscoInfo &)),
					SLOT(onDiscoInfoReceived(const IDiscoInfo &)));
	}
	plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0,NULL);
	if (plugin)
	{
		FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());
	}
	plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
	if (plugin)
	{
		IXmppStreams *FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
		if (FXmppStreams)
		{
			connect(FXmppStreams->instance(),SIGNAL(opened(IXmppStream *)),SLOT(onStreamOpened(IXmppStream *)));
			connect(FXmppStreams->instance(),SIGNAL(jidChanged(IXmppStream *, const Jid &)),
			        SLOT(onStreamJidChanged(IXmppStream *, const Jid &)));
			connect(FXmppStreams->instance(),SIGNAL(closed(IXmppStream *)),SLOT(onStreamClosed(IXmppStream *)));
		}
	}
	return (FDisco != NULL && FStanzaProcessor != NULL);
}

bool PEPManager::initObjects()
{
	return true;
}

bool PEPManager::initSettings()
{
	return true;
}

bool PEPManager::startPlugin()
{
	return true;
}

bool PEPManager::publishItem(const QString &ANode, const QDomElement &AItem)
{
	Stanza iq("iq");
	iq.setType("set");
	QDomElement publish = iq.addElement("pubsub", NS_PUBSUB).appendChild(QDomDocument().createElement("publish")).toElement();
	publish.setAttribute("node", ANode);
	publish.appendChild(AItem);
	bool sent = false;
	foreach(Jid streamJid, pepCapableStreams)
	{
		sent = FStanzaProcessor->sendStanzaOut(streamJid, iq) || sent;
	}
	return sent;
}

bool PEPManager::publishItem(const Jid &AStreamJid, const QString &ANode, const QDomElement &AItem)
{
	Stanza iq("iq");
	iq.setType("set");
	QDomElement publish = iq.addElement("pubsub", NS_PUBSUB).appendChild(QDomDocument().createElement("publish")).toElement();
	publish.setAttribute("node", ANode);
	publish.appendChild(AItem);
	return pepCapableStreams.contains(AStreamJid) && FStanzaProcessor->sendStanzaOut(AStreamJid, iq);
}

int PEPManager::insertNodeHandler(const QString &ANode, IPEPHandler *AHandle)
{
	static int handleId = 0;
	handleId++;
	while(handleId <= 0 || handlersById.contains(handleId))
		handleId = (handleId > 0) ? handleId+1 : 1;
	handlersById.insert(handleId, AHandle);
	handlersByNode.insert(ANode, handleId);
	connect(AHandle->instance(),SIGNAL(destroyed(QObject *)),SLOT(onPEPHandlerDestroyed(QObject *)));

	if (FDisco)
	{
		IDiscoFeature feature;
		feature.active = true;

		AHandle->fillDiscoFeature(ANode, feature);
		feature.var = ANode;
		FDisco->insertDiscoFeature(feature);

		AHandle->fillDiscoFeature(ANode + NODE_NOTIFY_SUFFIX, feature);
		feature.var = ANode + NODE_NOTIFY_SUFFIX;
		FDisco->insertDiscoFeature(feature);
	}
	return handleId;
}

bool PEPManager::removeNodeHandler(int AHandleId)
{
	if (handlersById.contains(AHandleId))
	{
		QList<QString> nodes = handlersByNode.keys(AHandleId);
		foreach(QString node, nodes)
		{
			handlersByNode.remove(node, AHandleId);
		}
		handlersById.remove(AHandleId);
		return true;
	}
	return false;
}

IPEPHandler* PEPManager::nodeHandler(int AHandleId)
{
	return handlersById.value(AHandleId, NULL);
}

bool PEPManager::stanzaEdit(int AHandleId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept)
{
	Q_UNUSED(AHandleId);
	Q_UNUSED(AStreamJid);
	Q_UNUSED(AStanza);
	Q_UNUSED(AAccept);
	return false;
}

bool PEPManager::stanzaRead(int AHandleId, const Jid &AStreamJid, const Stanza &AStanza, bool &AAccept)
{
	Q_UNUSED(AHandleId);
	Q_UNUSED(AStreamJid);

	bool hooked = false;
	QString node = AStanza.firstElement("event", NS_PUBSUB_EVENT)
	                      .firstChildElement("items").attribute("node", QString::null);
	QList<int> handlers = handlersByNode.values(node);
	foreach(int handlerId, handlers)
	{
		if (handlersById.contains(handlerId))
			hooked = handlersById[handlerId]->processPEPEvent(AStanza) || hooked;
	}
	AAccept = AAccept || hooked;
	return false;
}

void PEPManager::onStreamOpened(IXmppStream *AXmppStream)
{
	if (FDisco->requestDiscoInfo(AXmppStream->streamJid(), AXmppStream->streamJid().pBare()))
		unverifiedStreams.append(AXmppStream->streamJid());

	IStanzaHandle shandle;
	shandle.handler = this;
	shandle.order = SHO_DEFAULT;
	shandle.direction = IStanzaHandle::DirectionIn;
	shandle.streamJid = AXmppStream->streamJid();
	shandle.conditions.append(SHC_PUBSUB_EVENT);
	stanzaHandles.insert(AXmppStream->streamJid(), FStanzaProcessor->insertStanzaHandle(shandle));
}

void PEPManager::onStreamJidChanged(IXmppStream *AXmppStream, const Jid &ABefore)
{
	int i;
	for(i=0; i<pepCapableStreams.size(); i++)
	{
		if (pepCapableStreams.at(i) == ABefore)
			pepCapableStreams.replace(i, AXmppStream->streamJid());
	}
	for(i=0; i<unverifiedStreams.size(); i++)
	{
		if (unverifiedStreams.at(i) == ABefore)
			unverifiedStreams.replace(i, AXmppStream->streamJid());
	}
	if (stanzaHandles.contains(ABefore))
	{
		stanzaHandles.insert(AXmppStream->streamJid(), stanzaHandles[ABefore]);
		stanzaHandles.remove(ABefore);
		// we do not need to remove and insert StanzaHandle with new streamJid
	}
}

void PEPManager::onStreamClosed(IXmppStream *AXmppStream)
{
	pepCapableStreams.removeAll(AXmppStream->streamJid());
	unverifiedStreams.removeAll(AXmppStream->streamJid());
	if (stanzaHandles.contains(AXmppStream->streamJid()))
	{
		FStanzaProcessor->removeStanzaHandle(stanzaHandles[AXmppStream->streamJid()]);
		stanzaHandles.remove(AXmppStream->streamJid());
	}
}

void PEPManager::onPEPHandlerDestroyed(QObject *AHandler)
{
	IPEPHandler* handler = qobject_cast<IPEPHandler *>(AHandler);
	QList<int> ids = handlersById.keys(handler);
	foreach(int id, ids)
	{
		removeNodeHandler(id);
	}
}

void PEPManager::onDiscoInfoReceived(const IDiscoInfo &ADiscoInfo)
{
	if (ADiscoInfo.streamJid.pBare() != ADiscoInfo.contactJid.pFull())
		return; // we requested disco#info from ourself bare JID

	bool pepCapable = false;
	foreach(IDiscoIdentity identity, ADiscoInfo.identity)
	{
		if (identity.category == "pubsub" && identity.type == "pep")
			pepCapable = true;
	}

	if (unverifiedStreams.contains(ADiscoInfo.streamJid))
	{
		unverifiedStreams.removeAll(ADiscoInfo.streamJid);
		if (pepCapable)
			pepCapableStreams.append(ADiscoInfo.streamJid);
	}
}

Q_EXPORT_PLUGIN2(plg_pep, PEPManager)
