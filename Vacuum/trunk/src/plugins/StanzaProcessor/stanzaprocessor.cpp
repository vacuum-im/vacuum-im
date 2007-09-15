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

void StanzaProcessor::pluginInfo(PluginInfo *APluginInfo)
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
  if (!processStanzaOut(AStreamJid,&stanza))
  {
    IXmppStream *stream = FXmppStreams->getStream(AStreamJid);
    if (stream)
      sended = stream->sendStanza(stanza) > 0;
  }
  return sended;
}

bool StanzaProcessor::sendIqStanza(IIqStanzaOwner *AIqOwner,
                                   const Jid &AStreamJid, const Stanza &AStanza, 
                                   qint32 ATimeOut)
{
  if (AStanza.tagName() != "iq")
  {
    qDebug() << "Cant set trigger on not iq stanza";
    return false;
  }

  if (sendStanzaOut(AStreamJid,AStanza))
  {
    if (AIqOwner && !AStanza.id().isEmpty() && (AStanza.type() == "set" || AStanza.type() == "get"))
    {
      connect(AIqOwner->instance(),SIGNAL(destroyed(QObject *)),SLOT(onIqStanzaOwnerDestroyed(QObject *)));
      IqStanzaItem iqItem;
      if (ATimeOut > 0)
      {
        QTimer *timer = new QTimer(this);
        connect(timer,SIGNAL(timeout()),SLOT(onIqStanzaTimeOut()));
        timer->setSingleShot(true);  
        timer->start(ATimeOut); 
        iqItem.timer = timer;
      }
      else 
        iqItem.timer = 0;
      iqItem.owner = AIqOwner;
      iqItem.streamJid = AStreamJid;
      FIqStanzaItems.insert(AStanza.id(),iqItem);
    }
    return true;
  }
  return false;
}

PriorityId StanzaProcessor::setPriority(PriorityLevel ALevel, qint32 AOffset, QObject *AOwner)
{
  int index =0;
  while (index < FPriorityItems.count())
  {
    const PriorityItem *pItem = &FPriorityItems.value(FPriorities.at(index));
    if ((pItem->level > ALevel) || (pItem->level == ALevel && pItem->offset > AOffset))
      break;
    index++;  
  }

  PriorityId pId = rand()*rand();
  while (pId == 0 || FPriorities.contains(pId))
    pId++;
  FPriorities.insert(index,pId);

  if (AOwner)
    connect(AOwner,SIGNAL(destroyed(QObject *)),SLOT(onPriorityOwnerDestroyed(QObject *)));
  PriorityItem pItem;
  pItem.owner = AOwner;
  pItem.level = ALevel;
  pItem.offset = AOffset;
  FPriorityItems.insert(pId,pItem);

  return pId;
}

void StanzaProcessor::removePriority(PriorityId APriorityId)
{
  if (FPriorities.contains(APriorityId))
  {
    if (FPriorityToHandlers.contains(APriorityId))
    {
      QList<HandlerId> *handlers = &FPriorityToHandlers[APriorityId];
      while (handlers->count() > 0) 
        removeHandler(handlers->at(0));
    }
    FPriorityItems.remove(APriorityId);
    FPriorities.remove(FPriorities.indexOf(APriorityId));  
  }
}

HandlerId StanzaProcessor::setHandler(IStanzaHandler *AHandler, const QString &ACondition, 
                                      Direction ADirection, PriorityId APriorityId, 
                                      const Jid &AStreamJid)
{
  if (!AHandler || ACondition.isEmpty() || (APriorityId > 0 && !FPriorities.contains(APriorityId)))
  {
    qDebug() << "WRONG setHandler PARAMETRS";
    return 0;
  }

  HandlerId hId = rand()*rand();
  while (hId == 0 || FHandlerItems.contains(hId))
    hId++;
  
  FPriorityToHandlers[APriorityId].append(hId);

  connect(AHandler->instance(),SIGNAL(destroyed(QObject *)),SLOT(onHandlerOwnerDestroyed(QObject *)));
  HandlerItem hItem;
  hItem.handler = AHandler;
  hItem.conditions.append(ACondition);
  hItem.direction = ADirection;
  hItem.priority = APriorityId;
  hItem.streamJid = AStreamJid;
  FHandlerItems.insert(hId,hItem);
  
  int turn = 0;
  if (APriorityId > 0)
  {
    int i =0;
    while (i < FPriorities.count())
    {
      if (FPriorities.at(i) == APriorityId)
        break;
      turn += FPriorityToHandlers.value(FPriorities.at(i)).count();
      i++;
    }
  }
  else 
    turn = FHandlerTurn.count();  

  FHandlerTurn.insert(turn,hId); 

  return hId;
}

void StanzaProcessor::addCondition(HandlerId AHandlerId, const QString &ACondition)
{
  if (!ACondition.isEmpty() && FHandlerItems.contains(AHandlerId))
    FHandlerItems[AHandlerId].conditions.append(ACondition);
}

