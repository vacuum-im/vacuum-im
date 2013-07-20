#include "servermessagearchive.h"

#include <QDomElement>

#define RESULTSET_MAX         30
#define ARCHIVE_TIMEOUT       30000

ServerMessageArchive::ServerMessageArchive()
{
	FArchiver = NULL;
	FStanzaProcessor = NULL;
}

ServerMessageArchive::~ServerMessageArchive()
{

}

void ServerMessageArchive::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Server Message Archive");
	APluginInfo->description = tr("Allows to save the history of communications on the server");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(MESSAGEARCHIVER_UUID);
	APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool ServerMessageArchive::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
	IPlugin *plugin = APluginManager->pluginInterface("IMessageArchiver").value(0,NULL);
	if (plugin)
	{
		FArchiver = qobject_cast<IMessageArchiver *>(plugin->instance());
		if (FArchiver)
		{
			connect(FArchiver->instance(),SIGNAL(archivePrefsOpened(const Jid &)),SLOT(onArchivePrefsOpened(const Jid &)));
			connect(FArchiver->instance(),SIGNAL(archivePrefsClosed(const Jid &)),SLOT(onArchivePrefsClosed(const Jid &)));
		}
	}

	plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0,NULL);
	if (plugin)
	{
		FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());
	}

	connect(this,SIGNAL(serverHeadersLoaded(const QString &, const QList<IArchiveHeader> &, const IArchiveResultSet &)),
		SLOT(onServerHeadersLoaded(const QString &, const QList<IArchiveHeader> &, const IArchiveResultSet &)));
	connect(this,SIGNAL(serverCollectionLoaded(const QString &, const IArchiveCollection &, const IArchiveResultSet &)),
		SLOT(onServerCollectionLoaded(const QString &, const IArchiveCollection &, const IArchiveResultSet &)));
	connect(this,SIGNAL(serverModificationsLoaded(const QString &, const IArchiveModifications &, const IArchiveResultSet &)),
		SLOT(onServerModificationsLoaded(const QString &, const IArchiveModifications &, const IArchiveResultSet &)));
	connect(this,SIGNAL(requestFailed(const QString &, const XmppError &)),SLOT(onServerRequestFailed(const QString &, const XmppError &)));

	return FArchiver!=NULL && FStanzaProcessor!=NULL;
}

bool ServerMessageArchive::initObjects()
{
	if (FArchiver)
	{
		FArchiver->registerArchiveEngine(this);
	}
	return true;
}

