#include "messagearchiver.h"

#include <QtDebug>
#include <QDir>
#include <QStack>

#define ARCHIVE_DIRNAME       "archive"
#define COLLECTION_EXT        ".xml"
#define LOG_FILE_NAME         "archive.dat"
#define ARCHIVE_TIMEOUT       30000

#define LOG_ACTION_CREATE     "C"
#define LOG_ACTION_MODIFY     "M"
#define LOG_ACTION_REMOVE     "R"

#define RESULTSET_MAX         30

#define SHC_MESSAGE_BODY      "/message/body"
#define SHC_PREFS             "/iq[@type='set']/pref[@xmlns="NS_ARCHIVE"]"

#define ADR_STREAM_JID        Action::DR_StreamJid
#define ADR_CONTACT_JID       Action::DR_Parametr1
#define ADR_ITEM_SAVE         Action::DR_Parametr2
#define ADR_ITEM_OTR          Action::DR_Parametr3
#define ADR_METHOD_LOCAL      Action::DR_Parametr1
#define ADR_METHOD_AUTO       Action::DR_Parametr2
#define ADR_METHOD_MANUAL     Action::DR_Parametr3
#define ADR_FILTER_START      Action::DR_Parametr2
#define ADR_FILTER_END        Action::DR_Parametr3
#define ADR_GROUP_KIND        Action::DR_Parametr4

#define IN_HISTORY            "psi/history"

MessageArchiver::MessageArchiver()
{
  FPluginManager = NULL;
  FXmppStreams = NULL;
  FStanzaProcessor = NULL;
  FSettingsPlugin = NULL;
  FPrivateStorage = NULL;
  FAccountManager = NULL;
  FRostersViewPlugin = NULL;
  FDiscovery = NULL;
}

MessageArchiver::~MessageArchiver()
{

}

void MessageArchiver::pluginInfo(PluginInfo *APluginInfo)
{
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->description = tr("Archiving and retrieval of XMPP messages");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = tr("Message Archiver"); 
  APluginInfo->uid = MESSAGEARCHIVER_UUID;
  APluginInfo->version = "0.1";
  APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool MessageArchiver::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  FPluginManager = APluginManager;

  IPlugin *plugin = APluginManager->getPlugins("IXmppStreams").value(0,NULL);
  if (plugin) 
  {
    FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
    if (FXmppStreams)
    {
      connect(FXmppStreams->instance(),SIGNAL(opened(IXmppStream *)),SLOT(onStreamOpened(IXmppStream *)));
      connect(FXmppStreams->instance(),SIGNAL(closed(IXmppStream *)),SLOT(onStreamClosed(IXmppStream *)));
      connect(FXmppStreams->instance(),SIGNAL(jidChanged(IXmppStream *, const Jid &)),
        SLOT(onStreamJidChanged(IXmppStream *, const Jid &)));
    }
  }

  plugin = APluginManager->getPlugins("IStanzaProcessor").value(0,NULL);
  if (plugin) 
    FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());

  plugin = APluginManager->getPlugins("ISettingsPlugin").value(0,NULL);
  if (plugin)
    FSettingsPlugin = qobject_cast<ISettingsPlugin *>(plugin->instance());

  plugin = APluginManager->getPlugins("IPrivateStorage").value(0,NULL);
  if (plugin)
  {
    FPrivateStorage = qobject_cast<IPrivateStorage *>(plugin->instance());
    if (FPrivateStorage)
    {
      connect(FPrivateStorage->instance(),SIGNAL(dataSaved(const QString &, const Jid &, const QDomElement &)),
        SLOT(onPrivateDataChanged(const QString &, const Jid &, const QDomElement &)));
      connect(FPrivateStorage->instance(),SIGNAL(dataLoaded(const QString &, const Jid &, const QDomElement &)),
        SLOT(onPrivateDataChanged(const QString &, const Jid &, const QDomElement &)));
      connect(FPrivateStorage->instance(),SIGNAL(dataError(const QString &, const QString &)),
        SLOT(onPrivateDataError(const QString &, const QString &)));
    }
  }

  plugin = APluginManager->getPlugins("IAccountManager").value(0,NULL);
  if (plugin)
  {
    FAccountManager = qobject_cast<IAccountManager *>(plugin->instance());
    if (FAccountManager)
    {
      connect(FAccountManager->instance(),SIGNAL(hidden(IAccount *)),SLOT(onAccountHidden(IAccount *)));
    }
  }

  plugin = APluginManager->getPlugins("IRostersViewPlugin").value(0,NULL);
  if (plugin)
    FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());

  plugin = APluginManager->getPlugins("IServiceDiscovery").value(0,NULL);
  if (plugin)
    FDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());

  return FStanzaProcessor!=NULL;
}

bool MessageArchiver::initObjects()
{
  if (FSettingsPlugin)
  {
    FSettingsPlugin->insertOptionsHolder(this);
  }
  if (FRostersViewPlugin)
  {
    connect(FRostersViewPlugin->rostersView(),SIGNAL(contextMenu(IRosterIndex *, Menu *)),
      SLOT(onRostersViewContextMenu(IRosterIndex *, Menu *)));
  }
  if (FDiscovery)
  {
    registerDiscoFeatures();
  }
  return true;
}

bool MessageArchiver::readStanza(int AHandlerId, const Jid &AStreamJid, const Stanza &AStanza, bool &AAccept)
{
  if (FSHIMessageIn.value(AStreamJid) == AHandlerId)
  {
    Message message(AStanza);
    processMessage(AStreamJid,message,true);
  }
  else if (FSHIMessageOut.value(AStreamJid) == AHandlerId)
  {
    Message message(AStanza);
    processMessage(AStreamJid,message,false);
  }
  else if (FSHIPrefs.value(AStreamJid)==AHandlerId && (AStanza.from().isEmpty() || (AStreamJid && AStanza.from())))
  {
    QDomElement prefElem = AStanza.firstElement("pref",NS_ARCHIVE);
    applyArchivePrefs(AStreamJid,prefElem);

    AAccept = true;
    Stanza reply("iq");
    reply.setTo(AStanza.from()).setType("result").setId(AStanza.id());
    FStanzaProcessor->sendStanzaOut(AStreamJid,reply);
  }
  return false;
}

void MessageArchiver::iqStanza(const Jid &AStreamJid, const Stanza &AStanza)
{
  if (FPrefsLoadRequests.contains(AStanza.id()))
  {
    if (AStanza.type() == "result")
    {
      QDomElement prefElem = AStanza.firstElement("pref",NS_ARCHIVE);
      applyArchivePrefs(AStreamJid,prefElem);
    }
    else
    {
      applyArchivePrefs(AStreamJid,QDomElement());
    }
    FPrefsLoadRequests.remove(AStanza.id());
  }
  else if (FPrefsSaveRequests.contains(AStanza.id()))
  {
    FPrefsSaveRequests.removeAt(FPrefsSaveRequests.indexOf(AStanza.id()));
  }
  else if (FPrefsAutoRequests.contains(AStanza.id()))
  {
    if (isReady(AStreamJid) && AStanza.type() == "result")
    {
      bool autoSave = FPrefsAutoRequests.value(AStanza.id());
      FArchivePrefs[AStreamJid].autoSave = autoSave;
      emit archiveAutoSaveChanged(AStreamJid,autoSave);
    }
    FPrefsAutoRequests.remove(AStanza.id());
  }
  else if (FSaveRequests.contains(AStanza.id()))
  {
    if (AStanza.type() == "result")
    {
      IArchiveHeader header = FSaveRequests.value(AStanza.id());
      QDomElement chatElem = AStanza.firstElement("save",NS_ARCHIVE).firstChildElement("chat");
      header.subject = chatElem.attribute("subject",header.subject);
      header.threadId = chatElem.attribute("thread",header.threadId);
      header.version = chatElem.attribute("version",QString::number(header.version)).toInt();
      emit serverCollectionSaved(AStanza.id(),header);
    }
    FSaveRequests.remove(AStanza.id());
  }
  else if (FRetrieveRequests.contains(AStanza.id()))
  {
    if (AStanza.type() == "result")
    {
      IArchiveCollection collection;
      QDomElement chatElem = AStanza.firstElement("chat",NS_ARCHIVE);
      elementToCollection(chatElem,collection);

      QDomElement setElem = chatElem.firstChildElement("set");
      while (!setElem.isNull() && setElem.namespaceURI()!=NS_RESULTSET)
        setElem = setElem.nextSiblingElement("set");
      int count = setElem.firstChildElement("count").text().toInt();
      QString last = setElem.firstChildElement("last").text();

      emit serverCollectionLoaded(AStanza.id(),collection,last,count);
    }
    FRetrieveRequests.remove(AStanza.id());
  }
  else if (FListRequests.contains(AStanza.id()))
  {
    if (AStanza.type() == "result")
    {
      QList<IArchiveHeader> headers;
      QDomElement chatElem = AStanza.firstElement("chat",NS_ARCHIVE);
      while (!chatElem.isNull())
      {
        IArchiveHeader header;
        header.with = chatElem.attribute("with");
        header.start = DateTime(chatElem.attribute("start")).toLocal();
        header.subject = chatElem.attribute("subject");
        header.threadId = chatElem.attribute("thread");
        header.version = chatElem.attribute("version").toInt();
        headers.append(header);
        chatElem = chatElem.nextSiblingElement("chat");
      }
      QDomElement setElem = chatElem.firstChildElement("set");
      while (!setElem.isNull() && setElem.namespaceURI()!=NS_RESULTSET)
        setElem = setElem.nextSiblingElement("set");
      int count = setElem.firstChildElement("count").text().toInt();
      QString last = setElem.firstChildElement("last").text();
      emit serverHeadersLoaded(AStanza.id(),headers,last,count);
    }
    FListRequests.remove(AStanza.id());
  }
  else if (FRemoveRequests.contains(AStanza.id()))
  {
    if (AStanza.type() == "result")
    {
      IArchiveRequest request = FRemoveRequests.value(AStanza.id());
      emit serverCollectionsRemoved(AStanza.id(),request);
    }
    FRemoveRequests.remove(AStanza.id());
  }
  else if (FModifyRequests.contains(AStanza.id()))
  {
    if (AStanza.type() == "result")
    {
      IArchiveModifications modifs;
      modifs.startTime = FModifyRequests.value(AStanza.id());

      QDomElement modifsElem = AStanza.firstElement("modified",NS_ARCHIVE);
      QDomElement changeElem = modifsElem.firstChildElement();
      while (!changeElem.isNull())
      {
        IArchiveModification modif;
        modif.header.with = changeElem.attribute("with");
        modif.header.start = DateTime(changeElem.attribute("start")).toLocal();
        modif.header.version = changeElem.attribute("version").toInt();
        if (changeElem.tagName() == "changed")
        {
          modif.action =  modif.header.version == 0 ? IArchiveModification::Created : IArchiveModification::Modified;
          modifs.items.append(modif);
        }
        else if (changeElem.tagName() == "removed")
        {
          modif.action = IArchiveModification::Removed;
          modifs.items.append(modif);
        }
        changeElem = changeElem.nextSiblingElement();
      }

      QDomElement setElem = changeElem.firstChildElement("set");
      while (!setElem.isNull() && setElem.namespaceURI()!=NS_RESULTSET)
        setElem = setElem.nextSiblingElement("set");
      int count = setElem.firstChildElement("count").text().toInt();
      QString last = setElem.firstChildElement("last").text();
      
      modifs.endTime = last;
      emit serverModificationsLoaded(AStanza.id(),modifs,last,count);
    }
    FModifyRequests.remove(AStanza.id());
  }

  if (AStanza.type() == "result")
    emit requestCompleted(AStanza.id());
  else
    emit requestFailed(AStanza.id(),ErrorHandler(AStanza.element()).message());
}

