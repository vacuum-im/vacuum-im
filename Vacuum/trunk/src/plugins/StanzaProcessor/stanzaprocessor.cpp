#include <QtDebug>
#include "stanzaprocessor.h"

#include <QTime>
#include <QSet>
#include <QMultiHash>

StanzaProcessor::StanzaProcessor() 
{
  srand(QTime::currentTime().msec());
}

StanzaProcessor::~StanzaProcessor()
{

}

void StanzaProcessor::pluginInfo(IPluginInfo *APluginInfo)
{
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->description = tr("Managing stanza handlers");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = tr("Stanza Processor"); 
  APluginInfo->uid = STANZAPROCESSOR_UUID;
  APluginInfo->version = "0.1";
  APluginInfo->dependences.append("{8074A197-3B77-4bb0-9BD3-6F06D5CB8D15}"); //IXmppStreams  
}

bool StanzaProcessor::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *plugin = APluginManager->getPlugins("IXmppStreams").value(0,NULL);
  if (plugin)
  {
    FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
    if (FXmppStreams)
    {
      connect(FXmppStreams->instance(), SIGNAL(element(IXmppStream *, const QDomElement &)),
        SLOT(onStreamElement(IXmppStream *, const QDomElement &))); 
      connect(FXmppStreams->instance(), SIGNAL(jidChanged(IXmppStream *, const Jid &)),
        SLOT(onStreamJidChanged(IXmppStream *, const Jid &)));
    }
  }
  return FXmppStreams!=NULL;
}


//IStanzaProcessor
QString StanzaProcessor::newId() const
{
  return QString::number((rand()<<16)+rand(),36);
}

bool StanzaProcessor::sendStanzaIn(const Jid &AStreamJid, const Stanza &AStanza)
{
  Stanza stanza(AStanza);
  bool acceptedIn = processStanzaIn(AStreamJid,&stanza);
  bool acceptedIq = processIqStanza(AStreamJid,AStanza);
  return acceptedIn || acceptedIq;
}

bool StanzaProcessor::sendStanzaOut(const Jid &AStreamJid, const Stanza &AStanza)
{
  bool sended = false;
  Stanza stanza(AStanza);
  if (processStanzaOut(AStreamJid,&stanza))
  {
    IXmppStream *stream = FXmppStreams->getStream(AStreamJid);
    if (stream)
      sended = stream->sendStanza(stanza) > 0;
  }
  return sended;
}

bool StanzaProcessor::sendIqStanza(IIqStanzaOwner *AIqOwner, const Jid &AStreamJid, const Stanza &AStanza, int ATimeOut)
{
  if (AStanza.tagName() != "iq")
    return false;

  if (sendStanzaOut(AStreamJid,AStanza))
  {
    if (AIqOwner && !AStanza.id().isEmpty() && (AStanza.type() == "set" || AStanza.type() == "get"))
    {
      IqStanzaItem iqItem;
      if (ATimeOut > 0)
      {
        QTimer *timer = new QTimer(this);
        timer->setSingleShot(true);  
        timer->start(ATimeOut); 
        iqItem.timer = timer;
        connect(timer,SIGNAL(timeout()),SLOT(onIqStanzaTimeOut()));
      }
      else 
        iqItem.timer = NULL;
      iqItem.stanzaId = AStanza.id();
      iqItem.owner = AIqOwner;
      iqItem.streamJid = AStreamJid;
      FIqStanzaItems.insert(iqItem.stanzaId,iqItem);
      connect(AIqOwner->instance(),SIGNAL(destroyed(QObject *)),SLOT(onIqStanzaOwnerDestroyed(QObject *)));
    }
    return true;
  }
  return false;
}

int StanzaProcessor::insertHandler(IStanzaHandler *AHandler, const QString &ACondition, 
                                Direction ADirection, int APriority, const Jid &AStreamJid)
{
  if (!AHandler || ACondition.isEmpty())
    return 0;

  int hId = (rand()<<16) + rand();
  while (hId == 0 || FHandlerItems.contains(hId))
    hId++;

  HandlerItem hItem;
  hItem.handlerId = hId;
  hItem.handler = AHandler;
  hItem.conditions.append(ACondition);
  hItem.direction = ADirection;
  hItem.priority = APriority;
  hItem.streamJid = AStreamJid;

  FHandlerItems.insert(hId,hItem);
  FHandlerIdsByPriority.insertMulti(APriority,hId);
  connect(AHandler->instance(),SIGNAL(destroyed(QObject *)),SLOT(onHandlerOwnerDestroyed(QObject *)));

  emit handlerInserted(hId,AHandler);

  return hId;
}

IStanzaProcessor::Direction StanzaProcessor::handlerDirection(int AHandlerId) const
{
  if (FHandlerItems.contains(AHandlerId))
    return FHandlerItems[AHandlerId].direction;
  return IStanzaProcessor::DirectionIn;
}

int StanzaProcessor::handlerPriority(int AHandlerId) const
{
  if (FHandlerItems.contains(AHandlerId))
    return FHandlerItems[AHandlerId].priority;
  return 0;
}

