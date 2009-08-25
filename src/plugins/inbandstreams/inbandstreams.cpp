#include "inbandstreams.h"

InBandStreams::InBandStreams()
{
  FFileManager = NULL;
  FDataManager = NULL;
}

InBandStreams::~InBandStreams()
{

}

void InBandStreams::pluginInfo(IPluginInfo *APluginInfo)
{
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->description = tr("Enables any two entities to establish a one-to-one in-band bytestream");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = tr("In-Band Bytestreams");
  APluginInfo->uid = INBANDSTREAMS_UUID;
  APluginInfo->version = "0.1";
}

bool InBandStreams::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *plugin = APluginManager->getPlugins("IFileStreamsManager").value(0,NULL);
  if (plugin)
  {
    FFileManager = qobject_cast<IFileStreamsManager *>(plugin->instance());
  }

  plugin = APluginManager->getPlugins("IDataStreamsManager").value(0,NULL);
  if (plugin)
  {
    FDataManager = qobject_cast<IDataStreamsManager *>(plugin->instance());
  }

  return true;
}

bool InBandStreams::initObjects()
{
  if (FDataManager)
  {
    FDataManager->insertMethod(this);
  }
  if (FFileManager)
  {
    FFileManager->insertStreamMethod(NS_INBAND_BYTESTREAMS);
  }
  return true;
}

QString InBandStreams::methodNS() const
{
  return NS_INBAND_BYTESTREAMS;
}

QString InBandStreams::methodName() const
{
  return tr("In-Band Bytestreams");
}

QString InBandStreams::methodDescription() const
{
  return tr("Data is broken down into smaller chunks and transported in-band over XMPP");
}

IDataStreamSocket *InBandStreams::dataStreamSocket(const QString &ASocketId, const Jid &AStreamJid, const Jid &AContactJid, 
                                                   IDataStreamSocket::StreamKind AKind, QObject *AParent)
{
  return NULL;
}

void InBandStreams::loadSettings(IDataStreamSocket *ASocket, const QString &ASettingsNS)
{

}

void InBandStreams::saveSettings(IDataStreamSocket *ASocket, const QString &ASettingsNS)
{

}

Q_EXPORT_PLUGIN2(InBandStreamsPlugin, InBandStreams);
