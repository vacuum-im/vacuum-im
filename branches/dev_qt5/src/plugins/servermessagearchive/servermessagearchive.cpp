#include "servermessagearchive.h"

#include <QTextStream>
#include <QDomElement>
#include <definitions/namespaces.h>
#include <definitions/optionvalues.h>
#include <definitions/internalerrors.h>
#include <definitions/archivecapabilityorders.h>
#include <utils/options.h>

#define MAX_HEADER_ITEMS          50
#define MAX_MESSAGE_ITEMS         25
#define MAX_MODIFICATION_ITEMS    50

#define ARCHIVE_REQUEST_TIMEOUT   30000

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

	connect(this,SIGNAL(serverHeadersLoaded(const QString &, const QList<IArchiveHeader> &, const QString &)),
		SLOT(onServerHeadersLoaded(const QString &, const QList<IArchiveHeader> &, const QString &)));
	connect(this,SIGNAL(serverCollectionSaved(const QString &, const IArchiveCollection &, const QString &)),
		SLOT(onServerCollectionSaved(const QString &, const IArchiveCollection &, const QString &)));
	connect(this,SIGNAL(serverCollectionLoaded(const QString &, const IArchiveCollection &, const QString &)),
		SLOT(onServerCollectionLoaded(const QString &, const IArchiveCollection &, const QString &)));
	connect(this,SIGNAL(serverModificationsLoaded(const QString &, const IArchiveModifications &, const QString &)),
		SLOT(onServerModificationsLoaded(const QString &, const IArchiveModifications &, const QString &)));
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

bool ServerMessageArchive::initSettings()
{
	Options::setDefaultValue(OPV_SERVERARCHIVE_MAXUPLOADSIZE,4096);
	return true;
}

void ServerMessageArchive::stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza)
{
	Q_UNUSED(AStreamJid);
	if (FServerLoadHeadersRequests.contains(AStanza.id()))
	{
		IArchiveRequest request = FServerLoadHeadersRequests.take(AStanza.id());
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

			if ((quint32)headers.count() > request.maxItems)
				headers = headers.mid(0,request.maxItems);

			QString nextRef = getNextRef(readResultSetAnswer(listElem),headers.count(),MAX_HEADER_ITEMS,request.maxItems,request.order);
			emit serverHeadersLoaded(AStanza.id(),headers,nextRef);
		}
		else
		{
			emit requestFailed(AStanza.id(),XmppStanzaError(AStanza));
		}
	}
	else if (FServerSaveCollectionRequests.contains(AStanza.id()))
	{
		ServerCollectionRequest request = FServerSaveCollectionRequests.take(AStanza.id());
		if (AStanza.type() == "result")
		{
			QDomElement chatElem = AStanza.firstElement("save").firstChildElement("chat");
			request.collection.header.engineId = engineId();
			request.collection.header.subject = chatElem.attribute("subject",request.collection.header.subject);
			request.collection.header.threadId = chatElem.attribute("thread",request.collection.header.threadId);
			request.collection.header.version = chatElem.attribute("version",QString::number(request.collection.header.version)).toInt();
			emit serverCollectionSaved(AStanza.id(),request.collection,request.nextRef);
		}
		else
		{
			emit requestFailed(AStanza.id(),XmppStanzaError(AStanza));
		}
	}
	else if (FServerLoadCollectionRequests.contains(AStanza.id()))
	{
		FServerLoadCollectionRequests.remove(AStanza.id());
		if (AStanza.type() == "result")
		{
			IArchiveCollection collection;
			QDomElement chatElem = AStanza.firstElement("chat");
			FArchiver->elementToCollection(chatElem,collection);
			collection.header.engineId = engineId();

			QString nextRef = getNextRef(readResultSetAnswer(chatElem),collection.body.messages.count()+collection.body.notes.count(),MAX_MESSAGE_ITEMS);
			emit serverCollectionLoaded(AStanza.id(),collection,nextRef);
		}
		else
		{
			emit requestFailed(AStanza.id(),XmppStanzaError(AStanza));
	}
	}
	else if (FServerRemoveCollectionsRequests.contains(AStanza.id()))
	{
		IArchiveRequest request = FServerRemoveCollectionsRequests.take(AStanza.id());
		if (AStanza.type() == "result")
			emit collectionsRemoved(AStanza.id(),request);
		else
			emit requestFailed(AStanza.id(),XmppStanzaError(AStanza));
	}
	else if (FServerLoadModificationsRequests.contains(AStanza.id()))
	{
		ServerModificationsRequest request = FServerLoadModificationsRequests.take(AStanza.id());
		if (AStanza.type() == "result")
		{
			IArchiveModifications modifications;
			modifications.isValid = true;

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
					modif.action = IArchiveModification::Changed;
					modifications.items.append(modif);
				}
				else if (changeElem.tagName() == "removed")
				{
					modif.action = IArchiveModification::Removed;
					modifications.items.append(modif);
				}
				changeElem = changeElem.nextSiblingElement();
			}

			ResultSet resultSet = readResultSetAnswer(modifsElem);
			QString nextRef = getNextRef(resultSet,modifications.items.count(),MAX_MODIFICATION_ITEMS,request.count);
			modifications.next = resultSet.last;
			modifications.start = resultSet.last.isEmpty() ? QDateTime::currentDateTime() : request.start;

			emit serverModificationsLoaded(AStanza.id(),modifications,nextRef);
		}
		else
		{
		emit requestFailed(AStanza.id(),XmppStanzaError(AStanza));
}
	}
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
			caps |= ArchiveReplication;
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
		case ArchiveReplication:
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
	QString id = saveServerCollection(AStreamJid,ACollection);
	if (!id.isEmpty())
		{
		LocalCollectionRequest request;
		request.id = QUuid::createUuid().toString();
		request.streamJid = AStreamJid;
		request.collection = ACollection;
		FLocalSaveCollectionRequests.insert(id,request);
		return request.id;
	}
	return QString::null;
}

