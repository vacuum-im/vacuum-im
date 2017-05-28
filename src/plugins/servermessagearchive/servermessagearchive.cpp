#include "servermessagearchive.h"

#include <QTextStream>
#include <QDomElement>
#include <definitions/namespaces.h>
#include <definitions/optionvalues.h>
#include <definitions/internalerrors.h>
#include <definitions/stanzahandlerorders.h>
#include <definitions/archivecapabilityorders.h>
#include <utils/datetime.h>
#include <utils/options.h>
#include <utils/logger.h>

#define MAX_HEADER_ITEMS          50
#define MAX_MESSAGE_ITEMS         25
#define MAX_MODIFICATION_ITEMS    50

#define ARCHIVE_REQUEST_TIMEOUT   30000

#define SHC_ARCHIVE_RESULT        "/message/result[@xmlns=" NS_ARCHIVE_MAM "]"
#define SHC_ARCHIVE_RESULT_0      "/message/result[@xmlns=" NS_ARCHIVE_MAM_0 "]"


ServerMessageArchive::ServerMessageArchive()
{
	FArchiver = NULL;
	FDataForms = NULL;
	FStanzaProcessor = NULL;
}

ServerMessageArchive::~ServerMessageArchive()
{

}

void ServerMessageArchive::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Server Message Archive");
	APluginInfo->description = tr("Allows to save the history of communications on the server");
	APluginInfo->version = "2.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(DATAFORMS_UUID);
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

	plugin = APluginManager->pluginInterface("IDataForms").value(0,NULL);
	if (plugin)
	{
		FDataForms = qobject_cast<IDataForms *>(plugin->instance());
	}

	return FArchiver!=NULL && FStanzaProcessor!=NULL && FDataForms!=NULL;
}

bool ServerMessageArchive::initObjects()
{
	if (FArchiver)
		FArchiver->registerArchiveEngine(this);
	return true;
}

bool ServerMessageArchive::initSettings()
{
	return true;
}

bool ServerMessageArchive::stanzaReadWrite(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept)
{
	if (FSHIResult.value(AStreamJid) == AHandlerId)
	{
		AAccept = true;
		QDomElement resultElem = AStanza.firstElement("result", FNamespaces.value(AStreamJid));

		QString queryid = resultElem.attribute("queryid");
		if (FLoadMessagesRequests.contains(queryid))
		{
			QDomElement forwardElem = Stanza::findElement(resultElem,"forwarded",NS_MESSAGE_FORWARD);
			QDomElement messageElem =  forwardElem.firstChildElement("message");
			if (!messageElem.isNull())
			{
				Stanza stanza(messageElem);
				Message message(stanza);

				QDomElement delayElem = Stanza::findElement(forwardElem,"delay",NS_XMPP_DELAY);
				if (!delayElem.isNull())
					message.setDateTime(DateTime(delayElem.attribute("stamp")).toLocal());

				FLoadMessagesRequests[queryid].reply.messages.append(message);
			}
		}

		return true;
	}
	return false;
}

void ServerMessageArchive::stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza)
{
	if (FLoadMessagesRequests.contains(AStanza.id()))
	{
		LoadMessagesRequest request = FLoadMessagesRequests.take(AStanza.id());
		if (AStanza.isResult())
		{
			ResultSet resultSet = readResultSetReply(AStanza.firstElement("fin", FNamespaces.value(AStreamJid)));
			request.reply.nextRef = request.request.order==Qt::AscendingOrder ? resultSet.last : resultSet.first;
			emit messagesLoaded(AStanza.id(), request.reply);
		}
		else
		{
			XmppStanzaError err(AStanza);			LOG_STRM_WARNING(AStreamJid,QString("Failed to load messages, id=%1: %2").arg(AStanza.id(),err.condition()));			emit requestFailed(AStanza.id(),err);		}
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

IOptionsDialogWidget *ServerMessageArchive::engineSettingsWidget(QWidget *AParent)
{
	Q_UNUSED(AParent);
	return NULL;
}

quint32 ServerMessageArchive::capabilities(const Jid &AStreamJid) const
{
	return FArchiver->isSupported(AStreamJid) ? IArchiveEngine::AutomaticArchiving|IArchiveEngine::ArchiveManagement : 0;
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
		case AutomaticArchiving:
			return ACO_AUTOMATIC_SERVERARCHIVE;
		case ArchiveManagement:
			return ACO_MANAGE_SERVERARCHIVE;
		default:
			break;
		}
	}
	return -1;
}

