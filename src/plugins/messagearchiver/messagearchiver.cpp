#include "messagearchiver.h"

#include <QDir>
#include <QStack>

#define ARCHIVE_DIRNAME       "archive"
#define COLLECTION_EXT        ".xml"
#define LOG_FILE_NAME         "archive.dat"
#define GATEWAY_FILE_NAME     "gateways.dat"
#define ARCHIVE_TIMEOUT       30000

#define CATEGORY_GATEWAY      "gateway"

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

#define AVN_REPLICATION       "archiveReplication"

#define SFP_LOGGING           "logging"
#define SFV_MAY_LOGGING       "may"
#define SFV_MUSTNOT_LOGGING   "mustnot"

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
  FDataForms = NULL;
  FMessenger = NULL;
  FSessionNegotioation = NULL;
  FRosterPlugin = NULL;
}

MessageArchiver::~MessageArchiver()
{

}

void MessageArchiver::pluginInfo(IPluginInfo *APluginInfo)
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
  {
    FDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());
    if (FDiscovery)
    {
      connect(FDiscovery->instance(),SIGNAL(discoInfoReceived(const IDiscoInfo &)),SLOT(onDiscoInfoReceived(const IDiscoInfo &)));
    }
  }

  plugin = APluginManager->getPlugins("IDataForms").value(0,NULL);
  if (plugin)
    FDataForms = qobject_cast<IDataForms *>(plugin->instance());

  plugin = APluginManager->getPlugins("IMessenger").value(0,NULL);
  if (plugin)
    FMessenger = qobject_cast<IMessenger *>(plugin->instance());

  plugin = APluginManager->getPlugins("ISessionNegotiation").value(0,NULL);
  if (plugin)
  {
    FSessionNegotioation = qobject_cast<ISessionNegotiation *>(plugin->instance());
    if (FSessionNegotioation)
    {
      connect(FSessionNegotioation->instance(),SIGNAL(sessionActivated(const IStanzaSession &)),
        SLOT(onStanzaSessionActivated(const IStanzaSession &)));
      connect(FSessionNegotioation->instance(),SIGNAL(sessionTerminated(const IStanzaSession &)),
        SLOT(onStanzaSessionTerminated(const IStanzaSession &)));
    }
  }

  plugin = APluginManager->getPlugins("IRosterPlugin").value(0,NULL);
  if (plugin)
    FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());

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
    connect(FRostersViewPlugin->rostersView()->instance(),SIGNAL(contextMenu(IRosterIndex *, Menu *)),
      SLOT(onRostersViewContextMenu(IRosterIndex *, Menu *)));
  }
  if (FDiscovery)
  {
    registerDiscoFeatures();
  }
  if (FSessionNegotioation)
  {
    FSessionNegotioation->insertNegotiator(this,SNO_DEFAULT);
  }
  if (FSettingsPlugin)
  {
    FSettingsPlugin->openOptionsNode(ON_HISTORY,tr("History"),tr("Common history settings"),MNI_HISTORY,ONO_HISTORY);
  }
  return true;
}

bool MessageArchiver::initSettings()
{
  QString dirPath = collectionDirPath(Jid(),Jid());
  QFile gateways(dirPath+"/"GATEWAY_FILE_NAME);
  if (!dirPath.isEmpty() && gateways.open(QFile::ReadOnly|QFile::Text))
  {
    while (!gateways.atEnd())
    {
      QStringList gateway = QString::fromUtf8(gateways.readLine()).split(" ");
      if (!gateway.value(0).isEmpty() && !gateway.value(1).isEmpty())
        FGatewayTypes.insert(gateway.value(0),gateway.value(1));
    }
  }
  gateways.close();
  return true;
}