QString ServerMessageArchive::loadHeaders(const Jid &AStreamJid, const IArchiveRequest &ARequest)
{
	QString id = loadServerHeaders(AStreamJid,ARequest);
	if (!id.isEmpty())
	{
		LocalHeadersRequest request;
		request.id = QUuid::createUuid().toString();
		request.streamJid = AStreamJid;
		request.request = ARequest;
		FLocalLoadHeadersRequests.insert(id,request);
		return request.id;
	}
	return QString::null;
}

QString ServerMessageArchive::loadCollection(const Jid &AStreamJid, const IArchiveHeader &AHeader)
{
	QString id = loadServerCollection(AStreamJid,AHeader);
	if (!id.isEmpty())
	{
		LocalCollectionRequest request;
		request.id = QUuid::createUuid().toString();
		request.streamJid = AStreamJid;
		request.collection.header = AHeader;
		FLocalLoadCollectionRequests.insert(id,request);
		return request.id;
	}
	return QString::null;
}

QString ServerMessageArchive::removeCollections(const Jid &AStreamJid, const IArchiveRequest &ARequest)
{
	if (FStanzaProcessor && isCapable(AStreamJid,ArchiveManagement))
	{
		Stanza stanza("iq");
		stanza.setType("set").setId(FStanzaProcessor->newId());

		QDomElement removeElem = stanza.addElement("remove",FNamespaces.value(AStreamJid));
		if (ARequest.with.isValid())
			removeElem.setAttribute("with",ARequest.with.full());
		if (ARequest.with.isValid() && ARequest.exactmatch)
			removeElem.setAttribute("exactmatch",QVariant(ARequest.exactmatch).toString());
		if (ARequest.start.isValid())
			removeElem.setAttribute("start",DateTime(ARequest.start).toX85UTC());
		if (ARequest.end.isValid())
			removeElem.setAttribute("end",DateTime(ARequest.end).toX85UTC());
		if (ARequest.openOnly)
			removeElem.setAttribute("open",QVariant(ARequest.openOnly).toString());

		if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,stanza,ARCHIVE_REQUEST_TIMEOUT))
		{
			FServerRemoveCollectionsRequests.insert(stanza.id(),ARequest);
			return stanza.id();
		}
	}
	return QString::null;
}

QString ServerMessageArchive::loadModifications(const Jid &AStreamJid, const QDateTime &AStart, int ACount, const QString &ANextRef)
{
	QString id = loadServerModifications(AStreamJid,AStart,ACount,ANextRef);
	if (!id.isEmpty())
	{
		LocalModificationsRequest request;
		request.id = QUuid::createUuid().toString();
		request.streamJid = AStreamJid;
		request.start = AStart;
		request.count = ACount;
		FLocalLoadModificationsRequests.insert(id,request);
		return request.id;
	}
	return QString::null;
}