void MessageArchiver::iqStanzaTimeOut(const QString &AId)
{
  if (FPrefsLoadRequests.contains(AId))
  {
    Jid streamJid = FPrefsLoadRequests.take(AId);
    applyArchivePrefs(streamJid,QDomElement());
  }
  else if (FPrefsSaveRequests.contains(AId))
  {
    FPrefsSaveRequests.removeAt(FPrefsSaveRequests.indexOf(AId));
  }
  else if (FPrefsAutoRequests.contains(AId))
  {
    FPrefsAutoRequests.remove(AId);
  }
  else if (FSaveRequests.contains(AId))
  {
    FSaveRequests.remove(AId);
  }
  else if (FRetrieveRequests.contains(AId))
  {
    FRetrieveRequests.remove(AId);
  }
  else if (FListRequests.contains(AId))
  {
    FListRequests.remove(AId);
  }
  else if (FRemoveRequests.contains(AId))
  {
    FRemoveRequests.remove(AId);
  }
  else if (FModifyRequests.contains(AId))
  {
    FModifyRequests.remove(AId);
  }
  emit requestFailed(AId,ErrorHandler(ErrorHandler::REQUEST_TIMEOUT).message());
}

QWidget *MessageArchiver::optionsWidget(const QString &ANode, int &AOrder)
{
  AOrder = OO_HISTORY;
  QStringList nodeTree = ANode.split("::",QString::SkipEmptyParts);
  if (nodeTree.count()==2 && nodeTree.at(0)==ON_HISTORY)
  {
    QString accountId = nodeTree.at(1);
    IAccount *account = FAccountManager!=NULL ? FAccountManager->accountById(accountId) : NULL;
    if (account && account->isActive() && isReady(account->streamJid()))
    {
      ArchiveOptions *widget = new ArchiveOptions(this,account->streamJid(),NULL);
      connect(FSettingsPlugin->instance(),SIGNAL(optionsDialogAccepted()),widget,SLOT(apply()));
      connect(FSettingsPlugin->instance(),SIGNAL(optionsDialogRejected()),widget,SLOT(reset()));
      return widget;
    }
  }
  return NULL;
}

bool MessageArchiver::isReady(const Jid &AStreamJid) const
{
  return FArchivePrefs.contains(AStreamJid);
}

bool MessageArchiver::isSupported(const Jid &AStreamJid) const
{
  return isReady(AStreamJid) && !FInStoragePrefs.contains(AStreamJid);
}

bool MessageArchiver::isAutoArchiving(const Jid &AStreamJid) const
{
  return isSupported(AStreamJid) && FArchivePrefs.value(AStreamJid).autoSave;
}

bool MessageArchiver::isManualArchiving(const Jid &AStreamJid) const
{
  if (isSupported(AStreamJid) && !isAutoArchiving(AStreamJid))
  {
    IArchiveStreamPrefs prefs = archivePrefs(AStreamJid);
    return prefs.methodManual != ARCHIVE_METHOD_FORBID;
  }
  return false;
}

bool MessageArchiver::isLocalArchiving(const Jid &AStreamJid) const
{
  if (isReady(AStreamJid) && !isAutoArchiving(AStreamJid))
  {
    IArchiveStreamPrefs prefs = archivePrefs(AStreamJid);
    return prefs.methodLocal!=ARCHIVE_METHOD_FORBID;
  }
  return false;
}

bool MessageArchiver::isArchivingAllowed(const Jid &AStreamJid, const Jid &AContactJid) const
{
  if (isReady(AStreamJid) && AContactJid.isValid())
  {
    IArchiveItemPrefs itemPrefs = archiveItemPrefs(AStreamJid,AContactJid);
    return itemPrefs.save!=ARCHIVE_SAVE_FALSE;
  }
  return false;
}

QString MessageArchiver::methodName(const QString &AMethod) const
{
  if (AMethod == ARCHIVE_METHOD_PREFER)
    return tr("Prefer");
  else if (AMethod == ARCHIVE_METHOD_CONCEDE)
    return tr("Concede");
  else if (AMethod == ARCHIVE_METHOD_FORBID)
    return tr("Forbid");
  else
    return tr("Unknown");
}

QString MessageArchiver::otrModeName(const QString &AOTRMode) const
{
  if (AOTRMode == ARCHIVE_OTR_APPROVE)
    return tr("Approve");
  else if (AOTRMode == ARCHIVE_OTR_CONCEDE)
    return tr("Concede");
  else if (AOTRMode == ARCHIVE_OTR_FORBID)
    return tr("Forbid");
  else if (AOTRMode == ARCHIVE_OTR_OPPOSE)
    return tr("Oppose");
  else if (AOTRMode == ARCHIVE_OTR_PREFER)
    return tr("Prefer");
  else if (AOTRMode == ARCHIVE_OTR_REQUIRE)
    return tr("Require");
  else
    return tr("Unknown");
}

QString MessageArchiver::saveModeName(const QString &ASaveMode) const
{
  if (ASaveMode == ARCHIVE_SAVE_FALSE)
    return tr("False");
  else if (ASaveMode == ARCHIVE_SAVE_BODY)
    return tr("Body");
  else if (ASaveMode == ARCHIVE_SAVE_MESSAGE)
    return tr("Message");
  else if (ASaveMode == ARCHIVE_SAVE_STREAM)
    return tr("Stream");
  else 
    return tr("Unknown");
}

QString MessageArchiver::expireName(int AExpire) const
{
  static const int oneDay = 24*60*60;
  return QString::number(AExpire / oneDay)+" days";
}

IArchiveStreamPrefs MessageArchiver::archivePrefs(const Jid &AStreamJid) const
{
  return FArchivePrefs.value(AStreamJid);
}

IArchiveItemPrefs MessageArchiver::archiveItemPrefs(const Jid &AStreamJid, const Jid &AContactJid) const
{
  IArchiveStreamPrefs prefs = FArchivePrefs.value(AStreamJid);
  QHash<Jid, IArchiveItemPrefs>::const_iterator it = prefs.itemPrefs.constBegin();
  while (it != prefs.itemPrefs.constEnd())
  {
    QString node = it.key().pNode();
    QString resource = it.key().resource();
    if (
        (it.key().pDomain() == AContactJid.pDomain()) &&
        (node.isEmpty() || node == AContactJid.pNode()) &&
        (resource.isEmpty() || resource == AContactJid.resource())
       )
    {
      return it.value();
    }
    it++;
  }
  return prefs.defaultPrefs;
}

QString MessageArchiver::setArchiveAutoSave(const Jid &AStreamJid, bool &AAuto)
{
  if (isReady(AStreamJid) && !FInStoragePrefs.contains(AStreamJid))
  {
    Stanza autoSave;
    autoSave.setType("set").setId(FStanzaProcessor->newId());
    QDomElement autoElem = autoSave.addElement("auto",NS_ARCHIVE);
    autoElem.setAttribute("save",QVariant(AAuto).toString());
    if (FStanzaProcessor->sendIqStanza(this,AStreamJid,autoSave,ARCHIVE_TIMEOUT))
    {
      FPrefsAutoRequests.insert(autoSave.id(),AAuto);
      return autoSave.id();
    }
  }
  return QString();
}

