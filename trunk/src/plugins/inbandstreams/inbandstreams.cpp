#include "inbandstreams.h"

#define SVN_BLOCK_SIZE          "settings[]:blockSize"
#define SVN_MAX_BLOCK_SIZE      "settings[]:maxBlockSize"
#define SVN_DATA_STANZA_TYPE    "settings[]:dataStanzaType"

InBandStreams::InBandStreams()
{
  FDataManager = NULL;
  FStanzaProcessor = NULL;
  FSettings = NULL;
  FSettingsPlugin = NULL;
  FDiscovery = NULL;

  FMaxBlockSize = DEFAULT_MAX_BLOCK_SIZE;
  FBlockSize = DEFAULT_BLOCK_SIZE;
  FStatnzaType = DEFAULT_DATA_STANZA_TYPE;
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

  plugin = APluginManager->pluginInterface("ISettingsPlugin").value(0,NULL);
  if (plugin)
  {
    FSettingsPlugin = qobject_cast<ISettingsPlugin *>(plugin->instance());
    if (FSettingsPlugin)
    {
      connect(FSettingsPlugin->instance(),SIGNAL(settingsOpened()),SLOT(onSettingsOpened()));
      connect(FSettingsPlugin->instance(),SIGNAL(settingsClosed()),SLOT(onSettingsClosed()));
    }
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

QWidget *InBandStreams::settingsWidget(IDataStreamSocket *ASocket, bool AReadOnly)
{
  IInBandStream *stream = qobject_cast<IInBandStream *>(ASocket->instance());
  return stream!=NULL ? new InBandOptions(this,stream,AReadOnly,NULL) : NULL;
}

QWidget *InBandStreams::settingsWidget(const QString &ASettingsNS, bool AReadOnly)
{
  return new InBandOptions(this,ASettingsNS,AReadOnly,NULL);
}

void InBandStreams::loadSettings(IDataStreamSocket *ASocket, QWidget *AWidget) const
{
  InBandOptions *widget = qobject_cast<InBandOptions *>(AWidget);
  IInBandStream *stream = qobject_cast<IInBandStream *>(ASocket->instance());
  if (widget && stream)
    widget->saveSettings(stream);
}

void InBandStreams::loadSettings(IDataStreamSocket *ASocket, const QString &ASettingsNS) const
{
  IInBandStream *stream = qobject_cast<IInBandStream *>(ASocket->instance());
  if (stream)
  {
    stream->setMaximumBlockSize(maximumBlockSize(ASettingsNS));
    stream->setBlockSize(blockSize(ASettingsNS));
    stream->setDataStanzaType(dataStanzaType(ASettingsNS));
  }
}

void InBandStreams::saveSettings(const QString &ASettingsNS, QWidget *AWidget)
{
  InBandOptions *widget = qobject_cast<InBandOptions *>(AWidget);
  if (widget)
    widget->saveSettings(ASettingsNS);
}

void InBandStreams::saveSettings(const QString &ASettingsNS, IDataStreamSocket *ASocket)
{
  IInBandStream *stream = qobject_cast<IInBandStream *>(ASocket->instance());
  if (stream)
  {
    setMaximumBlockSize(ASettingsNS, stream->maximumBlockSize());
    setBlockSize(ASettingsNS, stream->blockSize());
    setDataStanzaType(ASettingsNS, stream->dataStanzaType());
  }
}

void InBandStreams::deleteSettings(const QString &ASettingsNS)
{
  if (ASettingsNS.isEmpty())
  {
    FMaxBlockSize = DEFAULT_MAX_BLOCK_SIZE;
    FBlockSize = DEFAULT_BLOCK_SIZE;
    FStatnzaType = DEFAULT_DATA_STANZA_TYPE;
  }
  else if (FSettings)
  {
    FSettings->deleteNS(ASettingsNS);
  }
}

int InBandStreams::blockSize(const QString &ASettingsNS) const
{
  if (FSettings && !ASettingsNS.isEmpty())
    return FSettings->valueNS(SVN_BLOCK_SIZE,ASettingsNS,FBlockSize).toInt();
  return FBlockSize;
}

void InBandStreams::setBlockSize(const QString &ASettingsNS, int ASize)
{
  if (ASize>=MINIMUM_BLOCK_SIZE && ASize<=maximumBlockSize(ASettingsNS))
  {
    if (ASettingsNS.isEmpty())
      FBlockSize = ASize;
    else if (FSettings && FBlockSize == ASize)
      FSettings->deleteValueNS(SVN_BLOCK_SIZE, ASettingsNS);
    else if (FSettings)
      FSettings->setValueNS(SVN_BLOCK_SIZE, ASettingsNS, ASize);
  }
}

int InBandStreams::maximumBlockSize(const QString &ASettingsNS) const
{
  if (FSettings && !ASettingsNS.isEmpty())
    return FSettings->valueNS(SVN_MAX_BLOCK_SIZE,ASettingsNS,FMaxBlockSize).toInt();
  return FMaxBlockSize;
}

void InBandStreams::setMaximumBlockSize(const QString &ASettingsNS, int AMaxSize)
{
  if (AMaxSize>=MINIMUM_BLOCK_SIZE && AMaxSize<=USHRT_MAX)
  {
    if (ASettingsNS.isEmpty())
      FMaxBlockSize = AMaxSize;
    else if (FSettings && FMaxBlockSize == AMaxSize)
      FSettings->deleteValueNS(SVN_MAX_BLOCK_SIZE, ASettingsNS);
    else if (FSettings)
      FSettings->setValueNS(SVN_MAX_BLOCK_SIZE, ASettingsNS, AMaxSize);
  }
}

int InBandStreams::dataStanzaType(const QString &ASettingsNS) const
{
  if (FSettings && !ASettingsNS.isEmpty())
    return FSettings->valueNS(SVN_DATA_STANZA_TYPE,ASettingsNS,FStatnzaType).toInt();
  return FStatnzaType;
}

void InBandStreams::setDataStanzaType(const QString &ASettingsNS, int AType)
{
  if (ASettingsNS.isEmpty())
    FStatnzaType = AType;
  else if (FSettings && FStatnzaType == AType)
    FSettings->deleteValueNS(SVN_DATA_STANZA_TYPE, ASettingsNS);
  else if (FSettings)
    FSettings->setValueNS(SVN_DATA_STANZA_TYPE, ASettingsNS, AType);
}

void InBandStreams::onSettingsOpened()
{
  FSettings = FSettingsPlugin->settingsForPlugin(INBANDSTREAMS_UUID);
  FMaxBlockSize = FSettings->valueNS(SVN_MAX_BLOCK_SIZE, QString::null, DEFAULT_MAX_BLOCK_SIZE).toInt();
  FBlockSize = FSettings->valueNS(SVN_BLOCK_SIZE, QString::null, DEFAULT_BLOCK_SIZE).toInt();
  FStatnzaType = FSettings->valueNS(SVN_DATA_STANZA_TYPE, QString::null, DEFAULT_DATA_STANZA_TYPE).toInt();
}

void InBandStreams::onSettingsClosed()
{
  FSettings->setValueNS(SVN_MAX_BLOCK_SIZE, QString::null, FMaxBlockSize);
  FSettings->setValueNS(SVN_BLOCK_SIZE, QString::null, FBlockSize);
  FSettings->setValueNS(SVN_DATA_STANZA_TYPE, QString::null, FStatnzaType);
  FSettings = NULL;
}

Q_EXPORT_PLUGIN2(plg_inbandstreams, InBandStreams);