QString ServerMessageArchive::loadServerHeaders(const Jid &AStreamJid, const IArchiveRequest &ARequest, const QString &ANextRef)
{
	if (FStanzaProcessor && isCapable(AStreamJid,ArchiveManagement))
	{
		Stanza stanza("iq");
		stanza.setType("get").setId(FStanzaProcessor->newId());

		QDomElement listElem = stanza.addElement("list",FNamespaces.value(AStreamJid));
		if (ARequest.with.isValid())
			listElem.setAttribute("with",ARequest.with.full());
		if (ARequest.with.isValid() && ARequest.exactmatch)
			listElem.setAttribute("exactmatch",QVariant(ARequest.exactmatch).toString());
		if (ARequest.start.isValid())
			listElem.setAttribute("start",DateTime(ARequest.start).toX85UTC());
		if (ARequest.end.isValid())
			listElem.setAttribute("end",DateTime(ARequest.end).toX85UTC());
		insertResultSetRequest(listElem,ANextRef,MAX_HEADER_ITEMS,ARequest.maxItems,ARequest.order);
		
		if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,stanza,ARCHIVE_REQUEST_TIMEOUT))
		{
			FServerLoadHeadersRequests.insert(stanza.id(),ARequest);
			return stanza.id();
		}
	}
	return QString::null;
}

QString ServerMessageArchive::saveServerCollection(const Jid &AStreamJid, const IArchiveCollection &ACollection, const QString &ANextRef)
{
	if (FStanzaProcessor && isCapable(AStreamJid,ManualArchiving) && ACollection.header.with.isValid() && ACollection.header.start.isValid())
	{
		Stanza stanza("iq");
		stanza.setType("set").setId(FStanzaProcessor->newId());

		QDomElement chatElem = stanza.addElement("save",FNamespaces.value(AStreamJid)).appendChild(stanza.createElement("chat")).toElement();

		IArchiveItemPrefs itemPrefs = FArchiver->archiveItemPrefs(AStreamJid,ACollection.header.with,ACollection.header.threadId);
		FArchiver->collectionToElement(ACollection,chatElem,itemPrefs.save);

		int curItem = 0;
		int firstItem = !ANextRef.isEmpty() ? ANextRef.toInt() : 0;

		QByteArray uploadData;
		QTextStream ts(&uploadData, QIODevice::WriteOnly);
		ts.setCodec("UTF-8");
		int maxUploadSize = Options::node(OPV_SERVERARCHIVE_MAXUPLOADSIZE).value().toInt();
		
		QString nextRef;
		QDomElement itemElem = chatElem.firstChildElement();
		while(!itemElem.isNull())
		{
			bool removeItem;
			if (curItem < firstItem)
			{
				removeItem = true;
			}
			else if (curItem == firstItem)
			{
				itemElem.save(ts,0);
				removeItem = false;
			}
			else if (uploadData.size() > maxUploadSize)
			{
				removeItem = true;
			}
			else
		{
				itemElem.save(ts,0);
				removeItem = (uploadData.size() > maxUploadSize);
		}

			if (removeItem)
			{
				if (curItem>firstItem && nextRef.isEmpty())
					nextRef = QString::number(curItem);

				QDomElement deleteElem = itemElem;
				itemElem = itemElem.nextSiblingElement();
				chatElem.removeChild(deleteElem);
			}
			else
			{
				itemElem = itemElem.nextSiblingElement();
			}

			curItem++;
		}

		if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,stanza,ARCHIVE_REQUEST_TIMEOUT))
		{
			ServerCollectionRequest requets;
			requets.nextRef = nextRef;
			requets.collection = ACollection;
			FServerSaveCollectionRequests.insert(stanza.id(),requets);
			return stanza.id();
		}
	}
	return QString::null;
}

QString ServerMessageArchive::loadServerCollection(const Jid &AStreamJid, const IArchiveHeader &AHeader, const QString &ANextRef)
{
	if (FStanzaProcessor && isCapable(AStreamJid,ArchiveManagement) && AHeader.with.isValid() && AHeader.start.isValid())
	{
		Stanza stanza("iq");
		stanza.setType("get").setId(FStanzaProcessor->newId());

		QDomElement retrieveElem = stanza.addElement("retrieve",FNamespaces.value(AStreamJid));
		retrieveElem.setAttribute("with",AHeader.with.full());
		retrieveElem.setAttribute("start",DateTime(AHeader.start).toX85UTC());
		insertResultSetRequest(retrieveElem,ANextRef,MAX_MESSAGE_ITEMS);

		if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,stanza,ARCHIVE_REQUEST_TIMEOUT))
		{
			FServerLoadCollectionRequests.insert(stanza.id(),AHeader);
			return stanza.id();
		}
	}
	return QString::null;
}