QString MessageArchiver::setArchivePrefs(const Jid &AStreamJid, const IArchiveStreamPrefs &APrefs)
{
  if (isReady(AStreamJid))
  {
    bool storage = FInStoragePrefs.contains(AStreamJid);

    IArchiveStreamPrefs oldPrefs = archivePrefs(AStreamJid);
    IArchiveStreamPrefs newPrefs = oldPrefs;

    if (!APrefs.defaultPrefs.save.isEmpty() && !APrefs.defaultPrefs.otr.isEmpty())
    {
      newPrefs.defaultPrefs.otr = APrefs.defaultPrefs.otr;
      if (newPrefs.defaultPrefs.otr == ARCHIVE_OTR_REQUIRE)
        newPrefs.defaultPrefs.save = ARCHIVE_SAVE_FALSE;
      else
        newPrefs.defaultPrefs.save = APrefs.defaultPrefs.save;
      newPrefs.defaultPrefs.expire = APrefs.defaultPrefs.expire;
    }

    if (!APrefs.methodLocal.isEmpty())
      newPrefs.methodLocal = APrefs.methodLocal;
    if (!APrefs.methodAuto.isEmpty())
      newPrefs.methodAuto = APrefs.methodAuto;
    if (!APrefs.methodManual.isEmpty())
      newPrefs.methodManual = APrefs.methodManual;

    QList<Jid> itemJids = APrefs.itemPrefs.keys();
    foreach(Jid itemJid, itemJids)
    {
      newPrefs.itemPrefs.insert(itemJid,APrefs.itemPrefs.value(itemJid));
      if (newPrefs.itemPrefs.value(itemJid).otr == ARCHIVE_OTR_REQUIRE)
        newPrefs.itemPrefs[itemJid].save = ARCHIVE_SAVE_FALSE;
    }

    Stanza save("iq");
    save.setType("set").setId(FStanzaProcessor->newId());

    QDomElement prefElem = save.addElement("pref",NS_ARCHIVE);

    bool defChanged = oldPrefs.defaultPrefs.save   != newPrefs.defaultPrefs.save ||
                      oldPrefs.defaultPrefs.otr    != newPrefs.defaultPrefs.otr  ||
                      oldPrefs.defaultPrefs.expire != newPrefs.defaultPrefs.expire;
    if (storage || defChanged)
    {
      QDomElement defElem = prefElem.appendChild(save.createElement("default")).toElement();
      defElem.setAttribute("save",newPrefs.defaultPrefs.save);
      defElem.setAttribute("otr",newPrefs.defaultPrefs.otr);
      if (newPrefs.defaultPrefs.expire > 0)
        defElem.setAttribute("expire",newPrefs.defaultPrefs.expire);
    }

    bool methodChanged = oldPrefs.methodAuto   != newPrefs.methodAuto  || 
                         oldPrefs.methodLocal  != newPrefs.methodLocal ||
                         oldPrefs.methodManual != newPrefs.methodManual;
    if (!storage && methodChanged)
    {
      QDomElement methodAuto = prefElem.appendChild(save.createElement("method")).toElement();
      methodAuto.setAttribute("type","auto");
      methodAuto.setAttribute("use",newPrefs.methodAuto);
      QDomElement methodLocal = prefElem.appendChild(save.createElement("method")).toElement();
      methodLocal.setAttribute("type","local");
      methodLocal.setAttribute("use",newPrefs.methodLocal);
      QDomElement methodManual = prefElem.appendChild(save.createElement("method")).toElement();
      methodManual.setAttribute("type","manual");
      methodManual.setAttribute("use",newPrefs.methodManual);
    }

    bool itemsChanged = false;
    itemJids = newPrefs.itemPrefs.keys();
    foreach(Jid itemJid, itemJids)
    {
      IArchiveItemPrefs newItemPrefs = newPrefs.itemPrefs.value(itemJid);
      IArchiveItemPrefs oldItemPrefs = oldPrefs.itemPrefs.value(itemJid);
      bool itemChanged = oldItemPrefs.save   != newItemPrefs.save || 
                         oldItemPrefs.otr    != newItemPrefs.otr ||
                         oldItemPrefs.expire != newItemPrefs.expire;
      bool itemDefault = newPrefs.defaultPrefs.save   == newItemPrefs.save &&
                         newPrefs.defaultPrefs.otr    == newItemPrefs.otr &&
                         newPrefs.defaultPrefs.expire == newItemPrefs.expire;
      if ( (!storage && itemChanged) || (storage && !itemDefault) )
      {
        QDomElement itemElem = prefElem.appendChild(save.createElement("item")).toElement();
        itemElem.setAttribute("jid",itemJid.eFull());
        itemElem.setAttribute("otr",newItemPrefs.otr);
        itemElem.setAttribute("save",newItemPrefs.save);
        if (newItemPrefs.expire > 0)
          itemElem.setAttribute("expire",newItemPrefs.expire);
      }
      itemsChanged |= itemChanged;
    }

    if (defChanged || methodChanged || itemsChanged)
    {
      QString requestId;
      if (storage)
        requestId = FPrivateStorage!=NULL ? FPrivateStorage->saveData(AStreamJid,prefElem) : QString();
      else if (FStanzaProcessor->sendIqStanza(this,AStreamJid,save,ARCHIVE_TIMEOUT))
        requestId = save.id();
      if (!requestId.isEmpty())
      {
        FPrefsSaveRequests.append(requestId);
        return requestId;
      }
    }
  }
  return QString();
}

IArchiveWindow *MessageArchiver::showArchiveWindow(const Jid &AStreamJid, const IArchiveFilter &AFilter, int AGroupKind, QWidget *AParent)
{
  ViewHistoryWindow *window = FArchiveWindows.value(AStreamJid);
  if (!window)
  {
    window = new ViewHistoryWindow(this,AStreamJid,AParent);
    connect(window,SIGNAL(windowDestroyed(IArchiveWindow *)),SLOT(onArchiveWindowDestroyed(IArchiveWindow *)));
    FArchiveWindows.insert(AStreamJid,window);
    emit archiveWindowCreated(window);
  }
  window->setGroupKind(AGroupKind);
  window->setFilter(AFilter);
  window->show();
  window->activateWindow();
  return window;
}

void MessageArchiver::insertArchiveHandler(IArchiveHandler *AHandler, int AOrder)
{
  connect(AHandler->instance(),SIGNAL(destroyed(QObject *)),SLOT(onArchiveHandlerDestroyed(QObject *)));
  FArchiveHandlers.insertMulti(AOrder,AHandler);
}

void MessageArchiver::removeArchiveHandler(IArchiveHandler *AHandler, int AOrder)
{
  FArchiveHandlers.remove(AOrder,AHandler);
}

bool MessageArchiver::saveNote(const Jid &AStreamJid, const Jid &AContactJid, const QString &ANote, const QString &AThreadId)
{
  if (isReady(AStreamJid) && AContactJid.isValid() && !ANote.isEmpty())
  {
    Jid with(AContactJid.node(),AContactJid.domain(),"");
    CollectionWriter *writer = findCollectionWriter(AStreamJid,with,AThreadId);
    if (!writer)
    {
      IArchiveHeader header;
      header.with = with;
      header.start = QDateTime::currentDateTime();
      header.subject = "";
      header.threadId = AThreadId;
      header.version = 0;
      writer = newCollectionWriter(AStreamJid,header);
    }
    if (writer)
      return writer->writeNote(ANote);
  }
  return false;
}

bool MessageArchiver::saveMessage(const Jid &AStreamJid, const Jid &AContactJid, const Message &AMessage)
{
  bool directionIn = AContactJid == AMessage.from();
  if (isReady(AStreamJid) && AContactJid.isValid() && !AMessage.body().isEmpty())
  {
    Jid with(AContactJid.node(),AContactJid.domain(),"");
    CollectionWriter *writer = findCollectionWriter(AStreamJid,with,AMessage.threadId());
    if (!writer)
    {
      QDateTime currentTime = QDateTime::currentDateTime();
      IArchiveHeader header;
      header.with = with;
      header.start = currentTime.addMSecs(-currentTime.time().msec());
      header.subject = AMessage.subject();
      header.threadId = AMessage.threadId();
      header.version = 0;
      writer = newCollectionWriter(AStreamJid,header);
    }
    if (writer)
      return writer->writeMessage(AMessage,archiveItemPrefs(AStreamJid,AContactJid).save,directionIn);
  }
  return false;
}

QList<Message> MessageArchiver::findLocalMessages(const Jid &AStreamJid, const IArchiveRequest &ARequest) const
{
  QList<Message> messages;
  QList<IArchiveHeader> headers = loadLocalHeaders(AStreamJid,ARequest);
  for (int i=0; i<headers.count() && messages.count()<ARequest.count; i++)
  {
    IArchiveCollection collection = loadLocalCollection(AStreamJid,headers.at(i));
    messages += collection.messages;
  }
  if (ARequest.order == Qt::AscendingOrder)
    qSort(messages.begin(),messages.end(),qLess<Message>());
  else
    qSort(messages.begin(),messages.end(),qGreater<Message>());
  return messages.mid(0,ARequest.count);
}

bool MessageArchiver::hasLocalCollection(const Jid &AStreamJid, const IArchiveHeader &AHeader) const
{
  return QFile::exists(collectionFilePath(AStreamJid,AHeader.with,AHeader.start));
}

bool MessageArchiver::saveLocalCollection(const Jid &AStreamJid, const IArchiveCollection &ACollection, bool AAppend)
{
  if (ACollection.header.with.isValid() && ACollection.header.start.isValid())
  {
    IArchiveCollection collection = loadLocalCollection(AStreamJid,ACollection.header);
    bool modified = collection.header == ACollection.header;
    collection.header = ACollection.header;
    if (AAppend)
    {
      if (!ACollection.messages.isEmpty())
      {
        collection.messages += ACollection.messages;
        qSort(collection.messages);
      }
      if (!ACollection.notes.isEmpty())
      {
        collection.notes += ACollection.notes;
      }
    }
    else
    {
      collection.messages = ACollection.messages;
      collection.notes = ACollection.notes;
    }

    QFile file(collectionFilePath(AStreamJid,ACollection.header.with, ACollection.header.start));
    if (file.open(QFile::WriteOnly|QFile::Truncate))
    {
      QDomDocument doc;
      QDomElement chatElem = doc.appendChild(doc.createElement("chat")).toElement();
      collectionToElement(collection,chatElem,archiveItemPrefs(AStreamJid,collection.header.with).save);
      file.write(doc.toByteArray(2));
      file.close();
      saveLocalModofication(AStreamJid,collection.header, modified ? LOG_ACTION_MODIFY : LOG_ACTION_CREATE);
      emit localCollectionSaved(AStreamJid,collection.header);
      return true;
    }
  }
  return false;
}

IArchiveHeader MessageArchiver::loadLocalHeader(const Jid &AStreamJid, const Jid &AWith, const QDateTime &AStart) const
{
  return loadCollectionHeader(collectionFilePath(AStreamJid,AWith,AStart));
}

QList<IArchiveHeader> MessageArchiver::loadLocalHeaders(const Jid &AStreamJid, const IArchiveRequest &ARequest) const
{
  QList<IArchiveHeader> headers;
  QStringList files = findCollectionFiles(AStreamJid,ARequest);
  for (int i=0; i<files.count() && headers.count()<ARequest.count; i++)
  {
    IArchiveHeader header = loadCollectionHeader(files.at(i));
    if (ARequest.threadId.isNull() || ARequest.threadId == header.threadId)
      headers.append(header);
  }

  if (ARequest.order == Qt::AscendingOrder)
    qSort(headers.begin(),headers.end(),qLess<IArchiveHeader>());
  else
    qSort(headers.begin(),headers.end(),qGreater<IArchiveHeader>());

  return headers;
}

