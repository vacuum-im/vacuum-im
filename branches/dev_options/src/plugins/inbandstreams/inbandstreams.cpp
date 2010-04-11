#include "inbandstreams.h"

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

bool InBandStreams::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *plugin = APluginManager->pluginInterface("IDataStreamsManager").value(0,NULL);
  if (plugin)
  {
    FDataManager = qobject_cast<IDataStreamsManager *>(plugin->instance());
  }

  plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0,NULL);
  if (plugin)
  {
    FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());
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
  Options::registerOption(OPV_DATASTREAMS_METHOD_BLOCKSIZE,4096,tr("Block Size"));
  Options::registerOption(OPV_DATASTREAMS_METHOD_MAXBLOCKSIZE,10240,tr("Maximum Block Size"));
  Options::registerOption(OPV_DATASTREAMS_METHOD_STANZATYPE,(int)IInBandStream::StanzaIq,tr("Stanza Type"));
  return true;
}

QString InBandStreams::methodNS() const
{
  return NS_INBAND_BYTESTREAMS;
}

QString InBandStreams::methodName() const
{
  return tr("In-Band Data Stream");
}

QString InBandStreams::methodDescription() const
{
  return tr("Data is broken down into smaller chunks and transported in-band over XMPP");
}

IDataStreamSocket *InBandStreams::dataStreamSocket(const QString &ASocketId, const Jid &AStreamJid, const Jid &AContactJid, 
                                                   IDataStreamSocket::StreamKind AKind, QObject *AParent)
{
  if (FStanzaProcessor)
  {
    InBandStream *stream = new InBandStream(FStanzaProcessor,ASocketId,AStreamJid,AContactJid,AKind,AParent);
    emit socketCreated(stream);
    return stream;
  }
  return NULL;
}

IOptionsWidget *InBandStreams::methodSettingsWidget(const OptionsNode &ANode, bool AReadOnly, QWidget *AParent)
{
  return new InBandOptions(this,ANode,AReadOnly,AParent);
}

IOptionsWidget *InBandStreams::methodSettingsWidget(IDataStreamSocket *ASocket, bool AReadOnly, QWidget *AParent)
{
  IInBandStream *stream = qobject_cast<IInBandStream *>(ASocket->instance());
  return stream!=NULL ? new InBandOptions(this,stream,AReadOnly,AParent) : NULL;
}

void InBandStreams::saveMethodSettings(IOptionsWidget *AWidget, OptionsNode ANode)
{
  InBandOptions *widget = qobject_cast<InBandOptions *>(AWidget->instance());
  if (widget)
    widget->apply(ANode);
}

void InBandStreams::loadMethodSettings(IDataStreamSocket *ASocket, IOptionsWidget *AWidget)
{
  InBandOptions *widget = qobject_cast<InBandOptions *>(AWidget->instance());
  IInBandStream *stream = qobject_cast<IInBandStream *>(ASocket->instance());
  if (widget && stream)
    widget->apply(stream);
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
}

Q_EXPORT_PLUGIN2(plg_inbandstreams, InBandStreams);