void ServerMessageArchive::stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza)
{
	Q_UNUSED(AStreamJid);
	if (FSaveRequests.contains(AStanza.id()))
	{
		IArchiveHeader header = FSaveRequests.take(AStanza.id());
		if (AStanza.type() == "result")
		{
			QDomElement chatElem = AStanza.firstElement("save").firstChildElement("chat");
			header.engineId = engineId();
			header.subject = chatElem.attribute("subject",header.subject);
			header.threadId = chatElem.attribute("thread",header.threadId);
			header.version = chatElem.attribute("version",QString::number(header.version)).toInt();
			emit collectionSaved(AStanza.id(),header);
		}
	}
	else if (FServerHeadersRequests.contains(AStanza.id()))
	{
		IArchiveRequest request = FServerHeadersRequests.take(AStanza.id());
		if (AStanza.type() == "result")
		{
			QList<IArchiveHeader> headers;
			QDomElement listElem = AStanza.firstElement("list");
			QDomElement chatElem = listElem.firstChildElement("chat");
			while (!chatElem.isNull())
			{
				IArchiveHeader header;
				header.engineId = engineId();
				header.with = chatElem.attribute("with");
				header.start = DateTime(chatElem.attribute("start")).toLocal();
				header.subject = chatElem.attribute("subject");
				header.threadId = chatElem.attribute("thread");
				header.version = chatElem.attribute("version").toInt();
				headers.append(header);
				chatElem = chatElem.nextSiblingElement("chat");
			}

			if (request.order == Qt::AscendingOrder)
				qSort(headers.begin(),headers.end(),qLess<IArchiveHeader>());
			else
				qSort(headers.begin(),headers.end(),qGreater<IArchiveHeader>());

			if (request.maxItems>0 && headers.count()>request.maxItems)
				headers = headers.mid(0,request.maxItems);

			IArchiveResultSet resultSet = readResultSetAnswer(listElem);
			emit serverHeadersLoaded(AStanza.id(),headers,resultSet);
		}
	}
	else if (FServerCollectionRequests.contains(AStanza.id()))
	{
		if (AStanza.type() == "result")
		{
			IArchiveCollection collection;
			QDomElement chatElem = AStanza.firstElement("chat");
			FArchiver->elementToCollection(chatElem,collection);
			collection.header.engineId = engineId();
			IArchiveResultSet resultSet = readResultSetAnswer(chatElem);
			emit serverCollectionLoaded(AStanza.id(),collection,resultSet);
		}
		FServerCollectionRequests.remove(AStanza.id());
	}
	else if (FRemoveRequests.contains(AStanza.id()))
	{
		IArchiveRequest request = FRemoveRequests.take(AStanza.id());
		if (AStanza.type() == "result")
		{
			emit collectionsRemoved(AStanza.id(),request);
		}
	}
	else if (FServerModificationsRequests.contains(AStanza.id()))
	{
		QDateTime startTime = FServerModificationsRequests.take(AStanza.id());
		if (AStanza.type() == "result")
		{
			IArchiveModifications modifs;
			modifs.startTime = startTime;

			QDomElement modifsElem = AStanza.firstElement("modified");
			QDomElement changeElem = modifsElem.firstChildElement();
			while (!changeElem.isNull())
			{
				IArchiveModification modif;
				modif.header.engineId = engineId();
				modif.header.with = changeElem.attribute("with");
				modif.header.start = DateTime(changeElem.attribute("start")).toLocal();
				modif.header.version = changeElem.attribute("version").toInt();
				if (changeElem.tagName() == "changed")
				{
					modif.action =  modif.header.version==0 ? IArchiveModification::Created : IArchiveModification::Modified;
					modifs.items.append(modif);
					modifs.endTime = modif.header.start;
				}
				else if (changeElem.tagName() == "removed")
				{
					modif.action = IArchiveModification::Removed;
					modifs.items.append(modif);
					modifs.endTime = modif.header.start;
				}
				changeElem = changeElem.nextSiblingElement();
			}
			IArchiveResultSet resultSet = readResultSetAnswer(modifsElem);
			emit serverModificationsLoaded(AStanza.id(),modifs,resultSet);
		}
	}

	if (AStanza.type() == "error")
		emit requestFailed(AStanza.id(),XmppStanzaError(AStanza));
}

QUuid ServerMessageArchive::engineId() const
{
	return SERVERMESSAGEARCHIVE_UUID;
}

QString ServerMessageArchive::engineName() const
{
	return tr("Standard Server Side Archive");
}

QString ServerMessageArchive::engineDescription() const
{
	return tr("History of conversations is stored on your jabber server");
}

IOptionsWidget *ServerMessageArchive::engineSettingsWidget(QWidget *AParent)
{
	Q_UNUSED(AParent);
	return NULL;
}

quint32 ServerMessageArchive::capabilities(const Jid &AStreamJid) const
{
	quint32 caps = 0;
	if (FArchiver->isReady(AStreamJid))
	{
		if (FArchiver->isSupported(AStreamJid,NS_ARCHIVE_AUTO))
			caps |= AutomaticArchiving;
		if (FArchiver->isSupported(AStreamJid,NS_ARCHIVE_MANAGE))
			caps |= ArchiveManagement;
		if (FArchiver->isSupported(AStreamJid,NS_ARCHIVE_MANUAL))
			caps |= ManualArchiving;
		if ((caps & ArchiveManagement)>0 && (caps & ManualArchiving)>0)
			caps |= Replication;
	}
	return caps;
}

bool ServerMessageArchive::isCapable(const Jid &AStreamJid, quint32 ACapability) const
{
	return (capabilities(AStreamJid) & ACapability) == ACapability;
}

int ServerMessageArchive::capabilityOrder(quint32 ACapability, const Jid &AStreamJid) const
{
	if (isCapable(AStreamJid,ACapability))
	{
		switch (ACapability)
		{
		case ManualArchiving:
			return ACO_MANUAL_SERVERARCHIVE;
		case AutomaticArchiving:
			return ACO_AUTOMATIC_SERVERARCHIVE;
		case ArchiveManagement:
			return ACO_MANAGE_SERVERARCHIVE;
		case Replication:
			return ACO_REPLICATION_SERVERARCHIVE;
		default:
			break;
		}
	}
	return -1;
}