IArchiveCollection MessageArchiver::loadLocalCollection(const Jid &AStreamJid, const IArchiveHeader &AHeader) const
{
  IArchiveCollection collection;
  if (AHeader.with.isValid() && AHeader.start.isValid())
  {
    QFile file(collectionFilePath(AStreamJid,AHeader.with,AHeader.start));
    if (file.open(QFile::ReadOnly))
    {
      QDomDocument doc;
      if (doc.setContent(file.readAll(),true))
        elementToCollection(doc.documentElement(),collection);
      file.close();
    }
  }
  return collection;
}

bool MessageArchiver::removeLocalCollection(const Jid &AStreamJid, const IArchiveHeader &AHeader)
{
  QString fileName = collectionFilePath(AStreamJid,AHeader.with,AHeader.start);
  if (QFile::remove(fileName))
  {
    saveLocalModofication(AStreamJid,AHeader,LOG_ACTION_REMOVE);
    emit localCollectionRemoved(AStreamJid,AHeader);
    return true;
  }
  return false;
}

IArchiveModifications MessageArchiver::loadLocalModifications(const Jid &AStreamJid, const QDateTime &AStart, int ACount) const
{
  IArchiveModifications modifs;
  modifs.startTime = AStart.toUTC();

  QString dirPath = collectionDirPath(AStreamJid,Jid());
  if (!dirPath.isEmpty() && AStart.isValid())
  {
    QFile log(dirPath+"/"+LOG_FILE_NAME);
    if (log.open(QFile::ReadOnly|QIODevice::Text))
    {
      //Поиск записи методом деления пополам
      qint64 sbound = 0;
      qint64 ebound = log.size();
      while (ebound - sbound > 1024)
      {
        log.seek((ebound + sbound)/2);
        log.readLine();
        DateTime logTime = QString::fromUtf8(log.readLine()).split(" ").value(0);
        if (!logTime.isValid())
          ebound = sbound;
        else if (logTime.toLocal() > AStart)
          ebound = log.pos();
        else
          sbound = log.pos();
      }
      log.seek(sbound);

      //Последовательный перебор записей
      while (!log.atEnd() && modifs.items.count()<ACount)
      {
        QString logLine = QString::fromUtf8(log.readLine());
        QStringList logFields = logLine.split(" ",QString::KeepEmptyParts);
        if (logFields.count() >= 6)
        {
          DateTime logTime = logFields.at(0);
          if (logTime.toLocal() > AStart)
          {
            IArchiveModification modif;
            modif.header.with = logFields.at(2);
            modif.header.start = DateTime(logFields.at(3)).toLocal();
            modif.header.version = logFields.at(4).toInt();
            modifs.endTime = logTime;
            if (logFields.at(1) == LOG_ACTION_CREATE)
            {
              modif.action = IArchiveModification::Created;
              modifs.items.append(modif);
            }
            else if (logFields.at(1) == LOG_ACTION_MODIFY)
            {
              modif.action = IArchiveModification::Modified;
              modifs.items.append(modif);
            }
            else if (logFields.at(1) == LOG_ACTION_REMOVE)
            {
              modif.action = IArchiveModification::Removed;
              modifs.items.append(modif);
            }
          }
        }
      }
    }
  }
  return modifs;
}

QDateTime MessageArchiver::replicationPoint(const Jid &AStreamJid) const
{
  if (isReady(AStreamJid))
  {
    if (isSupported(AStreamJid) && FReplicators.contains(AStreamJid))
      return FReplicators.value(AStreamJid)->replicationPoint();
    else
      return QDateTime::currentDateTime();
  }
  return QDateTime();
}

QString MessageArchiver::saveServerCollection(const Jid &AStreamJid, const IArchiveCollection &ACollection)
{
  if (FStanzaProcessor && isSupported(AStreamJid) && ACollection.header.with.isValid() && ACollection.header.start.isValid())
  {
    Stanza save("iq");
    save.setType("set").setId(FStanzaProcessor->newId());
    QDomElement chatElem = save.addElement("save",NS_ARCHIVE).appendChild(save.createElement("chat")).toElement();
    collectionToElement(ACollection, chatElem, archiveItemPrefs(AStreamJid,ACollection.header.with).save);
    if (FStanzaProcessor->sendIqStanza(this,AStreamJid,save,ARCHIVE_TIMEOUT))
    {
      FSaveRequests.insert(save.id(),ACollection.header);
      return save.id();
    }
  }
  return QString();
}

QString MessageArchiver::loadServerHeaders(const Jid AStreamJid, const IArchiveRequest &ARequest, const QString &AAfter)
{
  if (FStanzaProcessor && isSupported(AStreamJid))
  {
    Stanza request("iq");
    request.setType("get").setId(FStanzaProcessor->newId());
    QDomElement listElem = request.addElement("list",NS_ARCHIVE);
    if (ARequest.with.isValid())
      listElem.setAttribute("with",ARequest.with.eFull());
    if (ARequest.start.isValid())
      listElem.setAttribute("start",DateTime(ARequest.start).toX85UTC());
    if (ARequest.end.isValid())
      listElem.setAttribute("end",DateTime(ARequest.end).toX85UTC());
    QDomElement setElem = listElem.appendChild(request.createElement("set",NS_RESULTSET)).toElement();
    setElem.appendChild(request.createElement("max")).appendChild(request.createTextNode(QString::number(RESULTSET_MAX)));
    if (!AAfter.isEmpty())
      setElem.appendChild(request.createElement("after")).appendChild(request.createTextNode(AAfter));
    if (FStanzaProcessor->sendIqStanza(this,AStreamJid,request,ARCHIVE_TIMEOUT))
    {
      FListRequests.insert(request.id(),ARequest);
      return request.id();
    }
  }
  return QString();
}

QString MessageArchiver::loadServerCollection(const Jid AStreamJid, const IArchiveHeader &AHeader, const QString &AAfter)
{
  if (FStanzaProcessor && isSupported(AStreamJid) && AHeader.with.isValid() && AHeader.start.isValid())
  {
    Stanza retrieve("iq");
    retrieve.setType("get").setId(FStanzaProcessor->newId());
    QDomElement retrieveElem = retrieve.addElement("retrieve",NS_ARCHIVE);
    retrieveElem.setAttribute("with",AHeader.with.eFull());
    retrieveElem.setAttribute("start",DateTime(AHeader.start).toX85UTC());
    QDomElement setElem = retrieveElem.appendChild(retrieve.createElement("set",NS_RESULTSET)).toElement();
    setElem.appendChild(retrieve.createElement("max")).appendChild(retrieve.createTextNode(QString::number(RESULTSET_MAX)));
    if (!AAfter.isEmpty())
      setElem.appendChild(retrieve.createElement("after")).appendChild(retrieve.createTextNode(AAfter));
    if (FStanzaProcessor->sendIqStanza(this,AStreamJid,retrieve,ARCHIVE_TIMEOUT))
    {
      FRetrieveRequests.insert(retrieve.id(),AHeader);
      return retrieve.id();
    }
  }
  return QString();
}

QString MessageArchiver::removeServerCollections(const Jid &AStreamJid, const IArchiveRequest &ARequest, bool AOpened)
{
  if (FStanzaProcessor && isSupported(AStreamJid))
  {
    Stanza remove("iq");
    remove.setType("set").setId(FStanzaProcessor->newId());
    QDomElement removeElem = remove.addElement("remove",NS_ARCHIVE);
    if (ARequest.with.isValid())
      removeElem.setAttribute("with",ARequest.with.eFull());
    if (ARequest.start.isValid())
      removeElem.setAttribute("start",DateTime(ARequest.start).toX85UTC());
    if (ARequest.end.isValid())
      removeElem.setAttribute("end",DateTime(ARequest.end).toX85UTC());
    if (AOpened)
      removeElem.setAttribute("open",QVariant(AOpened).toString());
    if (FStanzaProcessor->sendIqStanza(this,AStreamJid,remove,ARCHIVE_TIMEOUT))
    {
      FRemoveRequests.insert(remove.id(),ARequest);
      return remove.id();
    }
  }
  return QString();
}

QString MessageArchiver::loadServerModifications(const Jid &AStreamJid, const QDateTime &AStart, int ACount, const QString &AAfter)
{
  if (FStanzaProcessor && isSupported(AStreamJid) && AStart.isValid() && ACount > 0)
  {
    Stanza modify("iq");
    modify.setType("get").setId(FStanzaProcessor->newId());
    QDomElement modifyElem = modify.addElement("modified",NS_ARCHIVE);
    modifyElem.setAttribute("start",DateTime(AStart).toX85UTC());
    QDomElement setElem = modifyElem.appendChild(modify.createElement("set",NS_RESULTSET)).toElement();
    setElem.appendChild(modify.createElement("max")).appendChild(modify.createTextNode(QString::number(ACount)));
    if (!AAfter.isEmpty())
      setElem.appendChild(modify.createElement("after")).appendChild(modify.createTextNode(AAfter));
    if (FStanzaProcessor->sendIqStanza(this,AStreamJid,modify,ARCHIVE_TIMEOUT))
    {
      if (AAfter.isEmpty())
        FModifyRequests.insert(modify.id(),AStart.toUTC());
      else
        FModifyRequests.insert(modify.id(),AAfter);
      return modify.id();
    }
  }
  return QString();
}

QString MessageArchiver::loadServerPrefs(const Jid &AStreamJid)
{
  if (FStanzaProcessor)
  {
    Stanza load("iq");
    load.setType("get").setId(FStanzaProcessor->newId());
    load.addElement("pref",NS_ARCHIVE);
    if (FStanzaProcessor->sendIqStanza(this,AStreamJid,load,ARCHIVE_TIMEOUT))
    {
      FPrefsLoadRequests.insert(load.id(),AStreamJid);
      return load.id();
    }
  }
  return QString();
}

QString MessageArchiver::loadStoragePrefs(const Jid &AStreamJid)
{
  QString requestId = FPrivateStorage!=NULL ? FPrivateStorage->loadData(AStreamJid,"pref",NS_ARCHIVE) : QString();
  if (!requestId.isEmpty())
    FPrefsLoadRequests.insert(requestId,AStreamJid);
  return requestId;
}