QString ServerMessageArchive::loadServerModifications(const Jid &AStreamJid, const QDateTime &AStart, int ACount, const QString &ANextRef)
{
	if (FStanzaProcessor && isCapable(AStreamJid,ArchiveManagement) && AStart.isValid() && ACount>0)
	{
		Stanza stanza("iq");
		stanza.setType("get").setId(FStanzaProcessor->newId());

		QDomElement modifyElem = stanza.addElement("modified",FNamespaces.value(AStreamJid));
		modifyElem.setAttribute("start",DateTime(AStart).toX85UTC());
		insertResultSetRequest(modifyElem,ANextRef,MAX_MODIFICATION_ITEMS,ACount);
		
		if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,stanza,ARCHIVE_REQUEST_TIMEOUT))
		{
			ServerModificationsRequest request = {AStart,ACount};
			FServerLoadModificationsRequests.insert(stanza.id(),request);
			return stanza.id();
		}
	}
	return QString::null;
}

ResultSet ServerMessageArchive::readResultSetAnswer(const QDomElement &AElem) const
{
	ResultSet resultSet;

	QDomElement setElem = AElem.firstChildElement("set");
	while (!setElem.isNull() && setElem.namespaceURI()!=NS_RESULTSET)
		setElem = setElem.nextSiblingElement("set");

	if (!setElem.isNull())
	{
		bool countOk = false;
		bool indexOk = false;
		resultSet.count = setElem.firstChildElement("count").text().toUInt(&countOk);
		resultSet.index = setElem.firstChildElement("first").attribute("index").toUInt(&indexOk);
	resultSet.first = setElem.firstChildElement("first").text();
	resultSet.last = setElem.firstChildElement("last").text();
		resultSet.hasCount = false && countOk && indexOk; // index and count MAY be approximate so we can not relay on them
	}

	return resultSet;
}

void ServerMessageArchive::insertResultSetRequest(QDomElement &AElem, const QString &ANextRef, quint32 ALimit, quint32 AMax, Qt::SortOrder AOrder) const
{
	QDomElement setElem = AElem.appendChild(AElem.ownerDocument().createElementNS(NS_RESULTSET,"set")).toElement();
	setElem.appendChild(AElem.ownerDocument().createElement("max")).appendChild(AElem.ownerDocument().createTextNode(QString::number(qMin(AMax,ALimit))));
	if (AOrder==Qt::AscendingOrder && !ANextRef.isEmpty())
		setElem.appendChild(AElem.ownerDocument().createElement("after")).appendChild(AElem.ownerDocument().createTextNode(ANextRef));
	else if (AOrder==Qt::DescendingOrder && !ANextRef.isEmpty())
		setElem.appendChild(AElem.ownerDocument().createElement("before")).appendChild(AElem.ownerDocument().createTextNode(ANextRef));
	else if (AOrder == Qt::DescendingOrder)
		setElem.appendChild(AElem.ownerDocument().createElement("before"));
}

QString ServerMessageArchive::getNextRef(const ResultSet &AResultSet, quint32 ACount, quint32 ALimit, quint32 AMax, Qt::SortOrder AOrder) const
{
	QString nextRef;
	if (ACount > 0)
	{
		if (AResultSet.hasCount)
		{
			bool hasMore = AOrder==Qt::AscendingOrder ? AResultSet.index+ACount<AResultSet.count : AResultSet.index>0;
			if (hasMore && ACount<AMax)
				nextRef = AOrder==Qt::AscendingOrder ? AResultSet.last : AResultSet.first;
		}
		else
		{
			bool hasMore = ACount >= qMin(AMax,ALimit);
			if (hasMore && ACount<AMax)
				nextRef = AOrder==Qt::AscendingOrder ? AResultSet.last : AResultSet.first;
		}
	}
	return nextRef;
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
	if (FLocalLoadHeadersRequests.contains(AId))
	{
		LocalHeadersRequest request = FLocalLoadHeadersRequests.take(AId);
		emit requestFailed(request.id,AError);
	}
	else if (FLocalSaveCollectionRequests.contains(AId))
	{
		LocalCollectionRequest request = FLocalSaveCollectionRequests.take(AId);
		emit requestFailed(request.id,AError);
	}
	else if (FLocalLoadCollectionRequests.contains(AId))
	{
		LocalCollectionRequest request = FLocalLoadCollectionRequests.take(AId);
		emit requestFailed(request.id,AError);
	}
	else if (FLocalLoadModificationsRequests.contains(AId))
	{
		LocalModificationsRequest request = FLocalLoadModificationsRequests.take(AId);
		emit requestFailed(request.id,AError);
	}
}

