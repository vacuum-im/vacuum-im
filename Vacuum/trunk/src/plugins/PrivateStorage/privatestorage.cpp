#include "privatestorage.h"

PrivateStorage::PrivateStorage()
{

}

PrivateStorage::~PrivateStorage()
{

}

//IPlugin
void PrivateStorage::pluginInfo(PluginInfo *APluginInfo)
{
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->description = tr("Store and retrive custom XML data from server");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = tr("Private Storage"); 
  APluginInfo->uid = PRIVATESTORAGE_UUID;
  APluginInfo->version = "0.1";
  APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool PrivateStorage::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
  IPlugin *plugin = APluginManager->getPlugins("IStanzaProcessor").value(0,NULL);
  if (plugin) 
    FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());

  return FStanzaProcessor!=NULL;
}

void PrivateStorage::iqStanza(const Jid &AStreamJid, const Stanza &AStanza)
{

}

void PrivateStorage::iqStanzaTimeOut(const QString &AId)
{

}

Q_EXPORT_PLUGIN2(PrivateStoragePlugin, PrivateStorage)