void MessageArchiver::applyArchivePrefs(const Jid &AStreamJid, const QDomElement &AElem)
{
  if (!AElem.hasChildNodes() && !isReady(AStreamJid) && !FInStoragePrefs.contains(AStreamJid))
  {
    FInStoragePrefs.append(AStreamJid);
    loadStoragePrefs(AStreamJid);
  }
  else
  {
    //Костыль для jabberd 1.4.3
    if (!FInStoragePrefs.contains(AStreamJid) && AElem.hasAttribute("j_private_flag"))
      FInStoragePrefs.append(AStreamJid);

    bool initPrefs = !isReady(AStreamJid);
    if (initPrefs)
    {
      IArchiveStreamPrefs defPrefs;
      defPrefs.autoSave = false;
      defPrefs.methodAuto = ARCHIVE_METHOD_CONCEDE;
      defPrefs.methodLocal = ARCHIVE_METHOD_CONCEDE;
      defPrefs.methodManual = ARCHIVE_METHOD_CONCEDE;
      defPrefs.defaultPrefs.otr = ARCHIVE_OTR_CONCEDE;
      defPrefs.defaultPrefs.save = ARCHIVE_SAVE_BODY;
      defPrefs.defaultPrefs.expire = 365*24*60*60;
      FArchivePrefs.insert(AStreamJid,defPrefs);
      openHistoryOptionsNode(AStreamJid);
    }

    IArchiveStreamPrefs &prefs = FArchivePrefs[AStreamJid];

    if (initPrefs && FInStoragePrefs.contains(AStreamJid) && !AElem.hasChildNodes())
    {
      setArchivePrefs(AStreamJid,prefs);
    }
    else
    {
      QDomElement autoElem = AElem.firstChildElement("auto");
      if (!autoElem.isNull())
      {
        prefs.autoSave = QVariant(autoElem.attribute("save")).toBool();
      }

      QDomElement defElem = AElem.firstChildElement("default");
      if (!defElem.isNull())
      {
        prefs.defaultPrefs.save = defElem.attribute("save");
        prefs.defaultPrefs.otr = defElem.attribute("otr");
        prefs.defaultPrefs.expire = defElem.attribute("expire").toInt();
      }

      QDomElement methodElem = AElem.firstChildElement("method");
      while (!methodElem.isNull())
      {
        if (methodElem.attribute("type") == "auto")
          prefs.methodAuto = methodElem.attribute("use");
        else if (methodElem.attribute("type") == "local")
          prefs.methodLocal = methodElem.attribute("use");
        else if (methodElem.attribute("type") == "manual")
          prefs.methodManual = methodElem.attribute("use");
        methodElem = methodElem.nextSiblingElement("method");
      }

      QSet<Jid> oldItemJids = prefs.itemPrefs.keys().toSet();
      QDomElement itemElem = AElem.firstChildElement("item");
      while (!itemElem.isNull())
      {
        IArchiveItemPrefs itemPrefs;
        Jid itemJid = itemElem.attribute("jid");
        itemPrefs.save = itemElem.attribute("save");
        itemPrefs.otr = itemElem.attribute("otr");
        itemPrefs.expire = itemElem.attribute("expire").toInt();
        prefs.itemPrefs.insert(itemJid,itemPrefs);
        emit archiveItemPrefsChanged(AStreamJid,itemJid,itemPrefs);
        oldItemJids -= itemJid;
        itemElem = itemElem.nextSiblingElement("item");
      }
    
      if (FInStoragePrefs.contains(AStreamJid))
      {
        foreach(Jid itemJid, oldItemJids)
        {
          prefs.itemPrefs.remove(itemJid);
          emit archiveItemPrefsChanged(AStreamJid,itemJid,prefs.defaultPrefs);
        }
      }
      else if (initPrefs)
      {
        //insertReplicator(AStreamJid);
      }

      emit archivePrefsChanged(AStreamJid,prefs);
    }
  }
}

QString MessageArchiver::collectionFileName(const DateTime &AStart) const
{
  if (AStart.isValid())
  {
    return AStart.toX85UTC().replace(":","=") + COLLECTION_EXT;
  }
  return QString();
}

QString MessageArchiver::collectionDirPath(const Jid &AStreamJid, const Jid &AWith) const
{
  if (AStreamJid.isValid())
  {
    QDir dir;
    bool noError = true;
    if (FSettingsPlugin)
      dir.setPath(FSettingsPlugin->homeDir().path());

    if (!dir.exists(ARCHIVE_DIRNAME))
      noError &= dir.mkdir(ARCHIVE_DIRNAME);
    noError &= dir.cd(ARCHIVE_DIRNAME);

    QString streamDir = Jid::encode(AStreamJid.pBare());
    if (!dir.exists(streamDir))
      noError &= dir.mkdir(streamDir);
    noError &= dir.cd(streamDir);

    if (AWith.isValid())
    {
      QString withDir = Jid::encode(AWith.pBare());
      if (!dir.exists(withDir))
        noError &= dir.mkdir(withDir);
      noError &= dir.cd(withDir);
    }

    if (noError)
      return dir.path();
  }
  return QString();
}

QString MessageArchiver::collectionFilePath(const Jid &AStreamJid, const Jid &AWith, const DateTime &AStart) const
{
  if (AStreamJid.isValid() && AWith.isValid() && AStart.isValid())
  {
    QString fileName = collectionFileName(AStart);
    QString dirPath = collectionDirPath(AStreamJid,AWith);
    if (!dirPath.isEmpty() && !fileName.isEmpty())
      return dirPath+"/"+fileName;
  }
  return QString();
}

QStringList MessageArchiver::findCollectionFiles(const Jid &AStreamJid, const IArchiveRequest &ARequest) const
{
  QStringList files;
  QString dirPath = collectionDirPath(AStreamJid,Jid());
  if (!dirPath.isEmpty())
  {
    QDir dir(dirPath);
    QStringList dirList;
    if (ARequest.with.isValid())
      dirList += Jid::encode(ARequest.with.pBare());
    else
      dirList = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    QString startName = collectionFileName(ARequest.start);
    QString endName = collectionFileName(ARequest.end);
    foreach(QString dirName, dirList)
    {
      dir.cd(dirName);
      QStringList dirFiles = dir.entryList(QStringList() << "*"COLLECTION_EXT,QDir::Files);
      if (dirFiles.count()>ARequest.count)
      {
        if (ARequest.order == Qt::AscendingOrder)
          qSort(dirFiles.begin(),dirFiles.end(),qLess<QString>());
        else
          qSort(dirFiles.begin(),dirFiles.end(),qGreater<QString>());
      }
      for(int i =0, dirCount=0; i<dirFiles.count() && dirCount<ARequest.count; i++)
      {
        const QString &file = dirFiles.at(i);
        if ( (startName.isEmpty() || startName<=file) && (endName.isEmpty() || file<=endName) )
        {
          dirCount++;
          files.append(dir.absoluteFilePath(file));
        }
      }
      dir.cdUp();
    }
    if (ARequest.order == Qt::AscendingOrder)
      qSort(files.begin(),files.end(),qLess<QString>());
    else
      qSort(files.begin(),files.end(),qGreater<QString>());
    files = files.mid(0,ARequest.count);
  }
  return files;
}

IArchiveHeader MessageArchiver::loadCollectionHeader(const QString &AFileName) const
{
  IArchiveHeader header;
  QFile file(AFileName);
  if (file.open(QFile::ReadOnly))
  {
    QXmlStreamReader reader(&file);
    while (!reader.atEnd())
    {
      reader.readNext();
      if (reader.isStartElement() && reader.qualifiedName() == "chat")
      {
        header.with = reader.attributes().value("with").toString();
        header.start = DateTime(reader.attributes().value("start").toString()).toLocal();
        header.subject = reader.attributes().value("subject").toString();
        header.threadId = reader.attributes().value("thread").toString();
        header.version = reader.attributes().value("version").toString().toInt();
        break;
      }
      else if (!reader.isStartDocument())
        break;
    }
    file.close();
  }
  return header;
}

CollectionWriter *MessageArchiver::findCollectionWriter(const Jid &AStreamJid, const Jid &AWith, const QString &AThreadId) const
{
  QList<CollectionWriter *> writers = FCollectionWriters.value(AStreamJid).values(AWith);
  foreach(CollectionWriter *writer, writers)
    if (writer->header().threadId == AThreadId)
      return writer;
  return NULL;
}

CollectionWriter *MessageArchiver::newCollectionWriter(const Jid &AStreamJid, const IArchiveHeader &AHeader)
{
  if (AHeader.with.isValid() && AHeader.start.isValid())
  {
    QString  fileName = collectionFilePath(AStreamJid,AHeader.with,AHeader.start);
    CollectionWriter *writer = new CollectionWriter(AStreamJid,fileName,AHeader,this);
    if (writer->isOpened())
    {
      FCollectionWriters[AStreamJid].insert(AHeader.with,writer);
      connect(writer,SIGNAL(destroyed(const Jid &, CollectionWriter *)),SLOT(onCollectionWriterDestroyed(const Jid &, CollectionWriter *)));
      emit localCollectionOpened(AStreamJid,AHeader);
    }
    else
    {
      delete writer;
      writer = NULL;
    }
    return writer;
  }
  return NULL;
}

void MessageArchiver::elementToCollection(const QDomElement &AChatElem, IArchiveCollection &ACollection) const
{
  ACollection.header.with = AChatElem.attribute("with");
  ACollection.header.start = DateTime(AChatElem.attribute("start")).toLocal();
  ACollection.header.subject = AChatElem.attribute("subject");
  ACollection.header.threadId = AChatElem.attribute("thread");
  ACollection.header.version = AChatElem.attribute("version").toInt();

  int secsSum = 0;
  QDomElement nodeElem = AChatElem.firstChildElement();
  while (!nodeElem.isNull())
  {
    if (nodeElem.tagName() == "to" || nodeElem.tagName() == "from")
    {
      Message message;
      
      QString resource = nodeElem.attribute("name");
      Jid contactJid(ACollection.header.with.node(),ACollection.header.with.domain(),resource);
      
      nodeElem.tagName()=="to" ? message.setTo(contactJid.eFull()) : message.setFrom(contactJid.eFull());

      if (!resource.isEmpty())
        message.setType(Message::GroupChat);

      QString utc = nodeElem.attribute("utc");
      if (utc.isEmpty())
      {
        QString secs = nodeElem.attribute("secs");
        secsSum += secs.toInt();
        message.setDateTime(ACollection.header.start.addSecs(secsSum));
      }
      else
        message.setDateTime(DateTime(utc).toLocal());

      message.setThreadId(ACollection.header.threadId);

      QDomElement childElem = nodeElem.firstChildElement();
      while (!childElem.isNull())
      {
        message.stanza().element().appendChild(childElem.cloneNode(true));
        childElem = childElem.nextSiblingElement();
      }

      ACollection.messages.append(message);
    }
    else if (nodeElem.tagName() == "note")
    {
      QString utc = nodeElem.attribute("utc");
      ACollection.notes.insertMulti(DateTime(utc).toLocal(),nodeElem.text());
    }
    nodeElem = nodeElem.nextSiblingElement();
  }
}