bool ServerMessageArchive::saveMessage(const Jid &AStreamJid, const Message &AMessage, bool ADirectionIn)
{
	Q_UNUSED(AStreamJid); Q_UNUSED(AMessage); Q_UNUSED(ADirectionIn);
	return false;
}

QString ServerMessageArchive::loadMessages(const Jid &AStreamJid, const IArchiveRequest &ARequest, const QString &ANextRef)
{
	if (FStanzaProcessor && FDataForms && isCapable(AStreamJid,ArchiveManagement))
	{
		Stanza stanza(STANZA_KIND_IQ);
		stanza.setType(STANZA_TYPE_SET).setUniqueId();

		QDomElement queryElem = stanza.addElement("query",FNamespaces.value(AStreamJid));
		queryElem.setAttribute("queryid", stanza.id());

		IDataForm form;
		form.type == DATAFORM_TYPE_SUBMIT;
		if (!ARequest.with.isEmpty())
		{
			IDataField with;
			with.var = "with";
			with.value = ARequest.with.full();
			form.fields.append(with);
		}
		if (ARequest.start.isValid())
		{
			IDataField start;
			start.var = "start";
			start.value = DateTime(ARequest.start).toX85UTC();
			form.fields.append(start);
		}
		if (ARequest.end.isValid())
		{
			IDataField end;
			end.var = "end";
			end.value = DateTime(ARequest.end).toX85UTC();
			form.fields.append(end);
		}
		if (!form.fields.isEmpty())
		{
			IDataField type;
			type.var = "FORM_TYPE";
			type.type = DATAFIELD_TYPE_HIDDEN;
			type.value = FNamespaces.value(AStreamJid);
			form.fields.prepend(type);

			FDataForms->xmlForm(form, queryElem);
		}

		insertResultSetRequest(queryElem,ANextRef,MAX_MESSAGE_ITEMS,ARequest.maxItems,ARequest.order);

		if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,stanza,ARCHIVE_REQUEST_TIMEOUT))
		{
			LOG_STRM_DEBUG(AStreamJid,QString("Load messages request sent, id=%1, nextref=%2").arg(stanza.id(),ANextRef));
			LoadMessagesRequest request;
			request.request = ARequest;
			FLoadMessagesRequests.insert(stanza.id(),request);
			return stanza.id();
		}
		else
		{
			LOG_STRM_WARNING(AStreamJid,"Failed to send load messages request");
		}
	}
	else if (FStanzaProcessor && FDataForms)
	{
		LOG_STRM_ERROR(AStreamJid,"Failed to load messages: Not capable");
	}
	return QString::null;

}

QString ServerMessageArchive::saveCollection(const Jid &AStreamJid, const IArchiveCollection &ACollection)
{
	return QString::null;
}

QString ServerMessageArchive::loadHeaders(const Jid &AStreamJid, const IArchiveRequest &ARequest)
{
	return QString::null;
}

QString ServerMessageArchive::loadCollection(const Jid &AStreamJid, const IArchiveHeader &AHeader)
{
	return QString::null;
}

QString ServerMessageArchive::removeCollections(const Jid &AStreamJid, const IArchiveRequest &ARequest)
{
	return QString::null;
}

QString ServerMessageArchive::loadModifications(const Jid &AStreamJid, const QDateTime &AStart, int ACount, const QString &ANextRef)
{
	return QString::null;
}

ResultSet ServerMessageArchive::readResultSetReply(const QDomElement &AElem) const
{
	ResultSet resultSet;

	QDomElement setElem = Stanza::findElement(AElem, "set", NS_RESULTSET);
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

void ServerMessageArchive::onArchivePrefsOpened(const Jid &AStreamJid)
{
	if (FStanzaProcessor)
	{
		IStanzaHandle shandle;
		shandle.handler = this;
		shandle.streamJid = AStreamJid;
		shandle.order = SHO_MI_ARCHIVE_RESULT;
		shandle.direction = IStanzaHandle::DirectionIn;
		shandle.conditions.append(SHC_ARCHIVE_RESULT);
		shandle.conditions.append(SHC_ARCHIVE_RESULT_0);
		FSHIResult.insert(shandle.streamJid,FStanzaProcessor->insertStanzaHandle(shandle));
	}

	FNamespaces.insert(AStreamJid,FArchiver->supportedNamespace(AStreamJid));

	emit capabilitiesChanged(AStreamJid);
}

void ServerMessageArchive::onArchivePrefsClosed(const Jid &AStreamJid)
{
	FSHIResult.remove(AStreamJid);
	FNamespaces.remove(AStreamJid);
	emit capabilitiesChanged(AStreamJid);
}

Q_EXPORT_PLUGIN2(plg_servermessagearchive, ServerMessageArchive)
