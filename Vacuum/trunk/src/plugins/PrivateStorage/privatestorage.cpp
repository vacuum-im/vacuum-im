#include "privatestorage.h"

#include <QTimer>

PrivateStorage::PrivateStorage()
{
  FRequestId = 0;
  FChashElem = FCash.appendChild(FCash.createElement("private")).toElement();
}

PrivateStorage::~PrivateStorage()
{

}

//IPlugin
void PrivateStorage::pluginInfo(PluginInfo *APluginInfo)
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
  IPlugin *plugin = APluginManager->getPlugins("IStanzaProcessor").value(0,NULL);
  if (plugin) 
    FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());

  return FStanzaProcessor!=NULL;
}

void PrivateStorage::iqStanza(const Jid &/*AStreamJid*/, const Stanza &AStanza)
{
  if (FIq2Request.contains(AStanza.id()))
  {
    int rId = FIq2Request.take(AStanza.id());
    if (AStanza.type() == "result")
    {
      QDomElement elem = AStanza.firstElement("query",NS_JABBER_PRIVATE).firstChildElement();
      if (!elem.isNull())
      {
        QDomElement cashItem = getFromCash(elem.tagName(),elem.namespaceURI());
        if (!cashItem.isNull())
          FChashElem.removeChild(cashItem);
        FChashElem.appendChild(elem.cloneNode(true));
      }
      emit requestCompleted(rId);
    }
    else
      emit requestError(rId);
  }
}

void PrivateStorage::iqStanzaTimeOut(const QString &AId)
{
  if (FIq2Request.contains(AId))
  {
    int rId = FIq2Request.take(AId);
    emit requestTimeout(rId);
  }
}

int PrivateStorage::saveElement(const Jid &AStreamJid, const QDomElement &AElement)
{
  if (AStreamJid.isValid() && !AElement.namespaceURI().isEmpty())
  {
    QString iqId = FStanzaProcessor->newId();

    Stanza stanza("iq");
    QDomElement elem = stanza.setId(iqId).setType("set").addElement("query",NS_JABBER_PRIVATE);
    elem.appendChild(AElement.cloneNode(true));

    if (FStanzaProcessor->sendIqStanza(this,AStreamJid,stanza,60000))
    {
      FRequestId++;
      FIq2Request.insert(iqId,FRequestId);

      QDomElement elem = getFromCash(AElement.tagName(),AElement.namespaceURI());
      if (!elem.isNull())
        FChashElem.removeChild(elem);
      FChashElem.appendChild(AElement.cloneNode(true));

      return FRequestId;
    }
  }
  return -1;
}

int PrivateStorage::loadElement(const Jid &AStreamJid, const QString &ATagName, 
                                const QString &ANamespace, bool ACheckCash /*= true*/)
{
  if (AStreamJid.isValid() && !ATagName.isEmpty() && !ANamespace.isEmpty())
  {
    QDomElement elem;
    if (ACheckCash)
      elem = getFromCash(ATagName,ANamespace);
    
    if (!elem.isNull())
    {
      FRequestId++;
      FCashedRequests.append(FRequestId);
      QTimer::singleShot(0,this,SLOT(emitCashedRequests()));
      return FRequestId;
    }
    else
    {
      QString iqId = FStanzaProcessor->newId();
      Stanza stanza("iq");
      QDomElement elem = stanza.setId(iqId).setType("get").addElement("query",NS_JABBER_PRIVATE);
      elem.appendChild(FCash.createElementNS(ANamespace,ATagName));
      if (FStanzaProcessor->sendIqStanza(this,AStreamJid,stanza,30000))
      {
        FRequestId++;
        FIq2Request.insert(iqId,FRequestId);
        return FRequestId;
      }
    }
  }
  return -1;
}

int PrivateStorage::removeElement(const Jid &AStreamJid, const QString &ATagName, 
                                  const QString &ANamespace)
{
  if (AStreamJid.isValid() && !ATagName.isEmpty() && !ANamespace.isEmpty())
  {
    QString iqId = FStanzaProcessor->newId();
    Stanza stanza("iq");
    QDomElement elem = stanza.setId(iqId).setType("set").addElement("query",NS_JABBER_PRIVATE);
    elem.appendChild(FCash.createElementNS(ANamespace,ATagName));
    if (FStanzaProcessor->sendIqStanza(this,AStreamJid,stanza,30000))
    {
      QDomElement elem = getFromCash(ATagName,ANamespace);
      if (!elem.isNull())
        FChashElem.removeChild(elem);

      FRequestId++;
      FIq2Request.insert(iqId,FRequestId);
      return FRequestId;
    }
  }
  return -1;
}

QDomElement PrivateStorage::getFromCash(const QString &ATagName, const QString &ANamespace) const
{
  return FChashElem.elementsByTagNameNS(ANamespace,ATagName).at(0).toElement();
}

void PrivateStorage::emitCashedRequests()
{
  QList<int> rIds = FCashedRequests;
  FCashedRequests.clear();
  foreach (int rId, rIds)
    emit requestCompleted(rId);
}

Q_EXPORT_PLUGIN2(PrivateStoragePlugin, PrivateStorage)