Jid StanzaProcessor::handlerStreamJid(int AHandlerId) const
{
  if (FHandlerItems.contains(AHandlerId))
    return FHandlerItems[AHandlerId].streamJid;
  return Jid();
}

QStringList StanzaProcessor::handlerConditions(int AHandlerId) const
{
  if (FHandlerItems.contains(AHandlerId))
    return FHandlerItems[AHandlerId].conditions;
  return QStringList();
}

void StanzaProcessor::appendCondition(int AHandlerId, const QString &ACondition)
{
  if (!ACondition.isEmpty() && FHandlerItems.contains(AHandlerId))
    FHandlerItems[AHandlerId].conditions.append(ACondition);
}

void StanzaProcessor::removeCondition(int AHandlerId, const QString &ACondition)
{
  if (FHandlerItems.contains(AHandlerId))
    FHandlerItems[AHandlerId].conditions.removeAt(FHandlerItems[AHandlerId].conditions.lastIndexOf(ACondition));

}

void StanzaProcessor::removeHandler(int AHandlerId)
{
  if (FHandlerItems.contains(AHandlerId))
  {
    emit handlerRemoved(AHandlerId);
    FHandlerIdsByPriority.remove(FHandlerItems[AHandlerId].priority,AHandlerId);
    FHandlerItems.remove(AHandlerId);
  }
}

bool StanzaProcessor::checkCondition(const QDomElement &AElem, const QString &ACondition, const int APos)
{
  static QSet<QChar> delimiters;
  if (delimiters.isEmpty())
    delimiters <<' '<<'/'<<'\\'<<'\t'<<'\n'<<'['<<']'<<'='<<'\''<<'"'<<'@';
  
  QDomElement elem = AElem;
  
  int pos = APos;
  if (pos<ACondition.count() && ACondition[pos] == '/') 
    pos++;

  QString tagName;
  while (pos<ACondition.count() && !delimiters.contains(ACondition[pos]))
    tagName.append(ACondition[pos++]);

  if (!tagName.isEmpty() &&  elem.tagName() != tagName)
    elem = elem.nextSiblingElement(tagName);

  if (elem.isNull())  
    return false; 

  QMultiHash<QString,QString> attributes;
  while (pos<ACondition.count() && ACondition[pos] != '/')
  {
    if (ACondition[pos] == '[')
    {
      pos++;
      QString attrName = "";
      QString attrValue = "";
      while (pos<ACondition.count() && ACondition[pos] != ']')
      {
        if (ACondition[pos] == '@')
        {
          pos++;
          while (pos<ACondition.count() && !delimiters.contains(ACondition[pos]))
            attrName.append(ACondition[pos++]);
        } 
        else if (ACondition[pos] == '"' || ACondition[pos] == '\'')
        {
          QChar end = ACondition[pos++];
          while (pos<ACondition.count() && ACondition[pos] != end)
            attrValue.append(ACondition[pos++]);
          pos++;
        }
        else pos++;
      }
      if (!attrName.isEmpty())
        attributes.insertMulti(attrName,attrValue); 
      pos++;
    } 
    else pos++;
  }

  if (pos < ACondition.count() && !elem.hasChildNodes())
    return false;

  while (!elem.isNull())
  {
    int attr = 0;
    QList<QString> attrNames = attributes.keys();
    while (attr<attrNames.count() && !elem.isNull())
    {
      QString attrName = attrNames.at(attr);
      QList<QString> attrValues = attributes.values(attrName);
      bool attrBlankValue = attrValues.contains("");
      bool elemHasAttr;
      QString elemAttrValue;
      if (elem.hasAttribute(attrName))
      {
        elemHasAttr = true;
        elemAttrValue = elem.attribute(attrName);
      } 
      else if (attrName == "xmlns")
      {
        elemHasAttr = true;
        elemAttrValue = elem.namespaceURI();
      }
      else
        elemHasAttr = false;

      if (!elemHasAttr || (!attrValues.contains(elemAttrValue) && !attrBlankValue))
      {
        elem = elem.nextSiblingElement(tagName);
        attr = 0;
      }
      else attr++;
    }

    if (!elem.isNull() && pos < ACondition.count())
    {
      if (checkCondition(elem.firstChildElement(),ACondition,pos))
        return true;
      else
        elem = elem.nextSiblingElement(tagName); 
    }
    else if (!elem.isNull())
      return true;
  }
  
  return false;
}

bool StanzaProcessor::processStanzaIn(const Jid &AStreamJid, Stanza *AStanza)
{
  bool hooked = false;
  bool accepted = false;
  QList<int> checkedHandlers;

  QMapIterator<int,int> it(FHandlerIdsByPriority);
  it.toBack();
  while(!hooked && it.hasPrevious())
  {
    it.previous();
    HandlerItem &hItem = FHandlerItems[it.value()];
    if (hItem.direction == DirectionIn && (!hItem.streamJid.isValid() || hItem.streamJid == AStreamJid))
    {
      for (int i = 0; i<hItem.conditions.count(); i++)
      {
        if (checkCondition(AStanza->element(), hItem.conditions.at(i)))
        {
          hooked = hItem.handler->editStanza(it.value(),AStreamJid,AStanza,accepted);
          checkedHandlers.append(it.value()); 
          break;
        }
      }
    }
  }

  for (int i = 0; !hooked && i<checkedHandlers.count(); i++)
  {
    int hId = checkedHandlers.at(i);
    HandlerItem &hItem = FHandlerItems[hId];
    hooked = hItem.handler->readStanza(hId,AStreamJid,*AStanza,accepted);
  } 

  return accepted;
}

