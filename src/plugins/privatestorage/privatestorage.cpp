#include "privatestorage.h"

#define PRIVATE_STORAGE_TIMEOUT       30000

#define SHC_NOTIFYDATACHANGED "/message/x[@xmlns='" NS_VACUUM_PRIVATESTORAGE_UPDATE "']"

PrivateStorage::PrivateStorage()
{
	FPresencePlugin = NULL;
	FStanzaProcessor = NULL;

	FSHINotifyDataChanged = -1;
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

	plugin = APluginManager->pluginInterface("IPresencePlugin").value(0,NULL);
	if (plugin)
	{
		FPresencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance());
		if (FPresencePlugin)
		{
			connect(FPresencePlugin->instance(),SIGNAL(presenceAboutToClose(IPresence *, int, const QString &)),
				SLOT(onPresenceAboutToClose(IPresence *, int, const QString &)));
		}
	}

	return FStanzaProcessor!=NULL;
}

bool PrivateStorage::initObjects()
{
	if (FStanzaProcessor)
	{
		IStanzaHandle handle;
		handle.handler = this;
		handle.order = SHO_MI_PRIVATESTORAGE;
		handle.conditions.append(SHC_NOTIFYDATACHANGED);
		handle.direction = IStanzaHandle::DirectionIn;
		FSHINotifyDataChanged = FStanzaProcessor->insertStanzaHandle(handle);
	}
	return true;
}

bool PrivateStorage::stanzaReadWrite(int AHandleId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept)
{
	if (AHandleId == FSHINotifyDataChanged)
	{
		AAccept = true;
		QDomElement dataElem = AStanza.firstElement("x",NS_VACUUM_PRIVATESTORAGE_UPDATE).firstChildElement();
		while (!dataElem.isNull())
		{
			emit dataChanged(AStreamJid,dataElem.tagName(),dataElem.namespaceURI());
			dataElem = dataElem.nextSiblingElement();
		}
		return true;
	}
	return false;
}

void PrivateStorage::stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza)
{
	if (FSaveRequests.contains(AStanza.id()))
	{
		QDomElement dataElem = FSaveRequests.take(AStanza.id());
		if (AStanza.type() == "result")
			notifyDataChanged(AStreamJid,dataElem.tagName(),dataElem.namespaceURI());
		saveOptionsElement(AStreamJid,dataElem);
		emit dataSaved(AStanza.id(),AStreamJid,dataElem);
	}
	else if (FLoadRequests.contains(AStanza.id()))
	{
		QDomElement dataElem; 
		QDomElement loadElem = FLoadRequests.take(AStanza.id());
		if (AStanza.type() == "result")
			dataElem = AStanza.firstElement("query",NS_JABBER_PRIVATE).firstChildElement(loadElem.tagName());
		if (dataElem.isNull())
			dataElem = loadOptionsElement(AStreamJid,loadElem.tagName(),loadElem.namespaceURI());
		emit dataLoaded(AStanza.id(),AStreamJid,insertElement(AStreamJid,dataElem));
	}
	else if (FRemoveRequests.contains(AStanza.id()))
	{
		QDomElement dataElem = FRemoveRequests.take(AStanza.id());
		if (AStanza.type() == "result")
			notifyDataChanged(AStreamJid,dataElem.tagName(),dataElem.namespaceURI());
		removeElement(AStreamJid,dataElem.tagName(),dataElem.namespaceURI());
		removeOptionsElement(AStreamJid,dataElem.tagName(),dataElem.namespaceURI());
		emit dataRemoved(AStanza.id(),AStreamJid,dataElem);
	}
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
	if (FStanzaProcessor && isOpen(AStreamJid) && !AElement.namespaceURI().isEmpty())
	{
		Stanza stanza("iq");
		stanza.setType("set").setId(FStanzaProcessor->newId());
		QDomElement elem = stanza.addElement("query",NS_JABBER_PRIVATE);
		elem.appendChild(AElement.cloneNode(true));
		if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,stanza,PRIVATE_STORAGE_TIMEOUT))
		{
			if (FPreClosedStreams.contains(AStreamJid))
				notifyDataChanged(AStreamJid,AElement.tagName(),AElement.namespaceURI());
			FSaveRequests.insert(stanza.id(),insertElement(AStreamJid,AElement));
			return stanza.id();
		}
	}
	return QString::null;
}