void MessageArchiver::collectionToElement(const IArchiveCollection &ACollection, QDomElement &AChatElem, const QString &ASaveMode) const
{
  QDomDocument &ownerDoc = AChatElem.ownerDocument();
  AChatElem.setAttribute("with",ACollection.header.with.eFull());
  AChatElem.setAttribute("start",DateTime(ACollection.header.start).toX85UTC());
  AChatElem.setAttribute("version",ACollection.header.version);
  if (!ACollection.header.subject.isEmpty())
    AChatElem.setAttribute("subject",ACollection.header.subject);
  if (!ACollection.header.threadId.isEmpty())
    AChatElem.setAttribute("thread",ACollection.header.threadId);
  
  int secSum = 0;
  bool groupChat = false;
  foreach(Message message,ACollection.messages)
  {
    Jid fromJid = message.from();
    groupChat |= message.type() == Message::GroupChat;
    if (!groupChat || !fromJid.resource().isEmpty())
    {
      bool directionIn = ACollection.header.with && message.from();
      QDomElement messageElem = AChatElem.appendChild(ownerDoc.createElement(directionIn ? "from" : "to")).toElement();

      int secs = ACollection.header.start.secsTo(message.dateTime());
      if (secs >= secSum)
      {
        messageElem.setAttribute("secs",secs-secSum);
        secSum += secs;
      }
      else
        messageElem.setAttribute("utc",DateTime(message.dateTime()).toX85UTC());

      if (groupChat)
        messageElem.setAttribute("name",fromJid.resource());

      if (ASaveMode == ARCHIVE_SAVE_MESSAGE || ASaveMode == ARCHIVE_SAVE_STREAM)
      {
        QDomElement childElem = message.stanza().element().firstChildElement();
        while (!childElem.isNull())
        {
          messageElem.appendChild(childElem.cloneNode(true));
          childElem = childElem.nextSiblingElement();
        }
      }
      else if (ASaveMode == ARCHIVE_SAVE_BODY)
      {
        messageElem.appendChild(ownerDoc.createElement("body")).appendChild(ownerDoc.createTextNode(message.body()));
      }
    }
  }

  QMultiMap<QDateTime,QString>::const_iterator it = ACollection.notes.constBegin();
  while(it != ACollection.notes.constEnd())
  {
    QDomElement noteElem = AChatElem.appendChild(ownerDoc.createElement("note")).toElement();
    noteElem.setAttribute("utc",DateTime(it.key()).toX85UTC());
    noteElem.appendChild(ownerDoc.createTextNode(it.value()));
    it++;
  }
}

bool MessageArchiver::saveLocalModofication(const Jid &AStreamJid, const IArchiveHeader &AHeader, const QString &AAction) const
{
  QString dirPath = collectionDirPath(AStreamJid,Jid());
  if (!dirPath.isEmpty() && AHeader.with.isValid() && AHeader.start.isValid())
  {
    QFile log(dirPath+"/"+LOG_FILE_NAME);
    if (log.open(QFile::WriteOnly|QFile::Append|QIODevice::Text))
    {
      QStringList logFields;
      logFields << DateTime(QDateTime::currentDateTime()).toX85UTC();
      logFields << AAction;
      logFields << AHeader.with.eFull();
      logFields << DateTime(AHeader.start).toX85UTC();
      logFields << QString::number(AHeader.version);
      logFields << "\n";
      log.write(logFields.join(" ").toUtf8());
      log.close();
      return true;
    }
  }
  return false;
}

void MessageArchiver::insertReplicator(const Jid &AStreamJid)
{
  if (isSupported(AStreamJid) && !FReplicators.contains(AStreamJid))
  {
    QString dirPath = collectionDirPath(AStreamJid,Jid());
    if (!dirPath.isEmpty())
    {
      Replicator *replicator = new Replicator(this,AStreamJid,dirPath,this);
      FReplicators.insert(AStreamJid,replicator);
    }
  }
}

void MessageArchiver::removeReplicator(const Jid &AStreamJid)
{
  if (FReplicators.contains(AStreamJid))
  {
    delete FReplicators.take(AStreamJid);
  }
}

bool MessageArchiver::prepareMessage(const Jid &AStreamJid, Message &AMessage, bool ADirectionIn)
{
  if (AMessage.type() == Message::Error)
    return false;
  if (AMessage.type()==Message::GroupChat && !ADirectionIn)
    return false;
  if (AMessage.type()==Message::GroupChat && AMessage.dateTime()<AMessage.createDateTime())
    return false;

  QString contactJid = ADirectionIn ? AMessage.from() : AMessage.to();
  if (contactJid.isEmpty())
  {
    if (ADirectionIn)
      AMessage.setFrom(AStreamJid.domain());
    else
      AMessage.setTo(AStreamJid.domain());
  }
    
  QMultiMap<int,IArchiveHandler *>::const_iterator it = FArchiveHandlers.constBegin();
  while (it != FArchiveHandlers.constEnd())
  {
    IArchiveHandler *handler = it.value();
    if (!handler->archiveMessage(it.key(),AStreamJid,AMessage,ADirectionIn))
      return false;
    it++;
  }

  if (AMessage.body().isEmpty())
    return false;

  return true;
}

bool MessageArchiver::processMessage(const Jid &AStreamJid, Message &AMessage, bool ADirectionIn)
{
  Jid contactJid = ADirectionIn ? AMessage.from() : AMessage.to();
  if (isArchivingAllowed(AStreamJid,contactJid) && (isLocalArchiving(AStreamJid) || isManualArchiving(AStreamJid)))
    if (prepareMessage(AStreamJid,AMessage,ADirectionIn))
      return saveMessage(AStreamJid,contactJid,AMessage);
  return false;
}

void MessageArchiver::openHistoryOptionsNode(const Jid &AStreamJid)
{
  IAccount *account = FAccountManager!=NULL ? FAccountManager->accountByStream(AStreamJid) : NULL;
  if (FSettingsPlugin && account)
  {
    QString node = ON_HISTORY"::"+account->accountId();
    FSettingsPlugin->openOptionsNode(node,account->name(),tr("Message archiving preferences"),QIcon());
  }
}

void MessageArchiver::closeHistoryOptionsNode(const Jid &AStreamJid)
{
  IAccount *account = FAccountManager!=NULL ? FAccountManager->accountByStream(AStreamJid) : NULL;
  if (FSettingsPlugin && account)
  {
    QString node = ON_HISTORY"::"+account->accountId();
    FSettingsPlugin->closeOptionsNode(node);
  }
}

