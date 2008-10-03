#include "privatestorage.h"

#include <QCryptographicHash>

#define PRIVATE_STORAGE_TIMEOUT       30000

PrivateStorage::PrivateStorage()
{
  FStanzaProcessor = NULL;
}

PrivateStorage::~PrivateStorage()
{

}

//IPlugin
void PrivateStorage::pluginInfo(IPluginInfo *APluginInfo)
{
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->description = tr("Store and retrieve custom XML data from server");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = tr("Private Storage"); 
  APluginInfo->uid = PRIVATESTORAGE_UUID;
  APluginInfo->version = "0.1";
  APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool PrivateStorage::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *plugin = APluginManager->getPlugins("IXmppStreams").value(0,NULL);
  if (plugin)
  {
    connect(plugin->instance(), SIGNAL(opened(IXmppStream *)), SLOT(onStreamOpened(IXmppStream *))); 
    connect(plugin->instance(), SIGNAL(closed(IXmppStream *)), SLOT(onStreamClosed(IXmppStream *))); 
  }

  plugin = APluginManager->getPlugins("IStanzaProcessor").value(0,NULL);
  if (plugin) 
    FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());

  return FStanzaProcessor!=NULL;
}

void PrivateStorage::iqStanza(const Jid &AStreamJid, const Stanza &AStanza)
{
  if (AStanza.type() == "error")
  {
    FSaveRequests.remove(AStanza.id());
    FLoadRequests.remove(AStanza.id());
    FRemoveRequests.remove(AStanza.id());
    ErrorHandler err(AStanza.element());
    emit dataError(AStanza.id(),err.message());
  }
  else if (FSaveRequests.contains(AStanza.id()))
  {
    QDomElement elem = FSaveRequests.take(AStanza.id());
    emit dataSaved(AStanza.id(),AStreamJid,elem);
  }
  else if (FLoadRequests.contains(AStanza.id()))
  {
    FLoadRequests.remove(AStanza.id());
    QDomElement dataElem = AStanza.firstElement("query",NS_JABBER_PRIVATE).firstChildElement();
    emit dataLoaded(AStanza.id(),AStreamJid,insertElement(AStreamJid,dataElem));
  }
  else if (FRemoveRequests.contains(AStanza.id()))
  {
    QDomElement dataElem = FRemoveRequests.take(AStanza.id());
    emit dataRemoved(AStanza.id(),AStreamJid,dataElem);
    removeElement(AStreamJid,dataElem.tagName(),dataElem.namespaceURI());
  }
}

void PrivateStorage::iqStanzaTimeOut(const QString &AId)
{
  FSaveRequests.remove(AId);
  FLoadRequests.remove(AId);
  FRemoveRequests.remove(AId);
  ErrorHandler err(ErrorHandler::REQUEST_TIMEOUT);
  emit dataError(AId,err.message());
}

bool PrivateStorage::hasData(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace) const
{
  return getData(AStreamJid,ATagName,ANamespace).hasChildNodes();
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
  if (AStreamJid.isValid() && !AElement.namespaceURI().isEmpty())
  {
    Stanza stanza("iq");
    stanza.setType("set").setId(FStanzaProcessor->newId());
    QDomElement elem = stanza.addElement("query",NS_JABBER_PRIVATE);
    elem.appendChild(AElement.cloneNode(true));
    if (FStanzaProcessor->sendIqStanza(this,AStreamJid,stanza,PRIVATE_STORAGE_TIMEOUT))
    {
      FSaveRequests.insert(stanza.id(),insertElement(AStreamJid,AElement));
      return stanza.id();
    }
  }
  return QString();
}

QString PrivateStorage::loadData(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace)
{
  if (AStreamJid.isValid() && !ATagName.isEmpty() && !ANamespace.isEmpty())
  {
    Stanza stanza("iq");
    stanza.setType("get").setId(FStanzaProcessor->newId());
    QDomElement elem = stanza.addElement("query",NS_JABBER_PRIVATE);
    QDomElement dataElem = elem.appendChild(stanza.createElement(ATagName,ANamespace)).toElement();
    if (FStanzaProcessor->sendIqStanza(this,AStreamJid,stanza,PRIVATE_STORAGE_TIMEOUT))
    {
      FLoadRequests.insert(stanza.id(),dataElem);
      return stanza.id();
    }
  }
  return QString();
}

QString PrivateStorage::removeData(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace)
{
  if (AStreamJid.isValid() && !ATagName.isEmpty() && !ANamespace.isEmpty())
  {
    Stanza stanza("iq");
    stanza.setType("set").setId(FStanzaProcessor->newId());
    QDomElement elem = stanza.addElement("query",NS_JABBER_PRIVATE);
    elem = elem.appendChild(stanza.createElement(ATagName,ANamespace)).toElement();
    if (FStanzaProcessor->sendIqStanza(this,AStreamJid,stanza,PRIVATE_STORAGE_TIMEOUT))
    {
      QDomElement dataElem = getData(AStreamJid,ATagName,ANamespace);
      if (dataElem.isNull())
        dataElem = insertElement(AStreamJid,elem); 
      FRemoveRequests.insert(stanza.id(),dataElem);
      return stanza.id();
    }
  }
  return QString();
}

QDomElement PrivateStorage::getStreamElement(const Jid &AStreamJid)
{
  if (!FStreamElements.contains(AStreamJid))
  {
    QDomElement elem = FStorage.appendChild(FStorage.createElement("stream")).toElement();
    FStreamElements.insert(AStreamJid,elem);
  }
  return FStreamElements.value(AStreamJid);
}

void PrivateStorage::removeStreamElement(const Jid &AStreamJid)
{
  FStreamElements.remove(AStreamJid);
}

QDomElement PrivateStorage::insertElement(const Jid &AStreamJid, const QDomElement &AElement)
{
  removeElement(AStreamJid,AElement.tagName(),AElement.namespaceURI());
  QDomElement streamElem = getStreamElement(AStreamJid);
  return streamElem.appendChild(AElement.cloneNode(true)).toElement();
}

void PrivateStorage::removeElement(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace)
{
  if (FStreamElements.contains(AStreamJid))
    FStreamElements[AStreamJid].removeChild(getData(AStreamJid,ATagName,ANamespace));
}

void PrivateStorage::onStreamOpened(IXmppStream *AXmppStream)
{
  emit storageOpened(AXmppStream->jid());
}

void PrivateStorage::onStreamClosed(IXmppStream *AXmppStream)
{
  emit storageClosed(AXmppStream->jid());
  removeStreamElement(AXmppStream->jid());
}

Q_EXPORT_PLUGIN2(PrivateStoragePlugin, PrivateStorage)