void ServerMessageArchive::onServerHeadersLoaded(const QString &AId, const QList<IArchiveHeader> &AHeaders, const QString &ANextRef)
{
	if (FLocalLoadHeadersRequests.contains(AId))
	{
		LocalHeadersRequest request = FLocalLoadHeadersRequests.take(AId);
		request.headers += AHeaders;

		if (!ANextRef.isEmpty() && (quint32)request.headers.count()<request.request.maxItems)
		{
			IArchiveRequest nextRequest = request.request;
			nextRequest.maxItems -= request.headers.count();

			QString id = loadServerHeaders(request.streamJid,nextRequest,ANextRef);
			if (!id.isEmpty())
				FLocalLoadHeadersRequests.insert(id,request);
			else
				emit requestFailed(request.id,XmppError(IERR_HISTORY_HEADERS_LOAD_ERROR));
		}
		else
		{
			emit headersLoaded(request.id,request.headers);
		}
	}
}

void ServerMessageArchive::onServerCollectionSaved(const QString &AId, const IArchiveCollection &ACollection, const QString &ANextRef)
{
	if (FLocalSaveCollectionRequests.contains(AId))
	{
		LocalCollectionRequest request = FLocalSaveCollectionRequests.take(AId);
		if (!ANextRef.isEmpty())
{
			QString id = saveServerCollection(request.streamJid,request.collection,ANextRef);
			if (!id.isEmpty())
				FLocalSaveCollectionRequests.insert(id,request);
			else
				emit requestFailed(request.id,XmppError(IERR_HISTORY_CONVERSATION_SAVE_ERROR));
		}
		else
	{
			emit collectionSaved(request.id,ACollection);
		}
	}
}

void ServerMessageArchive::onServerCollectionLoaded(const QString &AId, const IArchiveCollection &ACollection, const QString &ANextRef)
{
	if (FLocalLoadCollectionRequests.contains(AId))
	{
		LocalCollectionRequest request = FLocalLoadCollectionRequests.take(AId);
		request.collection.header = ACollection.header;
		request.collection.body.messages += ACollection.body.messages;
		request.collection.body.notes += ACollection.body.notes;
		if (!ANextRef.isEmpty())
		{
			QString id = loadServerCollection(request.streamJid,ACollection.header,ANextRef);
			if (!id.isEmpty())
				FLocalLoadCollectionRequests.insert(id,request);
			else
				emit requestFailed(request.id,XmppError(IERR_HISTORY_CONVERSATION_LOAD_ERROR));
		}
		else
		{
			emit collectionLoaded(request.id,request.collection);
		}
	}
}

void ServerMessageArchive::onServerModificationsLoaded(const QString &AId, const IArchiveModifications &AModifs, const QString &ANextRef)
{
	if (FLocalLoadModificationsRequests.contains(AId))
	{
		LocalModificationsRequest request = FLocalLoadModificationsRequests.take(AId);
		request.modifications.start = AModifs.start;
		request.modifications.next = AModifs.next;
		request.modifications.items += AModifs.items;

		if (!ANextRef.isEmpty() && (quint32)request.modifications.items.count()<request.count)
		{
			quint32 nextCount = request.count-request.modifications.items.count();
			QString id = loadServerModifications(request.streamJid,request.start,nextCount,ANextRef);
			if (!id.isEmpty())
				FLocalLoadModificationsRequests.insert(id,request);
			else
				emit requestFailed(request.id,XmppError(IERR_HISTORY_MODIFICATIONS_LOAD_ERROR));
		}
		else
		{
			emit modificationsLoaded(request.id,request.modifications);
		}
	}
}
