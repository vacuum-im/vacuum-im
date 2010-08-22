#include "privatestorage.h"

#define PRIVATE_STORAGE_TIMEOUT       30000

PrivateStorage::PrivateStorage()
{
	FStanzaProcessor = NULL;
}

PrivateStorage::~PrivateStorage()
{

}

//IPlugin
void PrivateStorage::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Private Storage");
	APluginInfo->description = tr("Allows other modules to store arbitrary data on a server");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool PrivateStorage::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);

	IPlugin *plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
	if (plugin)
	{
		connect(plugin->instance(), SIGNAL(opened(IXmppStream *)), SLOT(onStreamOpened(IXmppStream *)));
		connect(plugin->instance(), SIGNAL(aboutToClose(IXmppStream *)), SLOT(onStreamAboutToClose(IXmppStream *)));
		connect(plugin->instance(), SIGNAL(closed(IXmppStream *)), SLOT(onStreamClosed(IXmppStream *)));
	}

	plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0,NULL);
	if (plugin)
		FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());

	return FStanzaProcessor!=NULL;
}

void PrivateStorage::stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza)
{
	if (FSaveRequests.contains(AStanza.id()))
	{
		QDomElement elem = FSaveRequests.take(AStanza.id());
		if (AStanza.type() == "result")
			emit dataSaved(AStanza.id(),AStreamJid,elem);
		else
			removeElement(AStreamJid,elem.tagName(),elem.namespaceURI());
	}
	else if (FLoadRequests.contains(AStanza.id()))
	{
		FLoadRequests.remove(AStanza.id());
		if (AStanza.type() == "result")
		{
			QDomElement dataElem = AStanza.firstElement("query",NS_JABBER_PRIVATE).firstChildElement();
			emit dataLoaded(AStanza.id(),AStreamJid,insertElement(AStreamJid,dataElem));
		}
	}
	else if (FRemoveRequests.contains(AStanza.id()))
	{
		QDomElement dataElem = FRemoveRequests.take(AStanza.id());
		if (AStanza.type() == "result")
		{
			emit dataRemoved(AStanza.id(),AStreamJid,dataElem);
			removeElement(AStreamJid,dataElem.tagName(),dataElem.namespaceURI());
		}
	}

	if (AStanza.type() == "error")
	{
		ErrorHandler err(AStanza.element());
		emit dataError(AStanza.id(),err.message());
	}
}

void PrivateStorage::stanzaRequestTimeout(const Jid &AStreamJid, const QString &AStanzaId)
{
	if (FSaveRequests.contains(AStanzaId))
	{
		QDomElement elem = FSaveRequests.take(AStanzaId);
		removeElement(AStreamJid,elem.tagName(),elem.namespaceURI());
	}
	else if (FLoadRequests.contains(AStanzaId))
	{
		FLoadRequests.remove(AStanzaId);
	}
	else if (FRemoveRequests.contains(AStanzaId))
	{
		FRemoveRequests.remove(AStanzaId);
	}
	ErrorHandler err(ErrorHandler::REQUEST_TIMEOUT);
	emit dataError(AStanzaId,err.message());
}

bool PrivateStorage::isOpen(const Jid &AStreamJid) const
{
	return FStreamElements.contains(AStreamJid);
}

bool PrivateStorage::isLoaded(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace) const
{
	return !getData(AStreamJid,ATagName,ANamespace).isNull();
}

QDomElement PrivateStorage::getData(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace) const
{
	QDomElement elem = FStreamElements.value(AStreamJid).firstChildElement(ATagName);
	while (!elem.isNull() && elem.namespaceURI()!=ANamespace)
		elem= elem.nextSiblingElement(ATagName);
	return elem;
}

QString PrivateStorage::saveData(const Jid &AStreamJid, const QDomElement &AElement)
{
	if (isOpen(AStreamJid) && !AElement.namespaceURI().isEmpty())
	{
		Stanza stanza("iq");
		stanza.setType("set").setId(FStanzaProcessor->newId());
		QDomElement elem = stanza.addElement("query",NS_JABBER_PRIVATE);
		elem.appendChild(AElement.cloneNode(true));
		if (FStanzaProcessor && FStanzaProcessor->sendStanzaRequest(this,AStreamJid,stanza,PRIVATE_STORAGE_TIMEOUT))
		{
			FSaveRequests.insert(stanza.id(),insertElement(AStreamJid,AElement));
			return stanza.id();
		}
	}
	return QString::null;
}

QString PrivateStorage::loadData(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace)
{
	if (isOpen(AStreamJid) && !ATagName.isEmpty() && !ANamespace.isEmpty())
	{
		Stanza stanza("iq");
		stanza.setType("get").setId(FStanzaProcessor->newId());
		QDomElement elem = stanza.addElement("query",NS_JABBER_PRIVATE);
		QDomElement dataElem = elem.appendChild(stanza.createElement(ATagName,ANamespace)).toElement();
		if (FStanzaProcessor && FStanzaProcessor->sendStanzaRequest(this,AStreamJid,stanza,PRIVATE_STORAGE_TIMEOUT))
		{
			FLoadRequests.insert(stanza.id(),dataElem);
			return stanza.id();
		}
	}
	return QString::null;
}

QString PrivateStorage::removeData(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace)
{
	if (isOpen(AStreamJid) && !ATagName.isEmpty() && !ANamespace.isEmpty())
	{
		Stanza stanza("iq");
		stanza.setType("set").setId(FStanzaProcessor->newId());
		QDomElement elem = stanza.addElement("query",NS_JABBER_PRIVATE);
		elem = elem.appendChild(stanza.createElement(ATagName,ANamespace)).toElement();
		if (FStanzaProcessor && FStanzaProcessor->sendStanzaRequest(this,AStreamJid,stanza,PRIVATE_STORAGE_TIMEOUT))
		{
			QDomElement dataElem = getData(AStreamJid,ATagName,ANamespace);
			if (dataElem.isNull())
				dataElem = insertElement(AStreamJid,elem);
			FRemoveRequests.insert(stanza.id(),dataElem);
			return stanza.id();
		}
	}
	return QString::null;
}

QDomElement PrivateStorage::insertElement(const Jid &AStreamJid, const QDomElement &AElement)
{
	removeElement(AStreamJid,AElement.tagName(),AElement.namespaceURI());
	QDomElement streamElem = FStreamElements.value(AStreamJid);
	return streamElem.appendChild(AElement.cloneNode(true)).toElement();
}

void PrivateStorage::removeElement(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace)
{
	if (FStreamElements.contains(AStreamJid))
		FStreamElements[AStreamJid].removeChild(getData(AStreamJid,ATagName,ANamespace));
}

void PrivateStorage::onStreamOpened(IXmppStream *AXmppStream)
{
	FStreamElements.insert(AXmppStream->streamJid(),FStorage.appendChild(FStorage.createElement("stream")).toElement());
	emit storageOpened(AXmppStream->streamJid());
}

void PrivateStorage::onStreamAboutToClose(IXmppStream *AXmppStream)
{
	emit storageAboutToClose(AXmppStream->streamJid());
}

void PrivateStorage::onStreamClosed(IXmppStream *AXmppStream)
{
	emit storageClosed(AXmppStream->streamJid());
	FStorage.removeChild(FStreamElements.take(AXmppStream->streamJid()));
}

Q_EXPORT_PLUGIN2(plg_privatestorage, PrivateStorage)