bool ServerMessageArchive::saveMessage(const Jid &AStreamJid, const Message &AMessage, bool ADirectionIn)
{
	Q_UNUSED(AStreamJid);
	Q_UNUSED(AMessage);
	Q_UNUSED(ADirectionIn);
	return false;
}

bool ServerMessageArchive::saveNote(const Jid &AStreamJid, const Message &AMessage, bool ADirectionIn)
{
	Q_UNUSED(AStreamJid);
	Q_UNUSED(AMessage);
	Q_UNUSED(ADirectionIn);
	return false;
}

QString ServerMessageArchive::saveCollection(const Jid &AStreamJid, const IArchiveCollection &ACollection)
{
	if (FStanzaProcessor && isCapable(AStreamJid,ManualArchiving) && ACollection.header.with.isValid() && ACollection.header.start.isValid())
	{
		Stanza save("iq");
		save.setType("set").setId(FStanzaProcessor->newId());

		QDomElement chatElem = save.addElement("save",FNamespaces.value(AStreamJid)).appendChild(save.createElement("chat")).toElement();
		FArchiver->collectionToElement(ACollection, chatElem, FArchiver->archiveItemPrefs(AStreamJid,ACollection.header.with).save);

		if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,save,ARCHIVE_TIMEOUT))
		{
			FSaveRequests.insert(save.id(),ACollection.header);
			return save.id();
		}
	}
	return QString::null;
}

QString ServerMessageArchive::loadHeaders(const Jid &AStreamJid, const IArchiveRequest &ARequest)
{
	QString id = loadServerHeaders(AStreamJid,ARequest);
	if (!id.isEmpty())
	{
		HeadersRequest request;
		request.id = QUuid::createUuid().toString();
		request.streamJid = AStreamJid;
		request.request = ARequest;
		FHeadersRequests.insert(id,request);
		return request.id;
	}
	return QString::null;
}

QString ServerMessageArchive::loadCollection(const Jid &AStreamJid, const IArchiveHeader &AHeader)
{
	QString id = loadServerCollection(AStreamJid,AHeader);
	if (!id.isEmpty())
	{
		CollectionRequest request;
		request.id = QUuid::createUuid().toString();
		request.streamJid = AStreamJid;
		request.header = AHeader;
		FCollectionRequests.insert(id,request);
		return request.id;
	}
	return QString::null;
}

QString ServerMessageArchive::removeCollections(const Jid &AStreamJid, const IArchiveRequest &ARequest)
{
	if (FStanzaProcessor && isCapable(AStreamJid,ArchiveManagement))
	{
		Stanza remove("iq");
		remove.setType("set").setId(FStanzaProcessor->newId());

		QDomElement removeElem = remove.addElement("remove",FNamespaces.value(AStreamJid));
		if (ARequest.with.isValid())
			removeElem.setAttribute("with",ARequest.with.full());
		if (ARequest.with.isValid() && ARequest.exactmatch)
			removeElem.setAttribute("exactmatch",QVariant(ARequest.exactmatch).toString());
		if (ARequest.start.isValid())
			removeElem.setAttribute("start",DateTime(ARequest.start).toX85UTC());
		if (ARequest.end.isValid())
			removeElem.setAttribute("end",DateTime(ARequest.end).toX85UTC());
		if (ARequest.opened)
			removeElem.setAttribute("open",QVariant(ARequest.opened).toString());

		if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,remove,ARCHIVE_TIMEOUT))
		{
			FRemoveRequests.insert(remove.id(),ARequest);
			return remove.id();
		}
	}
	return QString::null;
}

QString ServerMessageArchive::loadModifications(const Jid &AStreamJid, const QDateTime &AStart, int ACount)
{
	QString id = loadServerModifications(AStreamJid,AStart,ACount);
	if (!id.isEmpty())
	{
		ModificationsRequest request;
		request.id = QUuid::createUuid().toString();
		request.streamJid = AStreamJid;
		request.start = AStart;
		request.count = ACount;
		FModificationsRequests.insert(id,request);
		return request.id;
	}
	return QString::null;
}

