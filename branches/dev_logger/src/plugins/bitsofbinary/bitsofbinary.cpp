#include "bitsofbinary.h"

#include <QFile>
#include <QDateTime>
#include <QFileInfo>
#include <QDomDocument>
#include <QXmlStreamReader>
#include <QCryptographicHash>
#include <definitions/namespaces.h>
#include <definitions/internalerrors.h>
#include <definitions/stanzahandlerorders.h>
#include <definitions/xmppstanzahandlerorders.h>
#include <utils/xmpperror.h>
#include <utils/stanza.h>
#include <utils/logger.h>

#define DIR_DATA                "bitsofbinary"
#define LOAD_TIMEOUT            30000

#define SHC_REQUEST             "/iq[@type='get']/data[@xmlns='" NS_BITS_OF_BINARY "']"

BitsOfBinary::BitsOfBinary()
{
	FPluginManager = NULL;
	FXmppStreams = NULL;
	FStanzaProcessor = NULL;
	FDiscovery = NULL;

	FOfflineTimer.setInterval(0);
	connect(&FOfflineTimer,SIGNAL(timeout()),SLOT(onOfflineTimerTimeout()));
}

BitsOfBinary::~BitsOfBinary()
{

}

void BitsOfBinary::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Bits Of Binary");
	APluginInfo->description = tr("Allows other modules to receive or send a small amount of binary data in XMPP stanza");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(XMPPSTREAMS_UUID);
	APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool BitsOfBinary::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
	FPluginManager = APluginManager;

	IPlugin *plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0,NULL);
	if (plugin)
	{
		FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());
		if (FStanzaProcessor == NULL)
			LOG_WARNING("Failed to find required interface: IStanzaProcessor");
	}
	
	plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
	if (plugin)
	{
		FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
		if (FXmppStreams)
			connect(FXmppStreams->instance(),SIGNAL(created(IXmppStream *)),SLOT(onXmppStreamCreated(IXmppStream *)));
		else
			LOG_WARNING("Failed to find required interface: IXmppStreams");
	}

	plugin = APluginManager->pluginInterface("IServiceDiscovery").value(0,NULL);
	if (plugin)
	{
		FDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());
	}

	return FStanzaProcessor!=NULL && FXmppStreams!=NULL;
}

bool BitsOfBinary::initObjects()
{
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_BOB_INVALID_RESPONCE,tr("Invalid response"));
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_BOB_DATA_LOAD_ERROR,tr("Failed to load data"));
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_BOB_DATA_SAVE_ERROR,tr("Failed to save data"));

	FDataDir.setPath(FPluginManager->homePath());
	if (!FDataDir.exists(DIR_DATA))
		FDataDir.mkdir(DIR_DATA);
	FDataDir.cd(DIR_DATA);

	if (FStanzaProcessor)
	{
		IStanzaHandle requestHandle;
		requestHandle.handler = this;
		requestHandle.order = SHO_DEFAULT;
		requestHandle.direction = IStanzaHandle::DirectionIn;
		requestHandle.conditions.append(SHC_REQUEST);
		FSHIRequest = FStanzaProcessor->insertStanzaHandle(requestHandle);
	}

	if (FDiscovery)
	{
		IDiscoFeature feature;
		feature.active = true;
		feature.var = NS_BITS_OF_BINARY;
		feature.name = tr("Bits Of Binary");
		feature.description = tr("Supports the exchange of a small amount of binary data in XMPP stanza");
		FDiscovery->insertDiscoFeature(feature);
	}

	return true;
}

bool BitsOfBinary::initSettings()
{
	foreach(QFileInfo fileInfo, FDataDir.entryInfoList(QDir::Files))
	{
		QFile file(fileInfo.absoluteFilePath());
		if (file.open(QFile::ReadOnly))
		{
			quint64 maxAge = 0;
			QXmlStreamReader reader(&file);
			while (!reader.atEnd())
			{
				reader.readNext();
				if (reader.isStartElement() && reader.qualifiedName() == "data")
				{
					maxAge = reader.attributes().value("max-age").toString().toLongLong();
					break;
				}
				else if (!reader.isStartDocument())
				{
					break;
				}
			}
			file.close();

			if (fileInfo.lastModified().addSecs(maxAge) < QDateTime::currentDateTime())
				QFile::remove(fileInfo.absoluteFilePath());
		}
		else
		{
			QFile::remove(fileInfo.absoluteFilePath());
			REPORT_ERROR(QString("Failed to check data file: %1").arg(file.errorString()));
		}
	}
	return true;
}