Menu *MessageArchiver::createContextMenu(const Jid &AStreamJid, const Jid &AContactJid, QWidget *AParent) const
{
  bool isStreamMenu = AStreamJid==AContactJid;
  IArchiveItemPrefs itemPrefs = isStreamMenu ? archivePrefs(AStreamJid).defaultPrefs : archiveItemPrefs(AStreamJid,AContactJid);

  Menu *menu = new Menu(AParent);
  menu->setIcon(SYSTEM_ICONSETFILE,IN_HISTORY);
  menu->setTitle(tr("History"));

  Action *action = new Action(menu);
  action->setText(tr("View History"));
  action->setData(ADR_STREAM_JID,AStreamJid.full());
  action->setData(ADR_FILTER_START,QDateTime::currentDateTime().addMonths(-1));
  if (!isStreamMenu)
  {
    action->setData(ADR_CONTACT_JID,AContactJid.full());
    action->setData(ADR_GROUP_KIND,IArchiveWindow::GK_NO_GROUPS);
  }
  else
    action->setData(ADR_GROUP_KIND,IArchiveWindow::GK_CONTACT);
  connect(action,SIGNAL(triggered(bool)),SLOT(onShowArchiveWindowAction(bool)));
  menu->addAction(action,AG_DEFAULT,false);

  if (isReady(AStreamJid))
  {
    if (isStreamMenu && isSupported(AStreamJid))
    {
      IArchiveStreamPrefs prefs = archivePrefs(AStreamJid);
      Menu *autoMenu = new Menu(menu);
      autoMenu->setTitle(tr("Set Auto Method"));
        action = new Action(autoMenu);
        action->setData(ADR_STREAM_JID,AStreamJid.full());
        action->setData(ADR_METHOD_AUTO,ARCHIVE_METHOD_CONCEDE);
        action->setCheckable(true);
        action->setChecked(prefs.methodAuto == ARCHIVE_METHOD_CONCEDE);
        action->setText(methodName(ARCHIVE_METHOD_CONCEDE));
        connect(action,SIGNAL(triggered(bool)),SLOT(onSetMethodAction(bool)));
        autoMenu->addAction(action,AG_DEFAULT,false);

        action = new Action(autoMenu);
        action->setData(ADR_STREAM_JID,AStreamJid.full());
        action->setData(ADR_METHOD_AUTO,ARCHIVE_METHOD_FORBID);
        action->setCheckable(true);
        action->setChecked(prefs.methodAuto == ARCHIVE_METHOD_FORBID);
        action->setText(methodName(ARCHIVE_METHOD_FORBID));
        connect(action,SIGNAL(triggered(bool)),SLOT(onSetMethodAction(bool)));
        autoMenu->addAction(action,AG_DEFAULT,false);

        action = new Action(autoMenu);
        action->setData(ADR_STREAM_JID,AStreamJid.full());
        action->setData(ADR_METHOD_AUTO,ARCHIVE_METHOD_PREFER);
        action->setCheckable(true);
        action->setChecked(prefs.methodAuto == ARCHIVE_METHOD_PREFER);
        action->setText(methodName(ARCHIVE_METHOD_PREFER));
        connect(action,SIGNAL(triggered(bool)),SLOT(onSetMethodAction(bool)));
        autoMenu->addAction(action,AG_DEFAULT,false);
      menu->addAction(autoMenu->menuAction(),AG_DEFAULT+500,false);

      Menu *manualMenu = new Menu(menu);
      manualMenu->setTitle(tr("Set Manual Method"));
        action = new Action(autoMenu);
        action->setData(ADR_STREAM_JID,AStreamJid.full());
        action->setData(ADR_METHOD_MANUAL,ARCHIVE_METHOD_CONCEDE);
        action->setCheckable(true);
        action->setChecked(prefs.methodManual == ARCHIVE_METHOD_CONCEDE);
        action->setText(methodName(ARCHIVE_METHOD_CONCEDE));
        connect(action,SIGNAL(triggered(bool)),SLOT(onSetMethodAction(bool)));
        manualMenu->addAction(action,AG_DEFAULT,false);

        action = new Action(autoMenu);
        action->setData(ADR_STREAM_JID,AStreamJid.full());
        action->setData(ADR_METHOD_MANUAL,ARCHIVE_METHOD_FORBID);
        action->setCheckable(true);
        action->setChecked(prefs.methodManual == ARCHIVE_METHOD_FORBID);
        action->setText(methodName(ARCHIVE_METHOD_FORBID));
        connect(action,SIGNAL(triggered(bool)),SLOT(onSetMethodAction(bool)));
        manualMenu->addAction(action,AG_DEFAULT,false);

        action = new Action(autoMenu);
        action->setData(ADR_STREAM_JID,AStreamJid.full());
        action->setData(ADR_METHOD_MANUAL,ARCHIVE_METHOD_PREFER);
        action->setCheckable(true);
        action->setChecked(prefs.methodManual == ARCHIVE_METHOD_PREFER);
        action->setText(methodName(ARCHIVE_METHOD_PREFER));
        connect(action,SIGNAL(triggered(bool)),SLOT(onSetMethodAction(bool)));
        manualMenu->addAction(action,AG_DEFAULT,false);
      menu->addAction(manualMenu->menuAction(),AG_DEFAULT+500,false);

      Menu *localMenu = new Menu(menu);
      localMenu->setTitle(tr("Set Local Method"));
        action = new Action(localMenu);
        action->setData(ADR_STREAM_JID,AStreamJid.full());
        action->setData(ADR_METHOD_LOCAL,ARCHIVE_METHOD_CONCEDE);
        action->setCheckable(true);
        action->setChecked(prefs.methodLocal == ARCHIVE_METHOD_CONCEDE);
        action->setText(methodName(ARCHIVE_METHOD_CONCEDE));
        connect(action,SIGNAL(triggered(bool)),SLOT(onSetMethodAction(bool)));
        localMenu->addAction(action,AG_DEFAULT,false);

        action = new Action(localMenu);
        action->setData(ADR_STREAM_JID,AStreamJid.full());
        action->setData(ADR_METHOD_LOCAL,ARCHIVE_METHOD_FORBID);
        action->setCheckable(true);
        action->setChecked(prefs.methodLocal == ARCHIVE_METHOD_FORBID);
        action->setText(methodName(ARCHIVE_METHOD_FORBID));
        connect(action,SIGNAL(triggered(bool)),SLOT(onSetMethodAction(bool)));
        localMenu->addAction(action,AG_DEFAULT,false);

        action = new Action(localMenu);
        action->setData(ADR_STREAM_JID,AStreamJid.full());
        action->setData(ADR_METHOD_LOCAL,ARCHIVE_METHOD_PREFER);
        action->setCheckable(true);
        action->setChecked(prefs.methodLocal == ARCHIVE_METHOD_PREFER);
        action->setText(methodName(ARCHIVE_METHOD_PREFER));
        connect(action,SIGNAL(triggered(bool)),SLOT(onSetMethodAction(bool)));
        localMenu->addAction(action,AG_DEFAULT,false);
      menu->addAction(localMenu->menuAction(),AG_DEFAULT+500,false);
    }

    Menu *otrMenu = new Menu(menu);
    otrMenu->setTitle(tr("Set OTR mode"));
      action = new Action(otrMenu);
      action->setData(ADR_STREAM_JID,AStreamJid.full());
      action->setData(ADR_CONTACT_JID,AContactJid.full());
      action->setData(ADR_ITEM_SAVE,itemPrefs.save);
      action->setData(ADR_ITEM_OTR,ARCHIVE_OTR_APPROVE);
      action->setCheckable(true);
      action->setChecked(itemPrefs.otr == ARCHIVE_OTR_APPROVE);
      action->setText(otrModeName(ARCHIVE_OTR_APPROVE));
      connect(action,SIGNAL(triggered(bool)),SLOT(onSetItemPrefsAction(bool)));
      otrMenu->addAction(action,AG_DEFAULT,false);

      action = new Action(otrMenu);
      action->setData(ADR_STREAM_JID,AStreamJid.full());
      action->setData(ADR_CONTACT_JID,AContactJid.full());
      action->setData(ADR_ITEM_SAVE,itemPrefs.save);
      action->setData(ADR_ITEM_OTR,ARCHIVE_OTR_CONCEDE);
      action->setCheckable(true);
      action->setChecked(itemPrefs.otr == ARCHIVE_OTR_CONCEDE);
      action->setText(otrModeName(ARCHIVE_OTR_CONCEDE));
      connect(action,SIGNAL(triggered(bool)),SLOT(onSetItemPrefsAction(bool)));
      otrMenu->addAction(action,AG_DEFAULT,false);

      action = new Action(otrMenu);
      action->setData(ADR_STREAM_JID,AStreamJid.full());
      action->setData(ADR_CONTACT_JID,AContactJid.full());
      action->setData(ADR_ITEM_SAVE,itemPrefs.save);
      action->setData(ADR_ITEM_OTR,ARCHIVE_OTR_FORBID);
      action->setCheckable(true);
      action->setChecked(itemPrefs.otr == ARCHIVE_OTR_FORBID);
      action->setText(otrModeName(ARCHIVE_OTR_FORBID));
      connect(action,SIGNAL(triggered(bool)),SLOT(onSetItemPrefsAction(bool)));
      otrMenu->addAction(action,AG_DEFAULT,false);

      action = new Action(otrMenu);
      action->setData(ADR_STREAM_JID,AStreamJid.full());
      action->setData(ADR_CONTACT_JID,AContactJid.full());
      action->setData(ADR_ITEM_SAVE,itemPrefs.save);
      action->setData(ADR_ITEM_OTR,ARCHIVE_OTR_OPPOSE);
      action->setCheckable(true);
      action->setChecked(itemPrefs.otr == ARCHIVE_OTR_OPPOSE);
      action->setText(otrModeName(ARCHIVE_OTR_OPPOSE));
      connect(action,SIGNAL(triggered(bool)),SLOT(onSetItemPrefsAction(bool)));
      otrMenu->addAction(action,AG_DEFAULT,false);

      action = new Action(otrMenu);
      action->setData(ADR_STREAM_JID,AStreamJid.full());
      action->setData(ADR_CONTACT_JID,AContactJid.full());
      action->setData(ADR_ITEM_SAVE,itemPrefs.save);
      action->setData(ADR_ITEM_OTR,ARCHIVE_OTR_PREFER);
      action->setCheckable(true);
      action->setChecked(itemPrefs.otr == ARCHIVE_OTR_PREFER);
      action->setText(otrModeName(ARCHIVE_OTR_PREFER));
      connect(action,SIGNAL(triggered(bool)),SLOT(onSetItemPrefsAction(bool)));
      otrMenu->addAction(action,AG_DEFAULT,false);

      action = new Action(otrMenu);
      action->setData(ADR_STREAM_JID,AStreamJid.full());
      action->setData(ADR_CONTACT_JID,AContactJid.full());
      action->setData(ADR_ITEM_SAVE,ARCHIVE_SAVE_FALSE);
      action->setData(ADR_ITEM_OTR,ARCHIVE_OTR_REQUIRE);
      action->setCheckable(true);
      action->setChecked(itemPrefs.otr == ARCHIVE_OTR_REQUIRE);
      action->setText(otrModeName(ARCHIVE_OTR_REQUIRE));
      connect(action,SIGNAL(triggered(bool)),SLOT(onSetItemPrefsAction(bool)));
      otrMenu->addAction(action,AG_DEFAULT,false);
    menu->addAction(otrMenu->menuAction(),AG_DEFAULT+500,false);

    Menu *saveMenu = new Menu(menu);
      saveMenu->setTitle(tr("Set Save mode"));
      action = new Action(saveMenu);
      action->setData(ADR_STREAM_JID,AStreamJid.full());
      action->setData(ADR_CONTACT_JID,AContactJid.full());
      action->setData(ADR_ITEM_SAVE,ARCHIVE_SAVE_FALSE);
      action->setData(ADR_ITEM_OTR,itemPrefs.otr);
      action->setCheckable(true);
      action->setChecked(itemPrefs.save == ARCHIVE_SAVE_FALSE);
      action->setText(saveModeName(ARCHIVE_SAVE_FALSE));
      connect(action,SIGNAL(triggered(bool)),SLOT(onSetItemPrefsAction(bool)));
      saveMenu->addAction(action,AG_DEFAULT,false);

      action = new Action(saveMenu);
      action->setData(ADR_STREAM_JID,AStreamJid.full());
      action->setData(ADR_CONTACT_JID,AContactJid.full());
      action->setData(ADR_ITEM_SAVE,ARCHIVE_SAVE_BODY);
      action->setData(ADR_ITEM_OTR,itemPrefs.otr);
      action->setCheckable(true);
      action->setChecked(itemPrefs.save == ARCHIVE_SAVE_BODY);
      action->setText(saveModeName(ARCHIVE_SAVE_BODY));
      connect(action,SIGNAL(triggered(bool)),SLOT(onSetItemPrefsAction(bool)));
      saveMenu->addAction(action,AG_DEFAULT,false);

      action = new Action(saveMenu);
      action->setData(ADR_STREAM_JID,AStreamJid.full());
      action->setData(ADR_CONTACT_JID,AContactJid.full());
      action->setData(ADR_ITEM_SAVE,ARCHIVE_SAVE_MESSAGE);
      action->setData(ADR_ITEM_OTR,itemPrefs.otr);
      action->setCheckable(true);
      action->setChecked(itemPrefs.save == ARCHIVE_SAVE_MESSAGE);
      action->setText(saveModeName(ARCHIVE_SAVE_MESSAGE));
      connect(action,SIGNAL(triggered(bool)),SLOT(onSetItemPrefsAction(bool)));
      saveMenu->addAction(action,AG_DEFAULT,false);
      saveMenu->setEnabled(itemPrefs.otr != ARCHIVE_OTR_REQUIRE);
    menu->addAction(saveMenu->menuAction(),AG_DEFAULT+500,false);

    if (isStreamMenu)
    {
      action = new Action(menu);
      action->setText(tr("Options..."));
      action->setData(ADR_STREAM_JID,AStreamJid.full());
      connect(action,SIGNAL(triggered(bool)),SLOT(onOpenHistoryOptionsAction(bool)));
      menu->addAction(action,AG_DEFAULT+500,false);
    }
  }
  return menu;
}