QString ServerMessageArchive::loadServerHeaders(const Jid &AStreamJid, const IArchiveRequest &ARequest, const IArchiveResultSet &AResult)
{
	if (FStanzaProcessor && isCapable(AStreamJid,ArchiveManagement))
	{
		Stanza request("iq");
		request.setType("get").setId(FStanzaProcessor->newId());

		QDomElement listElem = request.addElement("list",FNamespaces.value(AStreamJid));
		if (ARequest.with.isValid())
			listElem.setAttribute("with",ARequest.with.full());
		if (ARequest.with.isValid() && ARequest.exactmatch)
			listElem.setAttribute("exactmatch",QVariant(ARequest.exactmatch).toString());
		if (ARequest.start.isValid())
			listElem.setAttribute("start",DateTime(ARequest.start).toX85UTC());
		if (ARequest.end.isValid())
			listElem.setAttribute("end",DateTime(ARequest.end).toX85UTC());
		insertResultSetRequest(listElem,AResult,ARequest.order,ARequest.maxItems);
		
		if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,request,ARCHIVE_TIMEOUT))
		{
			FServerHeadersRequests.insert(request.id(),ARequest);
			return request.id();
		}
	}
	return QString::null;
}

QString ServerMessageArchive::loadServerCollection(const Jid &AStreamJid, const IArchiveHeader &AHeader, const IArchiveResultSet &AResult)
{
	if (FStanzaProcessor && isCapable(AStreamJid,ArchiveManagement) && AHeader.with.isValid() && AHeader.start.isValid())
	{
		Stanza retrieve("iq");
		retrieve.setType("get").setId(FStanzaProcessor->newId());

		QDomElement retrieveElem = retrieve.addElement("retrieve",FNamespaces.value(AStreamJid));
		retrieveElem.setAttribute("with",AHeader.with.full());
		retrieveElem.setAttribute("start",DateTime(AHeader.start).toX85UTC());
		insertResultSetRequest(retrieveElem,AResult,Qt::AscendingOrder);

		if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,retrieve,ARCHIVE_TIMEOUT))
		{
			FServerCollectionRequests.insert(retrieve.id(),AHeader);
			return retrieve.id();
		}
	}
	return QString::null;
}

QString ServerMessageArchive::loadServerModifications(const Jid &AStreamJid, const QDateTime &AStart, int ACount, const IArchiveResultSet &AResult)
{
	if (FStanzaProcessor && isCapable(AStreamJid,ArchiveManagement) && AStart.isValid() && ACount>0)
	{
		Stanza modify("iq");
		modify.setType("get").setId(FStanzaProcessor->newId());

		QDomElement modifyElem = modify.addElement("modified",FNamespaces.value(AStreamJid));
		modifyElem.setAttribute("start",DateTime(AStart).toX85UTC());
		insertResultSetRequest(modifyElem,AResult,Qt::AscendingOrder,ACount);
		
		if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,modify,ARCHIVE_TIMEOUT))
		{
			FServerModificationsRequests.insert(modify.id(),AStart);
			return modify.id();
		}
	}
	return QString::null;
}

IArchiveResultSet ServerMessageArchive::readResultSetAnswer(const QDomElement &AElem) const
{
	QDomElement setElem = AElem.firstChildElement("set");
	while (!setElem.isNull() && setElem.namespaceURI()!=NS_RESULTSET)
		setElem = setElem.nextSiblingElement("set");

	IArchiveResultSet resultSet;
	resultSet.count = setElem.firstChildElement("count").text().toInt();
	resultSet.index = setElem.firstChildElement("first").attribute("index").toInt();
	resultSet.first = setElem.firstChildElement("first").text();
	resultSet.last = setElem.firstChildElement("last").text();

	return resultSet;
}

void ServerMessageArchive::insertResultSetRequest(QDomElement &AElem, const IArchiveResultSet &AResult, Qt::SortOrder AOrder, int AMax) const
{
	QDomElement setElem = AElem.appendChild(AElem.ownerDocument().createElementNS(NS_RESULTSET,"set")).toElement();
	setElem.appendChild(AElem.ownerDocument().createElement("max")).appendChild(AElem.ownerDocument().createTextNode(QString::number(AMax>0 ? qMin(AMax,RESULTSET_MAX) : RESULTSET_MAX)));
	if (AOrder==Qt::AscendingOrder && !AResult.last.isEmpty())
		setElem.appendChild(AElem.ownerDocument().createElement("after")).appendChild(AElem.ownerDocument().createTextNode(AResult.last));
	else if (AOrder==Qt::DescendingOrder && !AResult.first.isEmpty())
		setElem.appendChild(AElem.ownerDocument().createElement("before")).appendChild(AElem.ownerDocument().createTextNode(AResult.first));
	else if (AOrder == Qt::DescendingOrder)
		setElem.appendChild(AElem.ownerDocument().createElement("before"));
}

