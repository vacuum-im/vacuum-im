#include "inbandstreams.h"

#include <definitions/namespaces.h>
#include <definitions/optionvalues.h>
#include <definitions/internalerrors.h>
#include <utils/options.h>
#include <utils/logger.h>

InBandStreams::InBandStreams()
{
	FDataManager = NULL;
	FStanzaProcessor = NULL;
	FDiscovery = NULL;
}

InBandStreams::~InBandStreams()
{

}

void InBandStreams::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("In-Band Data Stream");
	APluginInfo->description = tr("Allows to initiate in-band stream of data between two XMPP entities");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool InBandStreams::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);

	IPlugin *plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0,NULL);
	if (plugin)
	{
		FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IDataStreamsManager").value(0,NULL);
	if (plugin)
	{
		FDataManager = qobject_cast<IDataStreamsManager *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IServiceDiscovery").value(0,NULL);
	if (plugin)
	{
		FDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());
	}

	return FStanzaProcessor!=NULL;
}

bool InBandStreams::initObjects()
{
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_INBAND_STREAM_DESTROYED,tr("Stream destroyed"));
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_INBAND_STREAM_INVALID_DATA,tr("Malformed data packet"));
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_INBAND_STREAM_NOT_OPENED,tr("Failed to open stream"));
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_INBAND_STREAM_INVALID_BLOCK_SIZE,tr("Block size is not acceptable"));
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_INBAND_STREAM_DATA_NOT_SENT,tr("Failed to send data"));

	if (FDataManager)
	{
		FDataManager->insertMethod(this);
	}
	if (FDiscovery)
	{
		IDiscoFeature feature;
		feature.var = NS_INBAND_BYTESTREAMS;
		feature.active = true;
		feature.name = tr("In-Band Data Stream");
		feature.description = tr("Supports the initiating of the in-band stream of data between two XMPP entities");
		FDiscovery->insertDiscoFeature(feature);
	}
	return true;
}

bool InBandStreams::initSettings()
{
	Options::setDefaultValue(OPV_DATASTREAMS_METHOD_BLOCKSIZE,4096);
	Options::setDefaultValue(OPV_DATASTREAMS_METHOD_MAXBLOCKSIZE,10240);
	Options::setDefaultValue(OPV_DATASTREAMS_METHOD_STANZATYPE,(int)IInBandStream::StanzaIq);
	return true;
}

QString InBandStreams::methodNS() const
{
	return NS_INBAND_BYTESTREAMS;
}

QString InBandStreams::methodName() const
{
	return QString("In-Band");
}

QString InBandStreams::methodDescription() const
{
	return tr("Data is broken down into smaller chunks and transported in-band over XMPP");
}

IDataStreamSocket *InBandStreams::dataStreamSocket(const QString &AStreamId, const Jid &AStreamJid, const Jid &AContactJid, IDataStreamSocket::StreamKind AKind, QObject *AParent)
{
	if (FStanzaProcessor)
	{
		InBandStream *stream = new InBandStream(FStanzaProcessor,AStreamId,AStreamJid,AContactJid,AKind,AParent);
		emit socketCreated(stream);
		return stream;
	}
	return NULL;
}

IOptionsDialogWidget *InBandStreams::methodSettingsWidget(const OptionsNode &ANode, QWidget *AParent)
{
	return new InBandOptionsWidget(this,ANode,AParent);
}

void InBandStreams::loadMethodSettings(IDataStreamSocket *ASocket, const OptionsNode &ANode)
{
	IInBandStream *stream = qobject_cast<IInBandStream *>(ASocket->instance());
	if (stream)
	{
		stream->setMaximumBlockSize(ANode.value("max-block-size").toInt());
		stream->setBlockSize(ANode.value("block-size").toInt());
		stream->setDataStanzaType(ANode.value("stanza-type").toInt());
	}
	else
	{
		REPORT_ERROR("Failed to load inband stream settings: Invalid socket");
	}
}