QString PrivateStorage::loadData(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace)
{
	if (FStanzaProcessor && isOpen(AStreamJid) && !ATagName.isEmpty() && !ANamespace.isEmpty())
	{
		Stanza stanza("iq");
		stanza.setType("get").setId(FStanzaProcessor->newId());
		QDomElement elem = stanza.addElement("query",NS_JABBER_PRIVATE);
		QDomElement dataElem = elem.appendChild(stanza.createElement(ATagName,ANamespace)).toElement();
		if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,stanza,PRIVATE_STORAGE_TIMEOUT))
		{
			FLoadRequests.insert(stanza.id(),dataElem);
			return stanza.id();
		}
	}
	return QString::null;
}

QString PrivateStorage::removeData(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace)
{
	if (FStanzaProcessor && isOpen(AStreamJid) && !ATagName.isEmpty() && !ANamespace.isEmpty())
	{
		Stanza stanza("iq");
		stanza.setType("set").setId(FStanzaProcessor->newId());
		QDomElement elem = stanza.addElement("query",NS_JABBER_PRIVATE);
		elem = elem.appendChild(stanza.createElement(ATagName,ANamespace)).toElement();
		if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,stanza,PRIVATE_STORAGE_TIMEOUT))
		{
			QDomElement dataElem = getData(AStreamJid,ATagName,ANamespace);
			if (dataElem.isNull())
				dataElem = insertElement(AStreamJid,elem);
			if (FPreClosedStreams.contains(AStreamJid))
				notifyDataChanged(AStreamJid,ATagName,ANamespace);
			FRemoveRequests.insert(stanza.id(),dataElem);
			return stanza.id();
		}
	}
	return QString::null;
}

void PrivateStorage::notifyDataChanged(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace)
{
	IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->findPresence(AStreamJid) : NULL;
	if (FStanzaProcessor && presence && presence->isOpen())
	{
		foreach(const IPresenceItem &item, presence->findItems(AStreamJid.bare()))
		{
			if (item.itemJid != AStreamJid)
			{
				Stanza notify("message");
				notify.setTo(item.itemJid.full());
				QDomElement xElem = notify.addElement("x",NS_VACUUM_PRIVATESTORAGE_UPDATE);
				xElem.appendChild(notify.createElement(ATagName,ANamespace));
				FStanzaProcessor->sendStanzaOut(AStreamJid,notify);
			}
		}
	}
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

void PrivateStorage::saveOptionsElement(const Jid &AStreamJid, const QDomElement &AElement) const
{
	if (AStreamJid.isValid() && !AElement.tagName().isEmpty() && !AElement.namespaceURI().isEmpty())
	{
		QDomDocument doc;
		doc.appendChild(doc.createElement("storage")).appendChild(AElement.cloneNode(true));
		QString nodePath = QString("private-storage[%1].%2[%3]").arg(AStreamJid.pBare()).arg(AElement.tagName()).arg(AElement.namespaceURI());
		Options::setFileValue(Options::encrypt(doc.toByteArray(0)),nodePath);
	}
}

QDomElement PrivateStorage::loadOptionsElement(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace) const
{
	QDomDocument doc;
	if (AStreamJid.isValid() && !ATagName.isEmpty() && !ANamespace.isEmpty())
	{
		QString nodePath = QString("private-storage[%1].%2[%3]").arg(AStreamJid.pBare()).arg(ATagName).arg(ANamespace);
		doc.setContent(Options::decrypt(Options::fileValue(nodePath).toByteArray()).toByteArray(),true);
		QDomElement dataElem = doc.documentElement().firstChildElement();
		if (dataElem.tagName()!=ATagName || dataElem.namespaceURI()!=ANamespace)
		{
			doc.clear();
			doc.appendChild(doc.createElement("storage")).appendChild(doc.createElementNS(ANamespace,ATagName));
		}
	}
	return doc.documentElement().firstChildElement();
}

void PrivateStorage::removeOptionsElement(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace) const
{
	if (AStreamJid.isValid() && !ATagName.isEmpty() && !ANamespace.isEmpty())
	{
		QString nodePath = QString("private-storage[%1].%2[%3]").arg(AStreamJid.pBare()).arg(ATagName).arg(ANamespace);
		Options::setFileValue(QVariant(),nodePath);
	}
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
	FPreClosedStreams -= AXmppStream->streamJid();
	emit storageClosed(AXmppStream->streamJid());
	FStorage.removeChild(FStreamElements.take(AXmppStream->streamJid()));
}

void PrivateStorage::onPresenceAboutToClose(IPresence *APresence, int AShow, const QString &AStatus)
{
	Q_UNUSED(AShow); Q_UNUSED(AStatus);
	FPreClosedStreams += APresence->streamJid();
	emit storageNotifyAboutToClose(APresence->streamJid());
}

Q_EXPORT_PLUGIN2(plg_privatestorage, PrivateStorage)