void MessageArchiver::registerDiscoFeatures()
{
  IDiscoFeature dfeature;

  dfeature.active = false;
  dfeature.icon = Skin::getSkinIconset(SYSTEM_ICONSETFILE)->iconByName(IN_HISTORY);
  dfeature.var = NS_ARCHIVE;
  dfeature.name = tr("Message Archiving");
  dfeature.description = tr("Archiving and retrieval of messages");
  FDiscovery->insertDiscoFeature(dfeature);

  dfeature.var = NS_ARCHIVE_AUTO;
  dfeature.name = tr("Message Archiving: Auto archiving");
  dfeature.description = tr("Automatically archiving of messages on server");
  FDiscovery->insertDiscoFeature(dfeature);

  dfeature.var = NS_ARCHIVE_ENCRYPT;
  dfeature.name = tr("Message Archiving: Encrypted messages");
  dfeature.description = tr("Archiving of encrypted messages on server");
  FDiscovery->insertDiscoFeature(dfeature);

  dfeature.var = NS_ARCHIVE_MANAGE;
  dfeature.name = tr("Message Archiving: Mange messages");
  dfeature.description = tr("Retrieving and removing messages on server");
  FDiscovery->insertDiscoFeature(dfeature);

  dfeature.var = NS_ARCHIVE_MANUAL;
  dfeature.name = tr("Message Archiving: Manual archiving");
  dfeature.description = tr("Manually upload messages on server");
  FDiscovery->insertDiscoFeature(dfeature);

  dfeature.var = NS_ARCHIVE_PREF;
  dfeature.name = tr("Message Archiving: Preferences");
  dfeature.description = tr("Storing archive preferences on server");
  FDiscovery->insertDiscoFeature(dfeature);
}

void MessageArchiver::onStreamOpened(IXmppStream *AXmppStream)
{
  if (FStanzaProcessor)
  {
    int handler = FStanzaProcessor->insertHandler(this,SHC_PREFS,IStanzaProcessor::DirectionIn,SHP_DEFAULT);
    FSHIPrefs.insert(AXmppStream->jid(),handler);
    
    handler = FStanzaProcessor->insertHandler(this,SHC_MESSAGE_BODY,IStanzaProcessor::DirectionIn,SHP_DEFAULT);
    FSHIMessageIn.insert(AXmppStream->jid(),handler);

    handler = FStanzaProcessor->insertHandler(this,SHC_MESSAGE_BODY,IStanzaProcessor::DirectionOut,SHP_DEFAULT);
    FSHIMessageOut.insert(AXmppStream->jid(),handler);
  }
  loadServerPrefs(AXmppStream->jid());
}

void MessageArchiver::onStreamClosed(IXmppStream *AXmppStream)
{
  QList<CollectionWriter *> writers = FCollectionWriters.value(AXmppStream->jid()).values();
  qDeleteAll(writers);
  FCollectionWriters.remove(AXmppStream->jid());

  if (FStanzaProcessor)
  {
    int handler = FSHIPrefs.take(AXmppStream->jid());
    FStanzaProcessor->removeHandler(handler);

    handler = FSHIMessageIn.take(AXmppStream->jid());
    FStanzaProcessor->removeHandler(handler);

    handler = FSHIMessageOut.take(AXmppStream->jid());
    FStanzaProcessor->removeHandler(handler);
  }

  removeReplicator(AXmppStream->jid());
  closeHistoryOptionsNode(AXmppStream->jid());
  FArchivePrefs.remove(AXmppStream->jid());
  FInStoragePrefs.removeAt(FInStoragePrefs.indexOf(AXmppStream->jid()));
}

void MessageArchiver::onAccountHidden(IAccount *AAccount)
{
  if (FArchiveWindows.contains(AAccount->streamJid()))
    delete FArchiveWindows.take(AAccount->streamJid());
}

void MessageArchiver::onStreamJidChanged(IXmppStream * /*AXmppStream*/, const Jid &ABefour)
{
  if (FArchiveWindows.contains(ABefour))
    delete FArchiveWindows.take(ABefour);
}

void MessageArchiver::onPrivateDataChanged(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement)
{
  if (FPrefsLoadRequests.contains(AId))
  {
    FPrefsLoadRequests.remove(AId);
    applyArchivePrefs(AStreamJid,AElement);
    emit requestCompleted(AId);
  }
  else if (FPrefsSaveRequests.contains(AId))
  {
    applyArchivePrefs(AStreamJid,AElement);
    FPrefsSaveRequests.removeAt(FPrefsSaveRequests.indexOf(AId));
    emit requestCompleted(AId);
  }
}

void MessageArchiver::onPrivateDataError(const QString &AId, const QString &AError)
{
  if (FPrefsLoadRequests.contains(AId))
  {
    Jid streamJid = FPrefsLoadRequests.take(AId);
    applyArchivePrefs(streamJid,QDomElement());
    emit requestFailed(AId,AError);
  }
  else if (FPrefsSaveRequests.contains(AId))
  {
    FPrefsSaveRequests.removeAt(FPrefsSaveRequests.indexOf(AId));
    emit requestFailed(AId,AError);
  }
}

void MessageArchiver::onCollectionWriterDestroyed(const Jid &AStreamJid, CollectionWriter *AWriter)
{
  FCollectionWriters[AStreamJid].remove(AWriter->header().with,AWriter);
  if (AWriter->recordsCount() > 0)
  {
    saveLocalModofication(AStreamJid,AWriter->header(),LOG_ACTION_CREATE);
    emit localCollectionSaved(AStreamJid,AWriter->header());
  }
}

void MessageArchiver::onRostersViewContextMenu(IRosterIndex *AIndex, Menu *AMenu)
{
  if (AIndex->type()==RIT_StreamRoot || AIndex->type()==RIT_Contact || AIndex->type()==RIT_Agent)
  {
    Jid streamJid = AIndex->data(RDR_StreamJid).toString();
    Jid contactJid = AIndex->data(RDR_Jid).toString();
    Menu *menu = createContextMenu(streamJid,AIndex->type()==RIT_StreamRoot ? contactJid : contactJid.bare(),AMenu);
    if (menu)
    {
      AMenu->addAction(menu->menuAction(),AG_DEFAULT,true);
    }
  }
}

void MessageArchiver::onSetMethodAction(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    IArchiveStreamPrefs prefs;
    Jid streamJid = action->data(ADR_STREAM_JID).toString();
    prefs.methodLocal = action->data(ADR_METHOD_LOCAL).toString();
    prefs.methodAuto = action->data(ADR_METHOD_AUTO).toString();
    prefs.methodManual = action->data(ADR_METHOD_MANUAL).toString();
    setArchivePrefs(streamJid,prefs);
  }
}

void MessageArchiver::onSetItemPrefsAction(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    Jid streamJid = action->data(ADR_STREAM_JID).toString();
    Jid contactJid = action->data(ADR_CONTACT_JID).toString();
    IArchiveStreamPrefs prefs = archivePrefs(streamJid);
    if (streamJid == contactJid)
    {
      prefs.defaultPrefs.save = action->data(ADR_ITEM_SAVE).toString();
      prefs.defaultPrefs.otr = action->data(ADR_ITEM_OTR).toString();
    }
    else
    {
      prefs.itemPrefs[contactJid] = archiveItemPrefs(streamJid,contactJid);
      prefs.itemPrefs[contactJid].save = action->data(ADR_ITEM_SAVE).toString();
      prefs.itemPrefs[contactJid].otr = action->data(ADR_ITEM_OTR).toString();
    }
    setArchivePrefs(streamJid,prefs);
  }
}

void MessageArchiver::onShowArchiveWindowAction(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    IArchiveFilter filter;
    filter.with = action->data(ADR_CONTACT_JID).toString();
    filter.start = action->data(ADR_FILTER_START).toDateTime();
    filter.end = action->data(ADR_FILTER_END).toDateTime();
    int groupKind = action->data(ADR_GROUP_KIND).toInt();
    Jid streamJid = action->data(ADR_STREAM_JID).toString();
    showArchiveWindow(streamJid,filter,groupKind);
  }
}

void MessageArchiver::onOpenHistoryOptionsAction( bool )
{
  Action *action = qobject_cast<Action *>(sender());
  if (FSettingsPlugin && FAccountManager && action)
  {
    Jid streamJid = action->data(ADR_STREAM_JID).toString();
    IAccount *account = FAccountManager->accountByStream(streamJid);
    if (account)
      FSettingsPlugin->openOptionsDialog(ON_HISTORY"::"+account->accountId());
  }
}

void MessageArchiver::onOptionsDialogAccepted()
{
  emit optionsAccepted();
}

void MessageArchiver::onOptionsDialogRejected()
{
  emit optionsRejected();
}

void MessageArchiver::onArchiveHandlerDestroyed(QObject *AHandler)
{
  QList<int> orders = FArchiveHandlers.keys((IArchiveHandler *)AHandler);
  foreach(int order, orders)
  {
    removeArchiveHandler((IArchiveHandler *)AHandler,order);
  }
}

void MessageArchiver::onArchiveWindowDestroyed(IArchiveWindow *AWindow)
{
  FArchiveWindows.remove(AWindow->streamJid());
  emit archiveWindowDestroyed(AWindow);
}

Q_EXPORT_PLUGIN2(MessageArchiverPlugin, MessageArchiver)