bool MessageArchiver::editStanza(int AHandlerId, const Jid &AStreamJid, Stanza *AStanza, bool &/*AAccept*/)
{
  if (FSHIMessageBlocks.value(AStreamJid) == AHandlerId)
  {
    Jid contactJid = AStanza->to();
    IArchiveItemPrefs itemPrefs = archiveItemPrefs(AStreamJid,contactJid);
    if (itemPrefs.otr==ARCHIVE_OTR_REQUIRE && !isOTRStanzaSession(AStreamJid,contactJid))
    {
      int initResult = FSessionNegotioation!=NULL ? FSessionNegotioation->initSession(AStreamJid,contactJid) : ISessionNegotiator::Cancel;
      if (initResult == ISessionNegotiator::Skip)
        notifyInChatWindow(AStreamJid,contactJid, tr("OTR session not ready, please wait..."));
      else if (initResult != ISessionNegotiator::Cancel)
        notifyInChatWindow(AStreamJid,contactJid, tr("Negotiating OTR session ..."));
      return true;
    }
  }
  return false;
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
  else if (FSHIPrefs.value(AStreamJid)==AHandlerId && (AStanza.from().isEmpty() || Jid(AStreamJid.domain())==AStanza.from()))
  {
    QDomElement prefElem = AStanza.firstElement("pref",FNamespaces.value(AStreamJid));
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
      QDomElement prefElem = AStanza.firstElement("pref",FNamespaces.value(AStreamJid));
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
    FPrefsSaveRequests.remove(AStanza.id());
    if (AStanza.type() == "result")
      startSuspendedStanzaSession(AStreamJid,AStanza.id());
    else
      cancelSuspendedStanzaSession(AStreamJid,AStanza.id(),ErrorHandler(AStanza.element()).message());
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
  else if (FPrefsRemoveRequests.contains(AStanza.id()))
  {
    if (AStanza.type()=="result")
    {
      Jid itemJid = FPrefsRemoveRequests.value(AStanza.id());
      FArchivePrefs[AStreamJid].itemPrefs.remove(itemJid);
      emit archiveItemPrefsRemoved(AStreamJid,itemJid);
    }
    FPrefsRemoveRequests.remove(AStanza.id());
  }
  else if (FSaveRequests.contains(AStanza.id()))
  {
    if (AStanza.type() == "result")
    {
      IArchiveHeader header = FSaveRequests.value(AStanza.id());
      QDomElement chatElem = AStanza.firstElement("save",FNamespaces.value(AStreamJid)).firstChildElement("chat");
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
      QDomElement chatElem = AStanza.firstElement("chat"/*,NS_ARCHIVE*/);
      elementToCollection(chatElem,collection);
      QDomElement setElem = chatElem.firstChildElement("set");
      while (!setElem.isNull() && setElem.namespaceURI()!=NS_RESULTSET)
        setElem = setElem.nextSiblingElement("set");
      IArchiveResultSet resultSet;
      resultSet.count = setElem.firstChildElement("count").text().toInt();
      resultSet.index = setElem.firstChildElement("first").attribute("index").toInt();
      resultSet.first = setElem.firstChildElement("first").text();
      resultSet.last = setElem.firstChildElement("last").text();
      emit serverCollectionLoaded(AStanza.id(),collection,resultSet);
    }
    FRetrieveRequests.remove(AStanza.id());
  }
  else if (FListRequests.contains(AStanza.id()))
  {
    if (AStanza.type() == "result")
    {
      QList<IArchiveHeader> headers;
      QDomElement listElem = AStanza.firstElement("list",FNamespaces.value(AStreamJid));
      QDomElement chatElem = listElem.firstChildElement("chat");
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
      QDomElement setElem = listElem.firstChildElement("set");
      while (!setElem.isNull() && setElem.namespaceURI()!=NS_RESULTSET)
        setElem = setElem.nextSiblingElement("set");
      IArchiveResultSet resultSet;
      resultSet.count = setElem.firstChildElement("count").text().toInt();
      resultSet.index = setElem.firstChildElement("first").attribute("index").toInt();
      resultSet.first = setElem.firstChildElement("first").text();
      resultSet.last = setElem.firstChildElement("last").text();
      emit serverHeadersLoaded(AStanza.id(),headers,resultSet);
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

      QDomElement modifsElem = AStanza.firstElement("modified",FNamespaces.value(AStreamJid));
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
          modifs.endTime = modif.header.start;
        }
        else if (changeElem.tagName() == "removed")
        {
          modif.action = IArchiveModification::Removed;
          modifs.items.append(modif);
          modifs.endTime = modif.header.start;
        }
        changeElem = changeElem.nextSiblingElement();
      }

      QDomElement setElem = modifsElem.firstChildElement("set");
      while (!setElem.isNull() && setElem.namespaceURI()!=NS_RESULTSET)
        setElem = setElem.nextSiblingElement("set");
      IArchiveResultSet resultSet;
      resultSet.count = setElem.firstChildElement("count").text().toInt();
      resultSet.index = setElem.firstChildElement("first").attribute("rindex").toInt();
      resultSet.first = setElem.firstChildElement("first").text();
      resultSet.last = setElem.firstChildElement("last").text();
      emit serverModificationsLoaded(AStanza.id(),modifs,resultSet);
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
    Jid streamJid = FPrefsSaveRequests.take(AId);
    FPrefsSaveRequests.remove(AId);
    cancelSuspendedStanzaSession(streamJid,AId,ErrorHandler(ErrorHandler::REQUEST_TIMEOUT).message());
  }
  else if (FPrefsAutoRequests.contains(AId))
  {
    FPrefsAutoRequests.remove(AId);
  }
  else if (FPrefsAutoRequests.contains(AId))
  {
    FPrefsRemoveRequests.remove(AId);
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
  AOrder = OWO_HISTORY;
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

int MessageArchiver::sessionInit(const IStanzaSession &ASession, IDataForm &ARequest)
{
  IArchiveItemPrefs itemPrefs = archiveItemPrefs(ASession.streamJid,ASession.contactJid);
  int result = itemPrefs.otr!=ARCHIVE_OTR_REQUIRE ? ISessionNegotiator::Skip : ISessionNegotiator::Cancel;

  if (FDataForms)
  {
    IDataField logging;
    logging.var = SFP_LOGGING;
    logging.type = DATAFIELD_TYPE_LISTSINGLE;
    logging.required = false;

    if (itemPrefs.otr != ARCHIVE_OTR_FORBID)
    {
      IDataOption option;
      option.value = SFV_MUSTNOT_LOGGING;
      logging.options.append(option);
    }
    if (itemPrefs.otr != ARCHIVE_OTR_REQUIRE)
    {
      IDataOption option;
      option.value = SFV_MAY_LOGGING;
      logging.options.append(option);
      logging.value = SFV_MAY_LOGGING;
    }
    else
    {
      logging.value = SFV_MUSTNOT_LOGGING;
      logging.required = true;
    }

    if (ASession.status == IStanzaSession::Renegotiate)
    {
      int index = FDataForms->fieldIndex(SFP_LOGGING,ASession.form.fields);
      if (index<0 || ASession.form.fields.at(index).value!=logging.value)
      {
        ARequest.fields.append(logging);
        result = ISessionNegotiator::Auto;
      }
      else
        result = ISessionNegotiator::Skip;
    }
    else if (ASession.status == IStanzaSession::Init)
    {
      ARequest.fields.append(logging);
      result = ISessionNegotiator::Auto;
    }
  }
  return result;
}

int MessageArchiver::sessionAccept(const IStanzaSession &ASession, const IDataForm &ARequest, IDataForm &ASubmit)
{
  IArchiveItemPrefs itemPrefs = archiveItemPrefs(ASession.streamJid,ASession.contactJid);
  int result = ISessionNegotiator::Skip;
  
  int rindex = FDataForms!=NULL ? FDataForms->fieldIndex(SFP_LOGGING,ARequest.fields) : -1;
  if (rindex>=0)
  {
    result = ISessionNegotiator::Auto;
    if (ARequest.type == DATAFORM_TYPE_FORM)
    {
      IDataField logging;
      logging.var = SFP_LOGGING;
      logging.type = DATAFIELD_TYPE_LISTSINGLE;
      logging.value = ARequest.fields.at(rindex).value;
      logging.required = false;

      QStringList options;
      for (int i=0; i<ARequest.fields.at(rindex).options.count(); i++)
        options.append(ARequest.fields.at(rindex).options.at(i).value);
      
      if (itemPrefs.otr == ARCHIVE_OTR_APPROVE)
      {
        if (logging.value == SFV_MUSTNOT_LOGGING)
        {
          result = ISessionNegotiator::Manual;
          ASubmit.pages[0].fieldrefs.append(SFP_LOGGING);
          ASubmit.pages[0].childOrder.append(DATALAYOUT_CHILD_FIELDREF);
        }
      }
      else if (itemPrefs.otr == ARCHIVE_OTR_FORBID)
      {
        if (options.contains(SFV_MAY_LOGGING))
          logging.value = SFV_MAY_LOGGING;
        else
          result = ISessionNegotiator::Cancel;
      }
      else if (itemPrefs.otr == ARCHIVE_OTR_OPPOSE)
      {
        if (options.contains(SFV_MAY_LOGGING))
          logging.value = SFV_MAY_LOGGING;
      }
      else if (itemPrefs.otr == ARCHIVE_OTR_PREFER)
      {
        if (options.contains(SFV_MUSTNOT_LOGGING))
          logging.value = SFV_MUSTNOT_LOGGING;
      }
      else if (itemPrefs.otr == ARCHIVE_OTR_REQUIRE)
      {
        if (options.contains(SFV_MUSTNOT_LOGGING))
          logging.value = SFV_MUSTNOT_LOGGING;
        else
          result = ISessionNegotiator::Cancel;
        logging.required = true;
      }
      ASubmit.fields.append(logging);
    }
    else if (ARequest.type == DATAFORM_TYPE_SUBMIT)
    {
      QString sessionValue = ARequest.fields.at(rindex).value.toString();
      if (itemPrefs.otr==ARCHIVE_OTR_FORBID && sessionValue==SFV_MUSTNOT_LOGGING)
        result = ISessionNegotiator::Cancel;
      else if (itemPrefs.otr==ARCHIVE_OTR_REQUIRE && sessionValue==SFV_MAY_LOGGING)
        result = ISessionNegotiator::Cancel;
      else if (itemPrefs.otr==ARCHIVE_OTR_APPROVE && sessionValue==SFV_MUSTNOT_LOGGING)
      {
        result = ISessionNegotiator::Manual;
        ASubmit.pages[0].fieldrefs.append(SFP_LOGGING);
        ASubmit.pages[0].childOrder.append(DATALAYOUT_CHILD_FIELDREF);
      }
    }
  } 
  else if (ASession.status!=IStanzaSession::Active && itemPrefs.otr==ARCHIVE_OTR_REQUIRE)
  {
    result = ISessionNegotiator::Cancel;
  }
  return result;
}

int MessageArchiver::sessionApply(const IStanzaSession &ASession)
{
  int result = ISessionNegotiator::Skip;
  IArchiveItemPrefs itemPrefs = archiveItemPrefs(ASession.streamJid,ASession.contactJid);
  if (FDataForms && isReady(ASession.streamJid))
  {
    int index = FDataForms->fieldIndex(SFP_LOGGING,ASession.form.fields);
    QString sessionValue = index>=0 ? ASession.form.fields.at(index).value.toString() : "";
    if (itemPrefs.otr==ARCHIVE_OTR_REQUIRE && sessionValue==SFV_MAY_LOGGING)
    {
      result = ISessionNegotiator::Cancel;
    }
    else if (itemPrefs.otr==ARCHIVE_OTR_FORBID && sessionValue==SFV_MUSTNOT_LOGGING)
    {
      result = ISessionNegotiator::Cancel;
    }
    else if (sessionValue==SFV_MUSTNOT_LOGGING && itemPrefs.save!=ARCHIVE_SAVE_FALSE)
    {
      StanzaSession &session = FSessions[ASession.streamJid][ASession.contactJid];
      if (FPrefsSaveRequests.contains(session.requestId))
      {
        result = ISessionNegotiator::Wait;
      }
      else if (!session.error.isEmpty())
      {
        result = ISessionNegotiator::Cancel;
      }
      else 
      {
        IArchiveStreamPrefs prefs = archivePrefs(ASession.streamJid);
        if (session.sessionId.isEmpty())
        {
          session.sessionId = ASession.sessionId;
          session.saveMode = itemPrefs.save;
          session.defaultPrefs = !prefs.itemPrefs.contains(ASession.contactJid);
        }
        itemPrefs.save = ARCHIVE_SAVE_FALSE;
        prefs.itemPrefs[ASession.contactJid] = itemPrefs;
        session.requestId = setArchivePrefs(ASession.streamJid,prefs);
        result = !session.requestId.isEmpty() ? ISessionNegotiator::Wait : ISessionNegotiator::Cancel;
      }
    }
    else
    {
      result = ISessionNegotiator::Auto;
    }
  }
  else if (itemPrefs.otr==ARCHIVE_OTR_REQUIRE)
  {
    result = ISessionNegotiator::Cancel;
  }
  return result;
}

void MessageArchiver::sessionLocalize(const IStanzaSession &/*ASession*/, IDataForm &AForm)
{
  if (FDataForms)
  {
    int index = FDataForms->fieldIndex(SFP_LOGGING,AForm.fields);
    if (index>=0)
    {
      AForm.fields[index].label = tr("Message logging");
      QList<IDataOption> &options = AForm.fields[index].options;
      for (int i=0;i<options.count();i++)
      {
        if (options[i].value == SFV_MAY_LOGGING)
          options[i].label = tr("Allow message logging");
        else if (options[i].value == SFV_MUSTNOT_LOGGING)
          options[i].label = tr("Disallow all message logging");
      }
    }
  }
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

bool MessageArchiver::isArchivingAllowed(const Jid &AStreamJid, const Jid &AItemJid) const
{
  if (isReady(AStreamJid) && AItemJid.isValid())
  {
    IArchiveItemPrefs itemPrefs = archiveItemPrefs(AStreamJid,AItemJid);
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

IArchiveItemPrefs MessageArchiver::archiveItemPrefs(const Jid &AStreamJid, const Jid &AItemJid) const
{
  IArchiveStreamPrefs prefs = FArchivePrefs.value(AStreamJid);
  QHash<Jid, IArchiveItemPrefs>::const_iterator it = prefs.itemPrefs.constBegin();
  while (it != prefs.itemPrefs.constEnd())
  {
    QString node = it.key().pNode();
    QString resource = it.key().resource();
    if (
        (it.key().pDomain() == AItemJid.pDomain()) &&
        (node.isEmpty() || node == AItemJid.pNode()) &&
        (resource.isEmpty() || resource == AItemJid.resource())
       )
    {
      return it.value();
    }
    it++;
  }
  return prefs.defaultPrefs;
}

QString MessageArchiver::setArchiveAutoSave(const Jid &AStreamJid, bool AAuto)
{
  if (isSupported(AStreamJid))
  {
    Stanza autoSave("iq");
    autoSave.setType("set").setId(FStanzaProcessor->newId());
    QDomElement autoElem = autoSave.addElement("auto",FNamespaces.value(AStreamJid));
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

    bool itemsChanged = false;
    foreach(Jid itemJid, APrefs.itemPrefs.keys())
    {
      IArchiveItemPrefs newItemPrefs = APrefs.itemPrefs.value(itemJid);
      if (!newItemPrefs.save.isEmpty() && !newItemPrefs.otr.isEmpty())
      {
        newPrefs.itemPrefs.insert(itemJid,newItemPrefs);
        if (newPrefs.itemPrefs.value(itemJid).otr == ARCHIVE_OTR_REQUIRE)
          newPrefs.itemPrefs[itemJid].save = ARCHIVE_SAVE_FALSE;
      }
      else
      {
        itemsChanged = true;
        newPrefs.itemPrefs.remove(itemJid);
      }
    }

    Stanza save("iq");
    save.setType("set").setId(FStanzaProcessor->newId());

    QDomElement prefElem = save.addElement("pref",!storage ? FNamespaces.value(AStreamJid) : NS_ARCHIVE);

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

    foreach(Jid itemJid, newPrefs.itemPrefs.keys())
    {
      IArchiveItemPrefs newItemPrefs = newPrefs.itemPrefs.value(itemJid);
      IArchiveItemPrefs oldItemPrefs = oldPrefs.itemPrefs.value(itemJid);
      bool itemChanged = oldItemPrefs.save   != newItemPrefs.save ||
                         oldItemPrefs.otr    != newItemPrefs.otr ||
                         oldItemPrefs.expire != newItemPrefs.expire;
      if (storage || itemChanged)
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
        FPrefsSaveRequests.insert(requestId,AStreamJid);
        return requestId;
      }
    }
  }
  return QString();
}

QString MessageArchiver::removeArchiveItemPrefs(const Jid &AStreamJid, const Jid &AItemJid)
{
  if (isReady(AStreamJid) && archivePrefs(AStreamJid).itemPrefs.contains(AItemJid))
  {
    if (isSupported(AStreamJid))
    {
      Stanza remove("iq");
      remove.setType("set").setId(FStanzaProcessor->newId());
      QDomElement itemElem = remove.addElement("itemremove",FNamespaces.value(AStreamJid)).appendChild(remove.createElement("item")).toElement();
      itemElem.setAttribute("jid",AItemJid.eFull());
      if (FStanzaProcessor->sendIqStanza(this,AStreamJid,remove,ARCHIVE_TIMEOUT))
      {
        FPrefsRemoveRequests.insert(remove.id(),AItemJid);
        return remove.id();
      }
    }
    else
    {
      IArchiveStreamPrefs prefs;
      prefs.itemPrefs[AItemJid].otr = "";
      prefs.itemPrefs[AItemJid].save = "";
      return setArchivePrefs(AStreamJid,prefs);
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
    FCleanupHandler.add(window);
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

bool MessageArchiver::saveMessage(const Jid &AStreamJid, const Jid &AItemJid, const Message &AMessage)
{
  if (isReady(AStreamJid) && AItemJid.isValid() && !AMessage.body().isEmpty())
  {
    Jid itemJid = AMessage.type()==Message::GroupChat ? AItemJid.bare() : AItemJid;
    CollectionWriter *writer = findCollectionWriter(AStreamJid,itemJid,AMessage.threadId());
    if (!writer)
    {
      QDateTime currentTime = QDateTime::currentDateTime();
      IArchiveHeader header;
      header.with = itemJid;
      header.start = currentTime.addMSecs(-currentTime.time().msec());
      header.subject = AMessage.subject();
      header.threadId = AMessage.threadId();
      header.version = 0;
      writer = newCollectionWriter(AStreamJid,header);
    }
    if (writer)
    {
      bool directionIn = AItemJid == AMessage.from();
      return writer->writeMessage(AMessage,archiveItemPrefs(AStreamJid,itemJid).save,directionIn);
    }
  }
  return false;
}

bool MessageArchiver::saveNote(const Jid &AStreamJid, const Jid &AItemJid, const QString &ANote, const QString &AThreadId)
{
  if (isReady(AStreamJid) && AItemJid.isValid() && !ANote.isEmpty())
  {
    CollectionWriter *writer = findCollectionWriter(AStreamJid,AItemJid,AThreadId);
    if (!writer)
    {
      IArchiveHeader header;
      header.with = AItemJid;
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

Jid MessageArchiver::gateJid(const Jid &AContactJid) const
{
  Jid gate = AContactJid;
  if (!gate.node().isEmpty() && FGatewayTypes.contains(gate.domain()))
    gate.setDomain(QString("%1.gateway").arg(FGatewayTypes.value(gate.domain())));
  return gate;
}

QString MessageArchiver::gateNick(const Jid &AStreamJid, const Jid &AContactJid) const
{
  IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AStreamJid) : NULL;
  if (roster)
  {
    Jid gcontactJid = gateJid(AContactJid.bare());
    foreach(IRosterItem ritem, roster->rosterItems())
      if (ritem.itemJid.pNode()==AContactJid.pNode() && gcontactJid==gateJid(ritem.itemJid))
        return ritem.name.isEmpty() ? (ritem.itemJid.node().isEmpty() ? ritem.itemJid.domain() : ritem.itemJid.node()) : ritem.name;
  }
  return AContactJid.node().isEmpty() ? AContactJid.domain() : AContactJid.node();
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
      doc.setContent(file.readAll(),true);
      elementToCollection(doc.documentElement(),collection);
      file.close();
    }
  }
  return collection;
}

bool MessageArchiver::removeLocalCollection(const Jid &AStreamJid, const IArchiveHeader &AHeader)
{
  delete findCollectionWriter(AStreamJid,AHeader.with,AHeader.threadId);
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
  if (!dirPath.isEmpty() && AStreamJid.isValid() && AStart.isValid())
  {
    QFile log(dirPath+"/"LOG_FILE_NAME);
    if (log.open(QFile::ReadOnly|QIODevice::Text))
    {
      //����� ������ ������� ������� �������
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

      //���������������� ������� �������
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
    if (FReplicators.contains(AStreamJid))
      return FReplicators.value(AStreamJid)->replicationPoint();
    else
      return QDateTime::currentDateTime();
  }
  return QDateTime();
}

bool MessageArchiver::replicationEnabled(const Jid &AStreamJid) const
{
  if (isSupported(AStreamJid))
  {
    IAccount *account = FAccountManager!=NULL ? FAccountManager->accountByStream(AStreamJid) : NULL;
    if (account)
      return account->value(AVN_REPLICATION,false).toBool();
  }
  return false;
}

void MessageArchiver::setReplicationEnabled(const Jid &AStreamJid, bool AEnabled)
{
  if (FReplicators.contains(AStreamJid))
  {
    FReplicators.value(AStreamJid)->setEnabled(AEnabled);
    IAccount *account = FAccountManager!=NULL ? FAccountManager->accountByStream(AStreamJid) : NULL;
    if (account)  
      account->setValue(AVN_REPLICATION,AEnabled);
  }
}

QString MessageArchiver::saveServerCollection(const Jid &AStreamJid, const IArchiveCollection &ACollection)
{
  if (FStanzaProcessor && isSupported(AStreamJid) && ACollection.header.with.isValid() && ACollection.header.start.isValid())
  {
    Stanza save("iq");
    save.setType("set").setId(FStanzaProcessor->newId());
    QDomElement chatElem = save.addElement("save",FNamespaces.value(AStreamJid)).appendChild(save.createElement("chat")).toElement();
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
    QDomElement listElem = request.addElement("list",FNamespaces.value(AStreamJid));
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
    QDomElement retrieveElem = retrieve.addElement("retrieve",FNamespaces.value(AStreamJid));
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
    QDomElement removeElem = remove.addElement("remove",FNamespaces.value(AStreamJid));
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
    QDomElement modifyElem = modify.addElement("modified",FNamespaces.value(AStreamJid));
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
    load.addElement("pref",FNamespaces.value(AStreamJid));
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
    if (FNamespaces.value(AStreamJid) == NS_ARCHIVE)
    {
      FNamespaces.insert(AStreamJid,NS_ARCHIVE_OLD);
      loadServerPrefs(AStreamJid);
    }
    else
    {
      FInStoragePrefs.append(AStreamJid);
      loadStoragePrefs(AStreamJid);
    }
  }
  else
  {
    //������� ��� jabberd 1.4.3
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
        prefs.autoSave = QVariant(autoElem.attribute("save","false")).toBool();
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
          emit archiveItemPrefsRemoved(AStreamJid,itemJid);
        }
      }
      else if (initPrefs)
      {
        Replicator *replicator = insertReplicator(AStreamJid);
        if (replicator)
          replicator->setEnabled(replicationEnabled(AStreamJid));
      }

      terminateBrokenStanzaSessions(AStreamJid);

      emit archivePrefsChanged(AStreamJid,prefs);
    }
  }
}

QString MessageArchiver::collectionDirName(const Jid &AJid) const
{
  Jid jid = gateJid(AJid);
  QString dirName = Jid::encode(jid.pBare());
  if (!jid.resource().isEmpty())
    dirName += "/"+Jid::encode(jid.pResource());
  return dirName;
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
  QDir dir;
  bool noError = true;

  if (FSettingsPlugin)
    dir.setPath(FSettingsPlugin->homeDir().path());

  if (!dir.exists(ARCHIVE_DIRNAME))
    noError &= dir.mkdir(ARCHIVE_DIRNAME);
  noError &= dir.cd(ARCHIVE_DIRNAME);

  if (noError && AStreamJid.isValid())
  {
    QString streamDir = collectionDirName(AStreamJid.bare());
    if (!dir.exists(streamDir))
      noError &= dir.mkdir(streamDir);
    noError &= dir.cd(streamDir);
  }

  if (noError && AWith.isValid())
  {
    QString withDir = collectionDirName(AWith);
    if (!dir.exists(withDir))
      noError &= dir.mkdir(withDir);
    noError &= dir.cd(withDir);
  }

  return noError ? dir.path() : QString();
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

QMultiMap<QString,QString> MessageArchiver::filterCollectionFiles(const QStringList &AFiles, const IArchiveRequest &ARequest, 
                                                                  const QString &APrefix) const
{
  QMultiMap<QString,QString> filesMap;
  if (!AFiles.isEmpty())
  {
    QString startName = collectionFileName(ARequest.start);
    QString endName = collectionFileName(ARequest.end);
    foreach(QString file, AFiles)
      if ((startName.isEmpty() || startName<=file) && (endName.isEmpty() || endName>=file))
        filesMap.insert(file,APrefix);
  }
  return filesMap;
}

QStringList MessageArchiver::findCollectionFiles(const Jid &AStreamJid, const IArchiveRequest &ARequest) const
{
  QMultiMap<QString,QString> filesMap;
  QString dirPath = collectionDirPath(AStreamJid,ARequest.with.bare());
  QDir dir(dirPath);
  if (AStreamJid.isValid() && dir.exists())
  {
    if (ARequest.with.isValid())
      filesMap.unite(filterCollectionFiles(dir.entryList(QStringList() << "*"COLLECTION_EXT,QDir::Files),ARequest,""));
    QStringList dirs1 = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    foreach (QString dir1, dirs1)
    {
      dir.cd(dir1);
      QString dir1Prefix = dir1+"/";
      filesMap.unite(filterCollectionFiles(dir.entryList(QStringList() << "*"COLLECTION_EXT,QDir::Files),ARequest,dir1Prefix));
      if (!ARequest.with.isValid())
      {
        QStringList dirs2 = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        foreach (QString dir2, dirs2)
        {
          dir.cd(dir2);
          QString dir2Prefix = dir1Prefix+dir2+"/";
          filesMap.unite(filterCollectionFiles(dir.entryList(QStringList() << "*"COLLECTION_EXT,QDir::Files),ARequest,dir2Prefix));
          dir.cdUp();
        }
      }
      dir.cdUp();
    }

    QStringList files;
    QMapIterator<QString,QString> it(filesMap);
    if (ARequest.order == Qt::DescendingOrder)
      it.toBack();
    while (files.count()<ARequest.count && (ARequest.order==Qt::AscendingOrder ? it.hasNext() : it.hasPrevious()))
    {
      if (ARequest.order == Qt::AscendingOrder)
        it.next();
      else
        it.previous();
      files.append(dir.absoluteFilePath(it.value()+it.key()));
    }

    return files;
  }
  return QStringList();
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

      Jid with = ACollection.header.with;
      QString nick = nodeElem.attribute("name");
      Jid contactJid(with.node(),with.domain(),nick.isEmpty() ? with.resource() : nick);

      nodeElem.tagName()=="to" ? message.setTo(contactJid.eFull()) : message.setFrom(contactJid.eFull());

      if (!nick.isEmpty())
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
  QDomDocument ownerDoc = AChatElem.ownerDocument();
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
  if (!dirPath.isEmpty() && AStreamJid.isValid() && AHeader.with.isValid() && AHeader.start.isValid())
  {
    QFile log(dirPath+"/"LOG_FILE_NAME);
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

Replicator *MessageArchiver::insertReplicator(const Jid &AStreamJid)
{
  if (isSupported(AStreamJid) && !FReplicators.contains(AStreamJid))
  {
    QString dirPath = collectionDirPath(AStreamJid,Jid());
    if (AStreamJid.isValid() && !dirPath.isEmpty())
    {
      Replicator *replicator = new Replicator(this,AStreamJid,dirPath,this);
      FReplicators.insert(AStreamJid,replicator);
      return replicator;
    }
  }
  return NULL;
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
    FSettingsPlugin->openOptionsNode(node,account->name(),tr("Message archiving preferences"),MNI_HISTORY,ONO_HISTORY);
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
  menu->setTitle(tr("History"));
  menu->setIcon(RSR_STORAGE_MENUICONS,MNI_HISTORY);

  Action *action = new Action(menu);
  action->setText(tr("View History"));
  action->setIcon(RSR_STORAGE_MENUICONS,MNI_HISTORY_VIEW);
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
      autoMenu->setIcon(RSR_STORAGE_MENUICONS,MNI_HISTORY_AUTO);
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
      manualMenu->setIcon(RSR_STORAGE_MENUICONS,MNI_HISTORY_MANUAL);
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
      localMenu->setIcon(RSR_STORAGE_MENUICONS,MNI_HISTORY_LOCAL);
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
      action->setEnabled(!isOTRStanzaSession(AStreamJid,AContactJid));
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
      action->setEnabled(!hasStanzaSession(AStreamJid,AContactJid) || isOTRStanzaSession(AStreamJid,AContactJid));
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
      action->setEnabled(!isOTRStanzaSession(AStreamJid,AContactJid));
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
      action->setEnabled(!isOTRStanzaSession(AStreamJid,AContactJid));
      action->setText(saveModeName(ARCHIVE_SAVE_MESSAGE));
      connect(action,SIGNAL(triggered(bool)),SLOT(onSetItemPrefsAction(bool)));
      saveMenu->addAction(action,AG_DEFAULT,false);
    saveMenu->setEnabled(itemPrefs.otr!=ARCHIVE_OTR_REQUIRE);
    menu->addAction(saveMenu->menuAction(),AG_DEFAULT+500,false);

    if (isStreamMenu)
    {
      action = new Action(menu);
      action->setText(tr("Options..."));
      action->setIcon(RSR_STORAGE_MENUICONS,MNI_HISTORY_OPTIONS);
      action->setData(ADR_STREAM_JID,AStreamJid.full());
      connect(action,SIGNAL(triggered(bool)),SLOT(onOpenHistoryOptionsAction(bool)));
      menu->addAction(action,AG_DEFAULT+500,false);
    }
    else if (!isOTRStanzaSession(AStreamJid,AContactJid) && archivePrefs(AStreamJid).itemPrefs.contains(AContactJid))
    {
      action = new Action(menu);
      action->setText(tr("Restore defaults"));
      action->setIcon(RSR_STORAGE_MENUICONS,MNI_HISTORY_DEFAULTS);
      action->setData(ADR_STREAM_JID,AStreamJid.full());
      action->setData(ADR_CONTACT_JID,AContactJid.full());
      connect(action,SIGNAL(triggered(bool)),SLOT(onRemoveItemPrefsAction(bool)));
      menu->addAction(action,AG_DEFAULT+500,false);
    }
  }
  return menu;
}

void MessageArchiver::registerDiscoFeatures()
{
  IDiscoFeature dfeature;

  dfeature.active = false;
  dfeature.icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_HISTORY);
  dfeature.var = NS_ARCHIVE;
  dfeature.name = tr("Message Archiving");
  dfeature.description = tr("Archiving and retrieval of messages");
  FDiscovery->insertDiscoFeature(dfeature);
  dfeature.var = NS_ARCHIVE_OLD;
  FDiscovery->insertDiscoFeature(dfeature);

  dfeature.var = NS_ARCHIVE_AUTO;
  dfeature.icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_HISTORY_AUTO);
  dfeature.name = tr("Message Archiving: Auto archiving");
  dfeature.description = tr("Automatically archiving of messages on server");
  FDiscovery->insertDiscoFeature(dfeature);
  dfeature.var = NS_ARCHIVE_OLD_AUTO;
  FDiscovery->insertDiscoFeature(dfeature);

  dfeature.var = NS_ARCHIVE_MANAGE;
  dfeature.icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_HISTORY_MANAGE);
  dfeature.name = tr("Message Archiving: Mange messages");
  dfeature.description = tr("Retrieving and removing messages on server");
  FDiscovery->insertDiscoFeature(dfeature);
  dfeature.var = NS_ARCHIVE_OLD_MANAGE;
  FDiscovery->insertDiscoFeature(dfeature);

  dfeature.var = NS_ARCHIVE_MANUAL;
  dfeature.icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_HISTORY_MANUAL);
  dfeature.name = tr("Message Archiving: Manual archiving");
  dfeature.description = tr("Manually upload messages on server");
  FDiscovery->insertDiscoFeature(dfeature);
  dfeature.var = NS_ARCHIVE_OLD_MANUAL;
  FDiscovery->insertDiscoFeature(dfeature);

  dfeature.var = NS_ARCHIVE_PREF;
  dfeature.icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_HISTORY_OPTIONS);
  dfeature.name = tr("Message Archiving: Preferences");
  dfeature.description = tr("Storing archive preferences on server");
  FDiscovery->insertDiscoFeature(dfeature);
  dfeature.var = NS_ARCHIVE_OLD_PREF;
  FDiscovery->insertDiscoFeature(dfeature);

  dfeature.var = NS_ARCHIVE_ENCRYPT;
  dfeature.name = tr("Message Archiving: Encrypted messages");
  dfeature.description = tr("Archiving of encrypted messages on server");
  FDiscovery->insertDiscoFeature(dfeature);
}

void MessageArchiver::notifyInChatWindow(const Jid &AStreamJid, const Jid &AContactJid, const QString &AMessage) const
{
  IChatWindow *window = FMessenger!=NULL ? FMessenger->findChatWindow(AStreamJid,AContactJid) : NULL;
  if (window)
  {
    QString html = QString("<span style='color:green;'>*** %1</span>").arg(AMessage);
    window->viewWidget()->showCustomMessage(html,QDateTime::currentDateTime());
  }
}

bool MessageArchiver::hasStanzaSession(const Jid &AStreamJid, const Jid &AContactJid) const
{
  return FSessionNegotioation!=NULL ? FSessionNegotioation->getSession(AStreamJid,AContactJid).status==IStanzaSession::Active : false;
}

bool MessageArchiver::isOTRStanzaSession(const IStanzaSession &ASession) const
{
  if (FDataForms)
  {
    int index = FDataForms->fieldIndex(SFP_LOGGING,ASession.form.fields);
    if (index>=0)
      return ASession.form.fields.at(index).value.toString() == SFV_MUSTNOT_LOGGING;
  }
  return false;
}

bool MessageArchiver::isOTRStanzaSession(const Jid &AStreamJid, const Jid &AContactJid) const
{
  if (FSessionNegotioation && FDataForms)
  {
    IStanzaSession session = FSessionNegotioation->getSession(AStreamJid,AContactJid);
    if (session.status == IStanzaSession::Active)
      return isOTRStanzaSession(session);
  }
  return false;
}

void MessageArchiver::startSuspendedStanzaSession(const Jid &AStreamJid, const QString &ARequestId)
{
  if (FSessionNegotioation)
  {
    foreach(Jid contactJid, FSessions.value(AStreamJid).keys())
    {
      StanzaSession &session = FSessions[AStreamJid][contactJid];
      if (session.requestId == ARequestId)
      {
        FSessionNegotioation->resumeSession(AStreamJid,contactJid);
        break;
      }
    }
  }
}

void MessageArchiver::cancelSuspendedStanzaSession(const Jid &AStreamJid, const QString &ARequestId, const QString &AError)
{
  if (FSessionNegotioation)
  {
    foreach(Jid contactJid, FSessions.value(AStreamJid).keys())
    {
      StanzaSession &session = FSessions[AStreamJid][contactJid];
      if (session.requestId == ARequestId)
      {
        session.error = AError;
        FSessionNegotioation->resumeSession(AStreamJid,contactJid);
        break;
      }
    }
  }
}

void MessageArchiver::terminateBrokenStanzaSessions(const Jid &AStreamJid) const
{
  if (FSessionNegotioation)
  {
    QList<IStanzaSession> sessions = FSessionNegotioation->getSessions(AStreamJid,IStanzaSession::Active);
    foreach(IStanzaSession session, sessions)
    {
      bool isOTRSession = isOTRStanzaSession(session);
      IArchiveItemPrefs itemPrefs = archiveItemPrefs(AStreamJid,session.contactJid);
      if (isOTRSession && itemPrefs.save!=ARCHIVE_SAVE_FALSE)
        FSessionNegotioation->initSession(AStreamJid,session.contactJid);
      else if (!isOTRSession && itemPrefs.otr==ARCHIVE_OTR_REQUIRE)
        FSessionNegotioation->initSession(AStreamJid,session.contactJid);
    }
  }
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

    handler = FStanzaProcessor->insertHandler(this,SHC_MESSAGE_BODY,IStanzaProcessor::DirectionOut,SHP_MESSAGEARCHIVER_SESSION);
    FSHIMessageBlocks.insert(AXmppStream->jid(),handler);
  }
  
  FNamespaces.insert(AXmppStream->jid(),NS_ARCHIVE);
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
  FNamespaces.remove(AXmppStream->jid());
  FArchivePrefs.remove(AXmppStream->jid());
  FInStoragePrefs.removeAt(FInStoragePrefs.indexOf(AXmppStream->jid()));
  FSessions.remove(AXmppStream->jid());
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
    FPrefsSaveRequests.remove(AId);
    startSuspendedStanzaSession(AStreamJid,AId);
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
    Jid streamJid = FPrefsSaveRequests.take(AId);
    cancelSuspendedStanzaSession(streamJid,AId,AError);
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

void MessageArchiver::onOpenHistoryOptionsAction(bool)
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

void MessageArchiver::onRemoveItemPrefsAction(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    Jid streamJid = action->data(ADR_STREAM_JID).toString();
    Jid contactJid = action->data(ADR_CONTACT_JID).toString();
    removeArchiveItemPrefs(streamJid,contactJid);
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

void MessageArchiver::onDiscoInfoReceived(const IDiscoInfo &ADiscoInfo)
{
  if (ADiscoInfo.node.isEmpty() && ADiscoInfo.contactJid.node().isEmpty() &&  ADiscoInfo.contactJid.resource().isEmpty() &&
    !FGatewayTypes.contains(ADiscoInfo.contactJid))
  {
    foreach(IDiscoIdentity identity, ADiscoInfo.identity)
    {
      if (identity.category==CATEGORY_GATEWAY && !identity.type.isEmpty())
      {
        QString dirPath = collectionDirPath(Jid(),Jid());
        QFile gateways(dirPath+"/"GATEWAY_FILE_NAME);
        if (!dirPath.isEmpty() && gateways.open(QFile::WriteOnly|QFile::Append|QFile::Text))
        {
          QStringList gateway;
          gateway << ADiscoInfo.contactJid.pDomain();
          gateway << identity.type;
          gateway << "\n";
          gateways.write(gateway.join(" ").toUtf8());
          gateways.close();
        }
        FGatewayTypes.insert(ADiscoInfo.contactJid,identity.type);
        break;
      }
    }
  }
}

void MessageArchiver::onStanzaSessionActivated(const IStanzaSession &ASession)
{
  if (isOTRStanzaSession(ASession))
  {
    notifyInChatWindow(ASession.streamJid,ASession.contactJid,tr("Off the record session established."));
  }
}

void MessageArchiver::onStanzaSessionTerminated(const IStanzaSession &ASession)
{
  StanzaSession session = FSessions[ASession.streamJid].take(ASession.contactJid);
  if (session.sessionId == ASession.sessionId)
  {
    if (!session.defaultPrefs)
    {
      IArchiveStreamPrefs prefs = archivePrefs(ASession.streamJid);
      prefs.itemPrefs[ASession.contactJid].save = session.saveMode;
      setArchivePrefs(ASession.streamJid,prefs);
    }
    else
    {
      removeArchiveItemPrefs(ASession.streamJid,ASession.contactJid);
    }
  }
  if (isOTRStanzaSession(ASession) || archiveItemPrefs(ASession.streamJid,ASession.contactJid).otr==ARCHIVE_OTR_REQUIRE)
  {
    if (ASession.errorCondition.isEmpty())
      notifyInChatWindow(ASession.streamJid,ASession.contactJid,tr("OTR session terminated."));
    else
      notifyInChatWindow(ASession.streamJid,ASession.contactJid,tr("OTR session failed: %1").arg(ErrorHandler(ASession.errorCondition).message()));
  }
}

Q_EXPORT_PLUGIN2(MessageArchiverPlugin, MessageArchiver)