bool StanzaProcessor::processStanzaOut(const Jid &AStreamJid, Stanza *AStanza)
{
  bool hooked = false;
  bool accepted = false;
  QList<int> checkedHandlers;

  QMapIterator<int,int> it(FHandlerIdsByPriority);
  while(!hooked && it.hasNext())
  {
    it.next();
    HandlerItem &hItem = FHandlerItems[it.value()];
    if (hItem.direction == DirectionOut && (!hItem.streamJid.isValid() || hItem.streamJid == AStreamJid))
    {
      for (int i = 0; i<hItem.conditions.count(); i++)
      {
        if (checkCondition(AStanza->element(), hItem.conditions.at(i)))
        {
          hooked = hItem.handler->editStanza(it.value(),AStreamJid,AStanza,accepted);
          checkedHandlers.append(it.value()); 
          break;
        }
      }
    }
  }

  for (int i = 0; !hooked && i<checkedHandlers.count(); i++)
  {
    int hId = checkedHandlers.at(i);
    HandlerItem &hItem = FHandlerItems[hId];
    hooked = hItem.handler->readStanza(hId,AStreamJid,*AStanza,accepted);
  } 

  return !hooked;
}

bool StanzaProcessor::processIqStanza(const Jid &AStreamJid, const Stanza &AStanza) 
{
  if (!FIqStanzaItems.contains(AStanza.id()) || AStanza.tagName()!="iq" || (AStanza.type()!="result" && AStanza.type()!="error"))
    return false;

  IqStanzaItem &iqItem = FIqStanzaItems[AStanza.id()];
  if (iqItem.streamJid == AStreamJid) 
  {
    iqItem.owner->iqStanza(AStreamJid,AStanza);  
    removeIqStanzaItem(AStanza.id());
    return true;
  }
  return false;
}

void StanzaProcessor::removeIqStanzaItem(const QString &AStanzaId)
{
  if (FIqStanzaItems.contains(AStanzaId))
  {
    IqStanzaItem iqItem = FIqStanzaItems.take(AStanzaId);
    delete iqItem.timer;
  }
}

void StanzaProcessor::onStreamElement(IXmppStream *AXmppStream, const QDomElement &AElem)
{
  Stanza stanza(AElem);
  if (stanza.from().isEmpty())
    stanza.setFrom(AXmppStream->jid().eFull());
  stanza.setTo(AXmppStream->jid().eFull());

  if (!sendStanzaIn(AXmppStream->jid(),stanza))
    if (stanza.canReplyError())
      sendStanzaOut(AXmppStream->jid(), stanza.replyError("service-unavailable")); 
}

void StanzaProcessor::onStreamJidChanged(IXmppStream *AXmppStream, const Jid &ABefour)
{
  Jid newStreamJid = AXmppStream->jid();
  
  foreach(IqStanzaItem item, FIqStanzaItems)
    if (item.streamJid == ABefour)
      FIqStanzaItems[item.stanzaId].streamJid = newStreamJid;

  foreach(HandlerItem item, FHandlerItems)
    if (item.streamJid == ABefour)
      FHandlerItems[item.handlerId].streamJid = newStreamJid;
}

void StanzaProcessor::onIqStanzaTimeOut()
{
  QTimer *timer = qobject_cast<QTimer *>(sender());
  foreach(IqStanzaItem iqItem, FIqStanzaItems)
    if (iqItem.timer == timer)
    {
      iqItem.owner->iqStanzaTimeOut(iqItem.stanzaId);
      removeIqStanzaItem(iqItem.stanzaId);
      break;
    }
}

void StanzaProcessor::onHandlerOwnerDestroyed(QObject *AOwner)
{
  QList<int> ownerHandlers;

  foreach (HandlerItem hItem, FHandlerItems)
    if (hItem.handler->instance() == AOwner)
      ownerHandlers.append(hItem.handlerId);

  foreach(int hId, ownerHandlers)
    removeHandler(hId);
}

void StanzaProcessor::onIqStanzaOwnerDestroyed(QObject *AOwner)
{
  QList<QString> ownerIqStatzaIds;

  foreach(IqStanzaItem iqItem, FIqStanzaItems)
    if (iqItem.owner->instance() == AOwner)
      ownerIqStatzaIds.append(iqItem.stanzaId);

  foreach(QString stanzaId, ownerIqStatzaIds)
    removeIqStanzaItem(stanzaId);
}

Q_EXPORT_PLUGIN2(StanzaProcessor, StanzaProcessor)