void StanzaProcessor::removeHandler(HandlerId AHandlerId)
{
  if (FHandlerItems.contains(AHandlerId))
  {
    PriorityId priority = FHandlerItems.value(AHandlerId).priority;
    QList<HandlerId> *handlers = &FPriorityToHandlers[priority];
    handlers->removeAt(handlers->indexOf(AHandlerId));  
    FHandlerItems.remove(AHandlerId);
    FHandlerTurn.remove(FHandlerTurn.indexOf(AHandlerId));  
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
  QList<HandlerId> checkedHandlers;

  int index = 0;
  while (!hooked && index < FHandlerTurn.count())
  {
    HandlerId hId = FHandlerTurn.at(index);
    HandlerItem *hItem = &FHandlerItems[hId];
    if (hItem->direction == DirectionIn && (!hItem->streamJid.isValid() || hItem->streamJid == AStreamJid))
    {
      int i = 0;
      bool checked = false;
      while (!checked && i < hItem->conditions.count())
        checked = checkCondition(AStanza->element(), hItem->conditions.at(i++));
      if (checked)
      {
        hooked = hItem->handler->editStanza(hId,AStreamJid,AStanza,accepted);
        checkedHandlers.append(hId); 
      }
    }
    index++;
  } 

  index = 0;
  while (!hooked && index < checkedHandlers.count())
  {
    HandlerId hId = checkedHandlers.at(index);
    HandlerItem *hItem = &FHandlerItems[hId];
    hooked = hItem->handler->readStanza(hId,AStreamJid,*AStanza,accepted);
    index++;
  } 

  return accepted;
}

bool StanzaProcessor::processStanzaOut(const Jid &AStreamJid, Stanza *AStanza)
{
  bool hooked = false;
  bool accepted = false;
  QList<HandlerId> checkedHandlers;

  int index = FHandlerTurn.count()-1;
  while (!hooked && index >= 0)
  {
    HandlerId hId = FHandlerTurn.at(index);
    HandlerItem *hItem = &FHandlerItems[hId];
    if (hItem->direction == DirectionOut && (!hItem->streamJid.isValid() || hItem->streamJid == AStreamJid))
    {
      int i = 0;
      bool checked = false;
      while (!checked && i < hItem->conditions.count())
        checked = checkCondition(AStanza->element(), hItem->conditions.at(i++));
      if (checked)
      {
        if (hItem->priority > 0)
          hooked = hItem->handler->editStanza(hId,AStreamJid,AStanza,accepted);
        checkedHandlers.append(hId); 
      }
    }
    index--;
  } 

  index = 0;
  while (!hooked && index < checkedHandlers.count())
  {
    HandlerId hId = checkedHandlers.at(index);
    HandlerItem *hItem = &FHandlerItems[hId];
    hooked = hItem->handler->readStanza(hId,AStreamJid,*AStanza,accepted);
    index++;
  } 

  return hooked;
}

bool StanzaProcessor::processIqStanza(const Jid &AStreamJid, const Stanza &AStanza) 
{
  if (AStanza.tagName() != "iq" || AStanza.id().isEmpty() || (AStanza.type() != "result" && AStanza.type() != "error"))
    return false;

  if (FIqStanzaItems.contains(AStanza.id()))  
  { 
    IqStanzaItem *iqItem = &FIqStanzaItems[AStanza.id()];
    if (iqItem->streamJid == AStreamJid) 
    {
      delete iqItem->timer;
      iqItem->owner->iqStanza(AStreamJid,AStanza);  
      FIqStanzaItems.remove(AStanza.id());
      return true;
    }
  }
  return false;
}

void StanzaProcessor::onStreamElement(IXmppStream *AStream, const QDomElement &AElem)
{
  Stanza stanza(AElem);
  if (stanza.from().isEmpty())
    stanza.setFrom(AStream->jid().full());
  stanza.setTo(AStream->jid().full());

  if (!sendStanzaIn(AStream->jid(),stanza) && stanza.canReplyError())
    sendStanzaOut(AStream->jid(), stanza.replyError("service-unavailable")); 
}

void StanzaProcessor::onStreamJidChanged(IXmppStream *AStream, const Jid &ABefour)
{
  Jid newStreamJid = AStream->jid();
  
  foreach(IqStanzaItem item, FIqStanzaItems)
    if (item.streamJid == ABefour)
      item.streamJid = newStreamJid;

  foreach(HandlerItem item, FHandlerItems)
    if (item.streamJid == ABefour)
      item.streamJid = newStreamJid;
}

void StanzaProcessor::onIqStanzaTimeOut()
{
  QHash<QString, IqStanzaItem>::iterator i = FIqStanzaItems.begin();
  while (i != FIqStanzaItems.end())
  {
    IqStanzaItem *iqItem = &i.value();
    if (iqItem->timer == sender())
    {
      delete (QTimer *)iqItem->timer;
      iqItem->owner->iqStanzaTimeOut(i.key());
      i = FIqStanzaItems.erase(i);
    }
    else
      i++;
  }
}

void StanzaProcessor::onPriorityOwnerDestroyed(QObject *AOwner)
{
  int i = 0;
  while (i<FPriorities.count())
  {
    if (FPriorityItems.value(FPriorities.at(i)).owner == AOwner)
      removePriority(FPriorities.at(i));
    else 
      i++;
  }
}

void StanzaProcessor::onHandlerOwnerDestroyed(QObject *AOwner)
{
  int i = 0;
  while (i<FHandlerTurn.count())
  {
    if (FHandlerItems.value(FHandlerTurn.at(i)).handler->instance() == AOwner)
      removeHandler(FHandlerTurn.at(i));
    else 
      i++;
  }
}

void StanzaProcessor::onIqStanzaOwnerDestroyed(QObject *AOwner)
{
  QHash<QString, IqStanzaItem>::iterator i = FIqStanzaItems.begin();
  while (i != FIqStanzaItems.end())
  {
    if (i.value().owner->instance() == AOwner)
    {
      delete (QTimer *)i.value().timer;
      i = FIqStanzaItems.erase(i);
    }
    else
      i++;
  }
}

Q_EXPORT_PLUGIN2(StanzaProcessor, StanzaProcessor)