bool BitsOfBinary::xmppStanzaIn(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder)
{
	Q_UNUSED(AXmppStream);
	if (AOrder == XSHO_BITSOFBINARY)
	{
		QDomElement dataElem = AStanza.tagName()=="iq" ? AStanza.firstElement().firstChildElement("data") : AStanza.firstElement("data");
		while (!dataElem.isNull())
		{
			if (dataElem.namespaceURI() == NS_BITS_OF_BINARY)
			{
				QString cid = dataElem.attribute("cid");
				QString type = dataElem.attribute("type");
				QByteArray data = QByteArray::fromBase64(dataElem.text().toLatin1());
				quint64 maxAge = dataElem.attribute("max-age").toLongLong();

				LOG_STRM_INFO(AXmppStream->streamJid(),QString("Received data, id=%1, from=%2").arg(cid,AStanza.from()));
				saveBinary(cid,type,data,maxAge);
			}
			dataElem = dataElem.nextSiblingElement("data");
		}
	}
	return false;
}

bool BitsOfBinary::xmppStanzaOut(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder)
{
	Q_UNUSED(AXmppStream);
	Q_UNUSED(AStanza);
	Q_UNUSED(AOrder);
	return false;
}

bool BitsOfBinary::stanzaReadWrite(int AHandleId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept)
{
	if (AHandleId == FSHIRequest)
	{
		AAccept = true;
		QDomElement dataElem = AStanza.firstElement("data",NS_BITS_OF_BINARY);

		QString cid = dataElem.attribute("cid");
		QString type;	QByteArray data; quint64 maxAge;
		if (!cid.isEmpty() && loadBinary(cid,type,data,maxAge))
		{
			LOG_STRM_INFO(AStreamJid,QString("Sending data, id=%1, to=%2").arg(cid,AStanza.from()));
			Stanza result = FStanzaProcessor->makeReplyResult(AStanza);
			dataElem = result.addElement("data",NS_BITS_OF_BINARY);
			dataElem.setAttribute("cid",cid);
			dataElem.setAttribute("type",type);
			dataElem.setAttribute("max-age",maxAge);
			dataElem.appendChild(result.createTextNode(data.toBase64()));
			FStanzaProcessor->sendStanzaOut(AStreamJid,result);
		}
		else
		{
			LOG_STRM_WARNING(AStreamJid,QString("Rejected request for absent data, id=%1, from=%1").arg(cid,AStanza.from()));
			Stanza error = FStanzaProcessor->makeReplyError(AStanza,XmppStanzaError::EC_ITEM_NOT_FOUND);
			FStanzaProcessor->sendStanzaOut(AStreamJid, error);
		}
	}
	return false;
}

void BitsOfBinary::stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza)
{
	Q_UNUSED(AStreamJid);
	if (FLoadRequests.contains(AStanza.id()))
	{
		QString cid = FLoadRequests.take(AStanza.id());
		if (AStanza.type() == "result")
		{
			QDomElement dataElem = AStanza.firstElement("data",NS_BITS_OF_BINARY);
			QString type = dataElem.attribute("type");
			QByteArray data = QByteArray::fromBase64(dataElem.text().toLatin1());
			quint64 maxAge = dataElem.attribute("max-age").toLongLong();
			if (cid!=dataElem.attribute("cid") || type.isEmpty() || data.isEmpty())
			{
				LOG_STRM_WARNING(AStreamJid,QString("Failed to request data, id=%1, from=%2: Invalid response").arg(cid,AStanza.from()));
				emit binaryError(cid,XmppError(IERR_BOB_INVALID_RESPONCE));
			}
			else if(!saveBinary(cid,type,data,maxAge))
			{
				LOG_STRM_ERROR(AStreamJid,QString("Failed to request data, id=%1, from=%2: Failed to save data").arg(cid,AStanza.from()));
				emit binaryError(cid,XmppError(IERR_BOB_DATA_SAVE_ERROR));
			}
			else
			{
				// Data was saved in xmppStanzaIn
			}
		}
		else
		{
			LOG_STRM_WARNING(AStreamJid,QString("Failed to request data, id=%1, from=%2: %3").arg(cid,AStanza.from(),XmppStanzaError(AStanza).errorMessage()));
			emit binaryError(cid,XmppStanzaError(AStanza));
		}
	}
}

QString BitsOfBinary::contentIdentifier(const QByteArray &AData) const
{
	return "sha1+"+QCryptographicHash::hash(AData,QCryptographicHash::Sha1).toHex()+"@bob.xmpp.org";
}

bool BitsOfBinary::isSupported(const Jid &AStreamJid, const Jid &AContactJid) const
{
	return FDiscovery==NULL || !FDiscovery->hasDiscoInfo(AStreamJid,AContactJid) || FDiscovery->discoInfo(AStreamJid,AContactJid).features.contains(NS_BITS_OF_BINARY);
}

bool BitsOfBinary::hasBinary(const QString &AContentId) const
{
	return QFile::exists(contentFileName(AContentId));
}