void ServerMessageArchive::onArchivePrefsOpened(const Jid &AStreamJid)
{
	FNamespaces.insert(AStreamJid,FArchiver->prefsNamespace(AStreamJid));
	emit capabilitiesChanged(AStreamJid);
}

void ServerMessageArchive::onArchivePrefsClosed(const Jid &AStreamJid)
{
	FNamespaces.remove(AStreamJid);
	emit capabilitiesChanged(AStreamJid);
}

void ServerMessageArchive::onServerRequestFailed(const QString &AId, const XmppError &AError)
{
	if (FHeadersRequests.contains(AId))
	{
		HeadersRequest request = FHeadersRequests.take(AId);
		emit requestFailed(request.id,AError);
	}
	else if (FCollectionRequests.contains(AId))
	{
		CollectionRequest request = FCollectionRequests.take(AId);
		emit requestFailed(request.id,AError);
	}
}

void ServerMessageArchive::onServerHeadersLoaded(const QString &AId, const QList<IArchiveHeader> &AHeaders, const IArchiveResultSet &AResult)
{
	if (FHeadersRequests.contains(AId))
	{
		HeadersRequest request = FHeadersRequests.take(AId);
		request.headers += AHeaders;
		if (!AResult.last.isEmpty() && AResult.index+AHeaders.count()<AResult.count && (request.request.maxItems<=0 || request.headers.count()<request.request.maxItems))
		{
			QString id = loadServerHeaders(request.streamJid,request.request,AResult);
			if (!id.isEmpty())
				FHeadersRequests.insert(id,request);
			else
				emit requestFailed(request.id,XmppError(IERR_HISTORY_HEADERS_LOAD_ERROR));
		}
		else
		{
			if (request.request.maxItems>0 && request.headers.count()>request.request.maxItems)
				request.headers = request.headers.mid(0,request.request.maxItems);
			emit headersLoaded(request.id,request.headers);
		}
	}
}

void ServerMessageArchive::onServerCollectionLoaded(const QString &AId, const IArchiveCollection &ACollection, const IArchiveResultSet &AResult)
{
	if (FCollectionRequests.contains(AId))
	{
		CollectionRequest request = FCollectionRequests.take(AId);
		request.collection.header = ACollection.header;
		request.collection.body.messages += ACollection.body.messages;
		request.collection.body.notes += ACollection.body.notes;
		if (!AResult.last.isEmpty() && AResult.index+ACollection.body.messages.count()+ACollection.body.notes.count()<AResult.count)
		{
			QString id = loadServerCollection(request.streamJid,request.header,AResult);
			if (!id.isEmpty())
				FCollectionRequests.insert(id,request);
			else
				emit requestFailed(request.id,XmppError(IERR_HISTORY_CONVERSATION_LOAD_ERROR));
		}
		else
		{
			emit collectionLoaded(request.id,request.collection);
		}
	}
}

void ServerMessageArchive::onServerModificationsLoaded(const QString &AId, const IArchiveModifications &AModifs, const IArchiveResultSet &AResult)
{
	if (FModificationsRequests.contains(AId))
	{
		ModificationsRequest request = FModificationsRequests.take(AId);
		request.modifications.startTime = !request.modifications.startTime.isValid() ? AModifs.startTime : request.modifications.startTime;
		request.modifications.endTime = AModifs.endTime;
		request.modifications.items += AModifs.items;
		if (!AResult.last.isEmpty() && AResult.index+AModifs.items.count()<AResult.count && AModifs.items.count()<request.count)
		{
			QString id = loadServerModifications(request.streamJid,request.start,request.count-request.modifications.items.count(),AResult);
			if (!id.isEmpty())
				FModificationsRequests.insert(id,request);
			else
				emit requestFailed(request.id,XmppError(IERR_HISTORY_MODIFICATIONS_LOAD_ERROR));
		}
		else
		{
			if (request.modifications.items.count() > request.count)
			{
				request.modifications.items = request.modifications.items.mid(0,request.count);
				request.modifications.endTime = request.modifications.items.last().header.start;
			}
			emit modificationsLoaded(request.id,request.modifications);
		}
	}
}