bool BitsOfBinary::loadBinary(const QString &AContentId, const Jid &AStreamJid, const Jid &AContactJid)
{
	if (FStanzaProcessor)
	{
		if (!hasBinary(AContentId))
		{
			if (!FLoadRequests.values().contains(AContentId))
			{
				Stanza request("iq");
				request.setTo(AContactJid.full()).setId(FStanzaProcessor->newId()).setType("get");
				QDomElement dataElem = request.addElement("data",NS_BITS_OF_BINARY);
				dataElem.setAttribute("cid",AContentId);
				if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,request,LOAD_TIMEOUT))
				{
					LOG_STRM_INFO(AStreamJid,QString("Data request sent, id=%1, from=%2").arg(AContentId,AContactJid.full()));
					FLoadRequests.insert(request.id(),AContentId);
					return true;
				}
				else
				{
					LOG_STRM_WARNING(AStreamJid,QString("Failed to send data request, id=%1, to=%2").arg(AContentId,AContactJid.full()));
				}
			}
			else
			{
				return true;
			}
		}
		else
		{
			FOfflineRequests.append(AContentId);
			FOfflineTimer.start();
			return true;
		}
	}
	return false;
}

bool BitsOfBinary::loadBinary(const QString &AContentId, QString &AType, QByteArray &AData, quint64 &AMaxAge)
{
	QFile file(contentFileName(AContentId));
	if (file.open(QFile::ReadOnly))
	{
		QDomDocument doc;
		if (doc.setContent(&file,true) && AContentId==doc.documentElement().attribute("cid"))
		{
			AType = doc.documentElement().attribute("type");
			AData = QByteArray::fromBase64(doc.documentElement().text().toLatin1());
			AMaxAge = doc.documentElement().attribute("max-age").toLongLong();
			return true;
		}
		else
		{
			REPORT_ERROR("Failed to load data from file: Invalid format");
		}
	}
	else
	{
		REPORT_ERROR(QString("Failed to load data from file: %1").arg(file.errorString()));
	}
	return false;
}

bool BitsOfBinary::saveBinary(const QString &AContentId, const QString &AType, const QByteArray &AData, quint64 AMaxAge)
{
	if (!AContentId.isEmpty() && !AType.isEmpty() && !AData.isEmpty())
	{
		QFile file(contentFileName(AContentId));
		if (file.open(QFile::WriteOnly|QFile::Truncate))
		{
			QDomDocument doc;
			QDomElement dataElem = doc.appendChild(doc.createElement("data")).toElement();
			dataElem.setAttribute("cid",AContentId);
			dataElem.setAttribute("type",AType);
			dataElem.setAttribute("max-age",AMaxAge);
			dataElem.appendChild(doc.createTextNode(AData.toBase64()));
			if (file.write(doc.toByteArray()) > 0)
			{
				file.close();
				emit binaryCached(AContentId,AType,AData,AMaxAge);
				return true;
			}
			else
			{
				REPORT_ERROR("Failed to save data to file: Write error");
			}
		}
		else
		{
			REPORT_ERROR(QString("Failed to save data to file: %1").arg(file.errorString()));
		}
	}
	else
	{
		REPORT_ERROR("Failed to save data to file: Invalid params");
	}
	return false;
}

bool BitsOfBinary::saveBinary(const QString &AContentId, const QString &AType, const QByteArray &AData, quint64 AMaxAge, Stanza &AStanza)
{
	if (!AContentId.isEmpty() && !AType.isEmpty() && !AData.isEmpty())
	{
		AStanza.detach();
		QDomElement dataElem = AStanza.addElement("data",NS_BITS_OF_BINARY);
		dataElem.setAttribute("cid",AContentId);
		dataElem.setAttribute("type",AType);
		dataElem.setAttribute("max-age",AMaxAge);
		dataElem.appendChild(AStanza.createTextNode(AData.toBase64()));
		return true;
	}
	else
	{
		REPORT_ERROR("Failed to save data to stanza: Invalid params");
	}
	return false;
}

bool BitsOfBinary::removeBinary(const QString &AContentId)
{
	if (QFile::remove(contentFileName(AContentId)))
	{
		emit binaryRemoved(AContentId);
		return true;
	}
	return false;
}

QString BitsOfBinary::contentFileName(const QString &AContentId) const
{
	return FDataDir.absoluteFilePath(QCryptographicHash::hash(AContentId.toUtf8(),QCryptographicHash::Sha1).toHex());
}

void BitsOfBinary::onXmppStreamCreated(IXmppStream *AXmppStream)
{
	AXmppStream->insertXmppStanzaHandler(XSHO_BITSOFBINARY,this);
}

void BitsOfBinary::onOfflineTimerTimeout()
{
	foreach(const QString &contentId, FOfflineRequests.toSet())
	{
		QString type; QByteArray data; quint64 maxAge;
		if (loadBinary(contentId,type,data,maxAge))
			emit binaryCached(contentId,type,data,maxAge);
		else
			emit binaryError(contentId,XmppError(IERR_BOB_DATA_LOAD_ERROR));
	}
}

Q_EXPORT_PLUGIN2(plg_bitsofbinary, BitsOfBinary)
