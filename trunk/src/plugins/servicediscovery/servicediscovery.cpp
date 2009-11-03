#include "servicediscovery.h"

#include <QDebug>
#include <QFile>
#include <QCryptographicHash>

#define SHC_DISCO_INFO          "/iq[@type='get']/query[@xmlns='" NS_DISCO_INFO "']"
#define SHC_DISCO_ITEMS         "/iq[@type='get']/query[@xmlns='" NS_DISCO_ITEMS "']"
#define SHC_PRESENCE            "/presence"

#define DISCO_TIMEOUT           60000

#define ADR_STREAMJID           Action::DR_StreamJid
#define ADR_CONTACTJID          Action::DR_Parametr1
#define ADR_NODE                Action::DR_Parametr2

#define BDI_ITEMS_GEOMETRY      "DiscoItemsWindowGeometry"

#define DIC_CLIENT              "client"

#define QUEUE_TIMER_INTERVAL    2000
#define QUEUE_REQUEST_WAIT      5000
#define QUEUE_REQUEST_START     QDateTime::currentDateTime().addMSecs(QUEUE_REQUEST_WAIT)

#define CAPS_DIRNAME            "caps"
#define CAPS_FILE_TAG_NAME      "capabilities"

#define CAPS_HASH_MD5           "md5"
#define CAPS_HASH_SHA1          "sha-1"

ServiceDiscovery::ServiceDiscovery()
{
  FPluginManager = NULL;
  FXmppStreams = NULL;
  FRosterPlugin = NULL;
  FPresencePlugin = NULL;
  FStanzaProcessor = NULL;
  FRostersView = NULL;
  FRostersViewPlugin = NULL;
  FMultiUserChatPlugin = NULL;
  FTrayManager = NULL;
  FMainWindowPlugin = NULL;
  FRostersModel = NULL;
  FStatusIcons = NULL;
  FDataForms = NULL;

  FDiscoMenu = NULL;
  FQueueTimer.setSingleShot(false);
  FQueueTimer.setInterval(QUEUE_TIMER_INTERVAL);
  connect(&FQueueTimer,SIGNAL(timeout()),SLOT(onQueueTimerTimeout()));

  connect(this,SIGNAL(discoInfoReceived(const IDiscoInfo &)),SLOT(onDiscoInfoReceived(const IDiscoInfo &)));
}

ServiceDiscovery::~ServiceDiscovery()
{
  delete FDiscoMenu;
}

void ServiceDiscovery::pluginInfo(IPluginInfo *APluginInfo)
{
  APluginInfo->name = tr("Service Discovery");
  APluginInfo->description = tr("Discovering information about Jabber entities and the items associated with such entities");
  APluginInfo->version = "1.0";
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->homePage = "http://jrudevels.org";
}

bool ServiceDiscovery::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  FPluginManager = APluginManager;

  IPlugin *plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
  if (plugin)
  {
    FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
    if (FXmppStreams)
    {
      connect(FXmppStreams->instance(),SIGNAL(opened(IXmppStream *)),SLOT(onStreamOpened(IXmppStream *)));
      connect(FXmppStreams->instance(),SIGNAL(closed(IXmppStream *)),SLOT(onStreamClosed(IXmppStream *)));
      connect(FXmppStreams->instance(),SIGNAL(removed(IXmppStream *)),SLOT(onStreamRemoved(IXmppStream *)));
      connect(FXmppStreams->instance(),SIGNAL(jidChanged(IXmppStream *, const Jid &)),SLOT(onStreamJidChanged(IXmppStream *, const Jid &)));
    }
  }

  plugin = APluginManager->pluginInterface("IPresencePlugin").value(0,NULL);
  if (plugin)
  {
    FPresencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance());
    if (FPresencePlugin)
    {
      connect(FPresencePlugin->instance(),SIGNAL(streamStateChanged(const Jid &, bool)),
        SLOT(onStreamStateChanged(const Jid &, bool)));
      connect(FPresencePlugin->instance(),SIGNAL(contactStateChanged(const Jid &, const Jid &, bool)),
        SLOT(onContactStateChanged(const Jid &, const Jid &, bool)));
    }
  }

  plugin = APluginManager->pluginInterface("IRosterPlugin").value(0,NULL);
  if (plugin)
  {
    FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
    if (FRosterPlugin)
    {
      connect(FRosterPlugin->instance(),SIGNAL(rosterItemReceived(IRoster *, const IRosterItem &)),
        SLOT(onRosterItemReceived(IRoster *, const IRosterItem &)));
    }
  }

  plugin = APluginManager->pluginInterface("IMultiUserChatPlugin").value(0,NULL);
  if (plugin)
  {
    FMultiUserChatPlugin = qobject_cast<IMultiUserChatPlugin *>(plugin->instance());
    if (FMultiUserChatPlugin)
    {
      connect(FMultiUserChatPlugin->instance(),SIGNAL(multiUserContextMenu(IMultiUserChatWindow *, IMultiUser *, Menu *)),
        SLOT(onMultiUserContextMenu(IMultiUserChatWindow *, IMultiUser *, Menu *)));
      connect(FMultiUserChatPlugin->instance(),SIGNAL(multiUserChatCreated(IMultiUserChat *)),SLOT(onMultiUserChatCreated(IMultiUserChat *)));
    }
  }

  plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0,NULL);
  if (plugin)
    FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());

  plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
  if (plugin)
    FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());

  plugin = APluginManager->pluginInterface("IRostersModel").value(0,NULL);
  if (plugin)
    FRostersModel = qobject_cast<IRostersModel *>(plugin->instance());

  plugin = APluginManager->pluginInterface("IStatusIcons").value(0,NULL);
  if (plugin)
    FStatusIcons = qobject_cast<IStatusIcons *>(plugin->instance());

  plugin = APluginManager->pluginInterface("ITrayManager").value(0,NULL);
  if (plugin)
    FTrayManager = qobject_cast<ITrayManager *>(plugin->instance());

  plugin = APluginManager->pluginInterface("IMainWindowPlugin").value(0,NULL);
  if (plugin)
    FMainWindowPlugin = qobject_cast<IMainWindowPlugin *>(plugin->instance());

  plugin = APluginManager->pluginInterface("ISettingsPlugin").value(0,NULL);
  if (plugin)
    FSettingsPlugin = qobject_cast<ISettingsPlugin *>(plugin->instance());

  plugin = APluginManager->pluginInterface("IDataForms").value(0,NULL);
  if (plugin)
    FDataForms = qobject_cast<IDataForms *>(plugin->instance());

  return true;
}

bool ServiceDiscovery::initObjects()
{
  FDiscoMenu = new Menu;
  FDiscoMenu->setIcon(RSR_STORAGE_MENUICONS,MNI_SDISCOVERY_DISCOVER);
  FDiscoMenu->setTitle(tr("Service Discovery"));

  registerFeatures();
  insertDiscoHandler(this);

  if (FRostersViewPlugin)
  {
    FRostersView = FRostersViewPlugin->rostersView();
    FRostersView->insertClickHooker(RCHO_SERVICEDISCOVERY,this);
    connect(FRostersView->instance(),SIGNAL(contextMenu(IRosterIndex *, Menu *)),SLOT(onRostersViewContextMenu(IRosterIndex *, Menu *)));
    connect(FRostersView->instance(),SIGNAL(labelToolTips(IRosterIndex *, int , QMultiMap<int,QString> &)),
      SLOT(onRosterLabelToolTips(IRosterIndex *, int , QMultiMap<int,QString> &)));
  }
  if (FRostersModel)
  {
    FRostersModel->insertDefaultDataHolder(this);
    connect(this,SIGNAL(discoInfoReceived(const IDiscoInfo &)),SLOT(onDiscoInfoChanged(const IDiscoInfo &)));
    connect(this,SIGNAL(discoInfoRemoved(const IDiscoInfo &)),SLOT(onDiscoInfoChanged(const IDiscoInfo &)));
  }
  if (FTrayManager)
  {
    FTrayManager->addAction(FDiscoMenu->menuAction(),AG_TMTM_DISCOVERY,true);
  }
  if (FMainWindowPlugin)
  {
    ToolBarChanger *changer = FMainWindowPlugin->mainWindow()->topToolBarChanger();
    QToolButton *button = changer->addToolButton(FDiscoMenu->menuAction(),TBG_MWTTB_DISCOVERY,false);
    button->setPopupMode(QToolButton::InstantPopup);
  }

  FDiscoMenu->setEnabled(false);
  return true;
}

bool ServiceDiscovery::startPlugin()
{
  return true;
}

bool ServiceDiscovery::stanzaEdit(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &/*AAccept*/)
{
  if (FSHIPresenceOut.value(AStreamJid)==AHandlerId && !FSelfCaps.value(AStreamJid).ver.isEmpty())
  {
    QDomElement capsElem = AStanza.addElement("c",NS_CAPS);
    capsElem.setAttribute("node",FSelfCaps.value(AStreamJid).node);
    capsElem.setAttribute("ver",FSelfCaps.value(AStreamJid).ver);
    capsElem.setAttribute("hash",FSelfCaps.value(AStreamJid).hash);
  }
  return false;
}

bool ServiceDiscovery::stanzaRead(int AHandlerId, const Jid &AStreamJid, const Stanza &AStanza, bool &AAccept)
{
  bool hooked = false;
  if (FSHIInfo.value(AStreamJid) == AHandlerId)
  {
    QDomElement query = AStanza.firstElement("query",NS_DISCO_INFO);
    IDiscoInfo dinfo = selfDiscoInfo(AStreamJid,query.attribute("node"));

    if (dinfo.error.code > 0)
    {
      AAccept = true;
      Stanza reply = AStanza.replyError(dinfo.error.condition,EHN_DEFAULT,dinfo.error.code,dinfo.error.message);
      FStanzaProcessor->sendStanzaOut(AStreamJid,reply);
    }
    else if (!dinfo.identity.isEmpty() || !dinfo.features.isEmpty() || !dinfo.extensions.isEmpty())
    {
      AAccept = true;
      Stanza reply("iq");
      reply.setTo(AStanza.from()).setId(AStanza.id()).setType("result");
      QDomElement query = reply.addElement("query",NS_DISCO_INFO);
      if (!dinfo.node.isEmpty())
        query.setAttribute("node",dinfo.node);
      discoInfoToElem(dinfo,query);
      FStanzaProcessor->sendStanzaOut(AStreamJid,reply);
    }
  }
  else if (FSHIItems.value(AStreamJid) == AHandlerId)
  {
    IDiscoItems ditems;
    QDomElement query = AStanza.firstElement("query",NS_DISCO_INFO);
    ditems.streamJid = AStreamJid;
    ditems.contactJid = AStanza.from();
    ditems.node = query.attribute("node");
    foreach(IDiscoHandler *AHandler, FDiscoHandlers)
      AHandler->fillDiscoItems(ditems);

    if (ditems.error.code > 0)
    {
      AAccept = true;
      Stanza reply = AStanza.replyError(ditems.error.condition,EHN_DEFAULT,ditems.error.code,ditems.error.message);
      FStanzaProcessor->sendStanzaOut(AStreamJid,reply);
    }
    else if (!ditems.items.isEmpty())
    {
      AAccept = true;
      Stanza reply("iq");
      reply.setTo(AStanza.from()).setId(AStanza.id()).setType("result");
      QDomElement query = reply.addElement("query",NS_DISCO_ITEMS);
      if (!ditems.node.isEmpty())
        query.setAttribute("node",ditems.node);
      foreach(IDiscoItem ditem, ditems.items)
      {
        QDomElement elem = query.appendChild(reply.createElement("item")).toElement();
        elem.setAttribute("jid",ditem.itemJid.eFull());
        if (!ditem.node.isEmpty())
          elem.setAttribute("node",ditem.node);
        if (!ditem.name.isEmpty())
          elem.setAttribute("name",ditem.name);
      }
      FStanzaProcessor->sendStanzaOut(AStreamJid,reply);
    }
  }
  else if (FSHIPresenceIn.value(AStreamJid) == AHandlerId)
  {
    if (AStanza.type().isEmpty())
    {
      Jid contactJid = AStanza.from();
      QDomElement capsElem = AStanza.firstElement("c",NS_CAPS);
      EntityCapabilities newCaps;
      newCaps.entityJid = contactJid;
      newCaps.node = capsElem.attribute("node");
      newCaps.ver = capsElem.attribute("ver");
      newCaps.hash = capsElem.attribute("hash");
      EntityCapabilities oldCaps = FEntityCaps.value(contactJid);
      if (capsElem.isNull() || oldCaps.ver!=newCaps.ver || oldCaps.node!=newCaps.node)
      {
        if (hasEntityCaps(newCaps))
        {
          IDiscoInfo dinfo = loadEntityCaps(newCaps);
          dinfo.streamJid = AStreamJid;
          dinfo.contactJid = contactJid;
          FDiscoInfo[dinfo.contactJid].insert(dinfo.node,dinfo);
          emit discoInfoReceived(dinfo);
        }
        else if (!hasDiscoInfo(contactJid) || discoInfo(contactJid).error.code>0)
        {
          QueuedInfoRequest request;
          request.streamJid = AStreamJid;
          request.contactJid = contactJid;
          //request.node = !newCaps.hash.isEmpty() ? newCaps.node+"#"+newCaps.ver : "";
          appendQueuedRequest(QUEUE_REQUEST_START,request);
        }
        if (!capsElem.isNull() && !newCaps.node.isEmpty() && !newCaps.ver.isEmpty())
          FEntityCaps.insert(contactJid,newCaps);
      }
    }
  }
  return hooked;
}

void ServiceDiscovery::stanzaRequestResult(const Jid &/*AStreamJid*/, const Stanza &AStanza)
{
  if (FInfoRequestsId.contains(AStanza.id()))
  {
    QPair<Jid,QString> jidnode = FInfoRequestsId.take(AStanza.id());
    IDiscoInfo dinfo = parseDiscoInfo(AStanza,jidnode);
    FDiscoInfo[dinfo.contactJid].insert(dinfo.node,dinfo);
    saveEntityCaps(dinfo);
    emit discoInfoReceived(dinfo);
  }
  else if (FItemsRequestsId.contains(AStanza.id()))
  {
    QPair<Jid,QString> jidnode = FItemsRequestsId.take(AStanza.id());
    IDiscoItems ditems = parseDiscoItems(AStanza,jidnode);
    FDiscoItems[ditems.contactJid].insert(ditems.node,ditems);
    emit discoItemsReceived(ditems);
  }
}

void ServiceDiscovery::stanzaRequestTimeout(const Jid &AStreamJid, const QString &AStanzaId)
{
  Q_UNUSED(AStreamJid);
  if (FInfoRequestsId.contains(AStanzaId))
  {
    IDiscoInfo dinfo;
    QPair<Jid,QString> jidnode = FInfoRequestsId.take(AStanzaId);
    ErrorHandler err(ErrorHandler::REMOTE_SERVER_TIMEOUT);
    dinfo.contactJid = jidnode.first;
    dinfo.node = jidnode.second;
    dinfo.error.code = err.code();
    dinfo.error.condition = err.condition();
    dinfo.error.message = err.message();
    FDiscoInfo[dinfo.contactJid].insert(dinfo.node,dinfo);
    emit discoInfoReceived(dinfo);
  }
  else if (FItemsRequestsId.contains(AStanzaId))
  {
    IDiscoItems ditems;
    QPair<Jid,QString> jidnode = FItemsRequestsId.take(AStanzaId);
    ErrorHandler err(ErrorHandler::REMOTE_SERVER_TIMEOUT);
    ditems.contactJid = jidnode.first;
    ditems.node = jidnode.second;
    ditems.error.code = err.code();
    ditems.error.condition = err.condition();
    ditems.error.message = err.message();
    FDiscoItems[ditems.contactJid].insert(ditems.node,ditems);
    emit discoItemsReceived(ditems);
  }
}

void ServiceDiscovery::fillDiscoInfo(IDiscoInfo &ADiscoInfo)
{
  if (ADiscoInfo.node.isEmpty())
  {
    IDiscoIdentity didentity;
    didentity.category = "client";
    didentity.type = "pc";
    didentity.name = CLIENT_NAME;
    ADiscoInfo.identity.append(didentity);

    foreach(IDiscoFeature feature, FDiscoFeatures)
      if (feature.active)
        ADiscoInfo.features.append(feature.var);
  }
}

QList<int> ServiceDiscovery::roles() const
{
  static QList<int> indexRoles = QList<int>()
    << RDR_DISCO_IDENT_CATEGORY << RDR_DISCO_IDENT_TYPE << RDR_DISCO_IDENT_NAME
    << RDR_DISCO_FEATURES;
  return indexRoles;
}

QList<int> ServiceDiscovery::types() const
{
  static QList<int> indexTypes =  QList<int>()
    << RIT_STREAM_ROOT << RIT_CONTACT << RIT_AGENT << RIT_MY_RESOURCE;
  return indexTypes;
}

QVariant ServiceDiscovery::data(const IRosterIndex *AIndex, int ARole) const
{
  Jid contactJid = AIndex->type()==RIT_STREAM_ROOT ? Jid(AIndex->data(RDR_JID).toString()).domain() : AIndex->data(RDR_JID).toString();
  if (hasDiscoInfo(contactJid,""))
  {
    IDiscoInfo dinfo = discoInfo(contactJid,"");
    if (ARole == RDR_DISCO_IDENT_CATEGORY)
      return dinfo.identity.value(0).category;
    else if (ARole == RDR_DISCO_IDENT_TYPE)
      return dinfo.identity.value(0).type;
    else if (ARole == RDR_DISCO_IDENT_NAME)
      return dinfo.identity.value(0).name;
    else if (ARole == RDR_DISCO_FEATURES)
      return dinfo.features;
  }
  return QVariant();
}

bool ServiceDiscovery::rosterIndexClicked(IRosterIndex *AIndex, int /*AOrder*/)
{
  if (AIndex->type() == RIT_AGENT)
  {
    IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->getPresence(AIndex->data(RDR_STREAM_JID).toString()) : NULL;
    if(presence && presence->isOpen())
      showDiscoItems(presence->streamJid(),AIndex->data(RDR_JID).toString(),"");
  }
  return false;
}

IDiscoInfo ServiceDiscovery::selfDiscoInfo(const Jid &AStreamJid, const QString &ANode) const
{
  IDiscoInfo dinfo;
  dinfo.streamJid = AStreamJid;
  dinfo.contactJid = AStreamJid;

  const EntityCapabilities myCaps = FSelfCaps.value(AStreamJid);
  QString capsNode = QString("%1#%2").arg(myCaps.node).arg(myCaps.ver);
  dinfo.node = ANode!=capsNode ? ANode : "";
  
  foreach(IDiscoHandler *handler, FDiscoHandlers)
    handler->fillDiscoInfo(dinfo);

  dinfo.node = ANode;

  return dinfo;
}

void ServiceDiscovery::showDiscoInfo(const Jid &AStreamJid, const Jid &AContactJid, const QString &ANode, QWidget *AParent)
{
  if (FDiscoInfoWindows.contains(AContactJid))
    FDiscoInfoWindows.take(AContactJid)->close();
  DiscoInfoWindow *infoWindow = new DiscoInfoWindow(this,AStreamJid,AContactJid,ANode,AParent);
  connect(infoWindow,SIGNAL(destroyed(QObject *)),SLOT(onDiscoInfoWindowDestroyed(QObject *)));
  FDiscoInfoWindows.insert(AContactJid,infoWindow);
  infoWindow->show();
}

void ServiceDiscovery::showDiscoItems(const Jid &AStreamJid, const Jid &AContactJid, const QString &ANode, QWidget *AParent)
{
  DiscoItemsWindow *itemsWindow = new DiscoItemsWindow(this,AStreamJid,AParent);
  connect(itemsWindow,SIGNAL(windowDestroyed(IDiscoItemsWindow *)),SLOT(onDiscoItemsWindowDestroyed(IDiscoItemsWindow *)));
  FDiscoItemsWindows.append(itemsWindow);
  if (FSettingsPlugin)
  {
    ISettings *settings = FSettingsPlugin->settingsForPlugin(SERVICEDISCOVERY_UUID);
    QString dataId = BDI_ITEMS_GEOMETRY+itemsWindow->streamJid().pBare();
    itemsWindow->restoreGeometry(settings->loadBinaryData(dataId));
  }
  emit discoItemsWindowCreated(itemsWindow);
  itemsWindow->discover(AContactJid,ANode);
  itemsWindow->show();
}

bool ServiceDiscovery::checkDiscoFeature(const Jid &AContactJid, const QString &ANode, const QString &AFeature, bool ADefault)
{
  IDiscoInfo dinfo = discoInfo(AContactJid,ANode);
  return dinfo.error.code>0 || !dinfo.contactJid.isValid() ? ADefault : dinfo.features.contains(AFeature);
}

QList<IDiscoInfo> ServiceDiscovery::findDiscoInfo(const IDiscoIdentity &AIdentity, const QStringList &AFeatures, const IDiscoItem &AParent) const
{
  QList<IDiscoInfo> result;
  QList<Jid> searchJids = AParent.itemJid.isValid() ? QList<Jid>()<<AParent.itemJid : FDiscoInfo.keys();
  foreach(Jid itemJid,searchJids)
  {
    QMap<QString,IDiscoInfo> itemInfos = FDiscoInfo.value(itemJid);
    QList<QString> searchNodes = !AParent.node.isEmpty() ? QList<QString>()<<AParent.node : itemInfos.keys();
    foreach(QString itemNode, searchNodes)
    {
      IDiscoInfo itemInfo = itemInfos.value(itemNode);
      if (compareIdentities(itemInfo.identity,AIdentity) && compareFeatures(itemInfo.features,AFeatures))
        result.append(itemInfo);
    }
  }
  return result;
}

QIcon ServiceDiscovery::identityIcon(const QList<IDiscoIdentity> &AIdentity) const
{
  QIcon icon;
  IconStorage *storage = IconStorage::staticStorage(RSR_STORAGE_SERVICEICONS);
  for (int i=0; icon.isNull() && i<AIdentity.count(); i++)
  {
    icon = storage->getIcon(AIdentity.at(i).category +"/"+ AIdentity.at(i).type);
    if (icon.isNull())
      icon = storage->getIcon(AIdentity.at(i).category);
  }
  if (icon.isNull())
    icon = storage->getIcon(SRI_SERVICE);
  return icon;
}

QIcon ServiceDiscovery::serviceIcon(const Jid AItemJid, const QString &ANode) const
{
  QIcon icon;
  IDiscoInfo info = discoInfo(AItemJid,ANode);
  IconStorage *storage = IconStorage::staticStorage(RSR_STORAGE_SERVICEICONS);
  if (FInfoRequestsId.values().contains(qMakePair<Jid,QString>(AItemJid,ANode)))
  {
    icon = storage->getIcon(SRI_SERVICE_WAIT);
  }
  else if (info.identity.isEmpty())
  {
    icon = storage->getIcon(info.error.code==-1 ? SRI_SERVICE_EMPTY : SRI_SERVICE_ERROR);
  }
  else 
  {
    icon = identityIcon(info.identity);
  }
  return icon;
}

void ServiceDiscovery::insertDiscoHandler(IDiscoHandler *AHandler)
{
  if (!FDiscoHandlers.contains(AHandler))
  {
    FDiscoHandlers.append(AHandler);
    emit discoHandlerInserted(AHandler);
  }
}

void ServiceDiscovery::removeDiscoHandler(IDiscoHandler *AHandler)
{
  if (FDiscoHandlers.contains(AHandler))
  {
    FDiscoHandlers.removeAt(FDiscoHandlers.indexOf(AHandler));
    emit discoHandlerRemoved(AHandler);
  }
}

bool ServiceDiscovery::hasFeatureHandler(const QString &AFeature) const
{
  return FFeatureHandlers.contains(AFeature);
}

void ServiceDiscovery::insertFeatureHandler(const QString &AFeature, IDiscoFeatureHandler *AHandler, int AOrder)
{
  if (!FFeatureHandlers.value(AFeature).values().contains(AHandler))
  {
    FFeatureHandlers[AFeature].insertMulti(AOrder,AHandler);
    emit featureHandlerInserted(AFeature,AHandler);
  }
}

bool ServiceDiscovery::execFeatureHandler(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo)
{
  QList<IDiscoFeatureHandler *> handlers = FFeatureHandlers.value(AFeature).values();
  foreach(IDiscoFeatureHandler *handler, handlers)
    if (handler->execDiscoFeature(AStreamJid,AFeature,ADiscoInfo))
      return true;
  return false;
}

QList<Action *> ServiceDiscovery::createFeatureActions(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo, QWidget *AParent)
{
  QList<Action *> actions;
  QList<IDiscoFeatureHandler *> handlers = FFeatureHandlers.value(AFeature).values();
  foreach(IDiscoFeatureHandler *handler, handlers)
  {
    Action *action = handler->createDiscoFeatureAction(AStreamJid,AFeature,ADiscoInfo,AParent);
    if (action)
      actions.append(action);
  }
  return actions;
}

void ServiceDiscovery::removeFeatureHandler(const QString &AFeature, IDiscoFeatureHandler *AHandler)
{
  if (FFeatureHandlers.value(AFeature).values().contains(AHandler))
  {
    FFeatureHandlers[AFeature].remove(FFeatureHandlers[AFeature].key(AHandler),AHandler);
    if (FFeatureHandlers.value(AFeature).isEmpty())
      FFeatureHandlers.remove(AFeature);
    emit featureHandlerRemoved(AFeature,AHandler);
  }
}

void ServiceDiscovery::insertDiscoFeature(const IDiscoFeature &AFeature)
{
  if (!AFeature.var.isEmpty())
  {
    removeDiscoFeature(AFeature.var);
    FDiscoFeatures.insert(AFeature.var,AFeature);
    QTimer::singleShot(0,this,SLOT(onSelfCapsChanged()));
    emit discoFeatureInserted(AFeature);
  }
}

QList<QString> ServiceDiscovery::discoFeatures() const
{
  return FDiscoFeatures.keys();
}

IDiscoFeature ServiceDiscovery::discoFeature(const QString &AFeatureVar) const
{
  return FDiscoFeatures.value(AFeatureVar);
}

void ServiceDiscovery::removeDiscoFeature(const QString &AFeatureVar)
{
  if (FDiscoFeatures.contains(AFeatureVar))
  {
    IDiscoFeature dfeature = FDiscoFeatures.take(AFeatureVar);
    QTimer::singleShot(0,this,SLOT(onSelfCapsChanged()));
    emit discoFeatureRemoved(dfeature);
  }
}

bool ServiceDiscovery::hasDiscoInfo(const Jid &AContactJid, const QString &ANode) const
{
  return FDiscoInfo.value(AContactJid).contains(ANode);
}

QList<Jid> ServiceDiscovery::discoInfoContacts() const
{
  return FDiscoInfo.keys();
}

QList<QString> ServiceDiscovery::dicoInfoContactNodes(const Jid &AContactJid) const
{
  return FDiscoInfo.value(AContactJid).keys();
}

IDiscoInfo ServiceDiscovery::discoInfo(const Jid &AContactJid, const QString &ANode) const
{
  return FDiscoInfo.value(AContactJid).value(ANode);
}

bool ServiceDiscovery::requestDiscoInfo(const Jid &AStreamJid, const Jid &AContactJid, const QString &ANode)
{
  bool sended = false;
  QPair<Jid,QString> jidnode(AContactJid,ANode);
  if (FInfoRequestsId.values().contains(jidnode))
  {
    sended = true;
  }
  else if (FStanzaProcessor && AStreamJid.isValid() && AContactJid.isValid())
  {
    Stanza iq("iq");
    iq.setTo(AContactJid.eFull()).setId(FStanzaProcessor->newId()).setType("get");
    QDomElement query =  iq.addElement("query",NS_DISCO_INFO);
    if (!ANode.isEmpty())
      query.setAttribute("node",ANode);
    sended = FStanzaProcessor->sendStanzaRequest(this,AStreamJid,iq,DISCO_TIMEOUT);
    if (sended)
      FInfoRequestsId.insert(iq.id(),jidnode);
  }
  return sended;
}

void ServiceDiscovery::removeDiscoInfo(const Jid &AContactJid, const QString &ANode)
{
  if (hasDiscoInfo(AContactJid,ANode))
  {
    QMap<QString,IDiscoInfo> &dnodeInfo = FDiscoInfo[AContactJid];
    IDiscoInfo dinfo = dnodeInfo.take(ANode);
    if (dnodeInfo.isEmpty())
      FDiscoInfo.remove(AContactJid);
    emit discoInfoRemoved(dinfo);
  }
}

int ServiceDiscovery::findIdentity(const QList<IDiscoIdentity> &AIdentity, const QString &ACategory, const QString &AType) const
{
  int index = -1;
  for (int i=0; index<0 && i<AIdentity.count(); i++)
    if ((ACategory.isEmpty() || AIdentity.at(i).category == ACategory) && (AType.isEmpty() || AIdentity.at(i).type == AType))
      index = i;
  return index;
}

bool ServiceDiscovery::hasDiscoItems(const Jid &AContactJid, const QString &ANode) const
{
  return FDiscoItems.value(AContactJid).contains(ANode);
}

QList<Jid> ServiceDiscovery::discoItemsContacts() const
{
  return FDiscoItems.keys();
}

QList<QString> ServiceDiscovery::dicoItemsContactNodes(const Jid &AContactJid) const
{
  return FDiscoItems.value(AContactJid).keys();
}

IDiscoItems ServiceDiscovery::discoItems(const Jid &AContactJid, const QString &ANode) const
{
  return FDiscoItems.value(AContactJid).value(ANode);
}

bool ServiceDiscovery::requestDiscoItems(const Jid &AStreamJid, const Jid &AContactJid, const QString &ANode)
{
  bool sended = false;
  QPair<Jid,QString> jidnode(AContactJid,ANode);
  if (FItemsRequestsId.values().contains(jidnode))
  {
    sended = true;
  }
  else if (FStanzaProcessor && AStreamJid.isValid() && AContactJid.isValid())
  {
    Stanza iq("iq");
    iq.setTo(AContactJid.eFull()).setId(FStanzaProcessor->newId()).setType("get");
    QDomElement query =  iq.addElement("query",NS_DISCO_ITEMS);
    if (!ANode.isEmpty())
      query.setAttribute("node",ANode);
    sended = FStanzaProcessor->sendStanzaRequest(this,AStreamJid,iq,DISCO_TIMEOUT);
    if (sended)
      FItemsRequestsId.insert(iq.id(),jidnode);
  }
  return sended;
}

void ServiceDiscovery::removeDiscoItems(const Jid &AContactJid, const QString &ANode)
{
  if (hasDiscoItems(AContactJid,ANode))
  {
    QMap<QString,IDiscoItems> &dnodeItems = FDiscoItems[AContactJid];
    IDiscoItems ditems = dnodeItems.take(ANode);
    if (dnodeItems.isEmpty())
      FDiscoItems.remove(AContactJid);
    emit discoItemsRemoved(ditems);
  }
}

void ServiceDiscovery::discoInfoToElem(const IDiscoInfo &AInfo, QDomElement &AElem) const
{
  QDomDocument doc = AElem.ownerDocument();
  foreach(IDiscoIdentity identity, AInfo.identity)
  {
    QDomElement elem = AElem.appendChild(doc.createElement("identity")).toElement();
    elem.setAttribute("category",identity.category);
    elem.setAttribute("type",identity.type);
    if (!identity.name.isEmpty())
      elem.setAttribute("name",identity.name);
    if (!identity.lang.isEmpty())
      elem.setAttribute("xml:lang",identity.lang);

  }
  foreach(QString feature, AInfo.features)
  {
    QDomElement elem = AElem.appendChild(doc.createElement("feature")).toElement();
    elem.setAttribute("var",feature);
  }
  if (FDataForms)
  {
    foreach(IDataForm form, AInfo.extensions)
    {
      FDataForms->xmlForm(form,AElem);
    }
  }
}

void ServiceDiscovery::discoInfoFromElem(const QDomElement &AElem, IDiscoInfo &AInfo) const
{
  AInfo.identity.clear();
  QDomElement elem = AElem.firstChildElement("identity");
  while (!elem.isNull())
  {
    IDiscoIdentity identity;
    identity.category = elem.attribute("category");
    identity.type = elem.attribute("type");
    identity.lang = elem.attribute("lang");
    identity.name = elem.attribute("name");
    AInfo.identity.append(identity);
    elem = elem.nextSiblingElement("identity");
  }

  AInfo.features.clear();
  elem = AElem.firstChildElement("feature");
  while (!elem.isNull())
  {
    QString feature = elem.attribute("var");
    if (!feature.isEmpty() && !AInfo.features.contains(feature))
      AInfo.features.append(feature);
    elem = elem.nextSiblingElement("feature");
  }

  if (FDataForms)
  {
    AInfo.extensions.clear();
    elem = AElem.firstChildElement("x");
    while (!elem.isNull())
    {
      if (elem.namespaceURI()==NS_JABBER_DATA)
      {
        IDataForm form = FDataForms->dataForm(elem);
        AInfo.extensions.append(form);
      }
      elem = elem.nextSiblingElement("x");
    }
  }
}

IDiscoInfo ServiceDiscovery::parseDiscoInfo(const Stanza &AStanza, const QPair<Jid,QString> &AJidNode) const
{
  IDiscoInfo result;
  result.streamJid = AStanza.to();
  result.contactJid = AJidNode.first;
  result.node = AJidNode.second;

  QDomElement query = AStanza.firstElement("query",NS_DISCO_INFO);
  if (AStanza.type() == "error")
  {
    ErrorHandler err(AStanza.element());
    result.error.code = err.code();
    result.error.condition = err.condition();
    result.error.message = err.message();
  }
  else
  {
    discoInfoFromElem(query,result);
  }
  return result;
}

IDiscoItems ServiceDiscovery::parseDiscoItems(const Stanza &AStanza, const QPair<Jid,QString> &AJidNode) const
{
  IDiscoItems result;
  result.streamJid = AStanza.to();
  result.contactJid = AJidNode.first;
  result.node = AJidNode.second;

  QDomElement query = AStanza.firstElement("query",NS_DISCO_ITEMS);
  if (AStanza.type() == "error")
  {
    ErrorHandler err(AStanza.element());
    result.error.code = err.code();
    result.error.condition = err.condition();
    result.error.message = err.message();
  }
  else
  {
    QDomElement elem = query.firstChildElement("item");
    while (!elem.isNull())
    {
      IDiscoItem ditem;
      ditem.itemJid = elem.attribute("jid");
      ditem.node = elem.attribute("node");
      ditem.name = elem.attribute("name");
      result.items.append(ditem);
      elem = elem.nextSiblingElement("item");
    }
  }
  return result;
}

void ServiceDiscovery::registerFeatures()
{
  IconStorage *storage = IconStorage::staticStorage(RSR_STORAGE_MENUICONS);
  IDiscoFeature dfeature;

  dfeature.var = NS_DISCO;
  dfeature.active = false;
  dfeature.icon = storage->getIcon(MNI_SDISCOVERY_DISCOINFO);
  dfeature.name = tr("Service Discovery");
  dfeature.actionName = "";
  dfeature.description = tr("Discover information and items associated with a Jabber Entity");
  insertDiscoFeature(dfeature);

  dfeature.var = NS_DISCO_INFO;
  dfeature.active = true;
  dfeature.icon = storage->getIcon(MNI_SDISCOVERY_DISCOINFO);
  dfeature.name = tr("Discovery information");
  dfeature.actionName = "";
  dfeature.description = tr("Discover information about another entity on the network");
  insertDiscoFeature(dfeature);

  dfeature.var = NS_DISCO_ITEMS;
  dfeature.active = false;
  dfeature.icon = storage->getIcon(MNI_SDISCOVERY_DISCOINFO);
  dfeature.name = tr("Discovery items");
  dfeature.actionName = "";
  dfeature.description = tr("Discover the items associated with a Jabber Entity");
  insertDiscoFeature(dfeature);

  dfeature.var = NS_DISCO_PUBLISH;
  dfeature.active = false;
  dfeature.icon = QIcon();
  dfeature.name = tr("Publish items");
  dfeature.actionName = "";
  dfeature.description = tr("Publish user defined items to server");
  insertDiscoFeature(dfeature);

  dfeature.var = NS_CAPS;
  dfeature.active = true;
  dfeature.icon = QIcon();
  dfeature.name = tr("Entity Capabilities");
  dfeature.actionName = "";
  dfeature.description = tr("Enables the capabilities information caching within a session or across sessions");
  insertDiscoFeature(dfeature);

  dfeature.var = "jid\\20escaping";
  dfeature.active = true;
  dfeature.icon = QIcon();
  dfeature.name = tr("JID Escaping");
  dfeature.actionName = "";
  dfeature.description = tr("Enables the display of Jabber Identifiers with disallowed characters");
  insertDiscoFeature(dfeature);
}

void ServiceDiscovery::appendQueuedRequest(const QDateTime &ATimeStart, const QueuedInfoRequest &ARequest)
{
  QMultiMap<QDateTime,QueuedInfoRequest>::const_iterator it = FQueuedRequests.constBegin();
  while (it!=FQueuedRequests.constEnd())
  {
    if (it.value().contactJid==ARequest.contactJid && it.value().node==ARequest.node)
      return;
    it++;
  }
  FQueuedRequests.insert(ATimeStart,ARequest);
  if (!FQueueTimer.isActive())
    FQueueTimer.start();
}

void ServiceDiscovery::removeQueuedRequest(const QueuedInfoRequest &ARequest)
{
  QMultiMap<QDateTime,QueuedInfoRequest>::iterator it = FQueuedRequests.begin();
  while (it!=FQueuedRequests.end())
  {
    if (
          (ARequest.streamJid.isEmpty() || it.value().streamJid == ARequest.streamJid) &&
          (ARequest.contactJid.isEmpty() || it.value().contactJid == ARequest.contactJid) &&
          (ARequest.node.isEmpty() || it.value().node == ARequest.node)
       )
      it = FQueuedRequests.erase(it);
    else
      it++;
  }
}

bool ServiceDiscovery::hasEntityCaps(const EntityCapabilities &ACaps) const
{
  return QFile::exists(capsFileName(ACaps,false)) || QFile::exists(capsFileName(ACaps,true));
}

QString ServiceDiscovery::capsFileName(const EntityCapabilities &ACaps, bool AForJid) const
{
  static bool entered = false;
  static QDir dir(FPluginManager->homePath());
  if (!entered)
  {
    entered = true;
    if (!dir.exists(CAPS_DIRNAME))
      dir.mkdir(CAPS_DIRNAME);
    dir.cd(CAPS_DIRNAME);
  }

  QString hashString = ACaps.hash.isEmpty() ? ACaps.node+ACaps.ver : ACaps.ver+ACaps.hash;
  hashString += AForJid ? ACaps.entityJid.pFull() : "";
  QString fileName = QCryptographicHash::hash(hashString.toUtf8(),QCryptographicHash::Md5).toHex().toLower() + ".xml";
  return dir.absoluteFilePath(fileName);
}

IDiscoInfo ServiceDiscovery::loadEntityCaps(const EntityCapabilities &ACaps) const
{
  QMap<Jid,EntityCapabilities>::const_iterator it = FEntityCaps.constBegin();
  while(it!=FEntityCaps.constEnd())
  {
    EntityCapabilities caps = it.value();
    if ((!ACaps.hash.isEmpty() || caps.node==ACaps.node) && caps.ver==ACaps.ver && caps.hash==ACaps.hash && hasDiscoInfo(it.key()))
    {
      IDiscoInfo dinfo = discoInfo(it.key());
      if (caps.ver == calcCapsHash(dinfo,caps.hash))
        return dinfo;
    }
    it++;
  }

  IDiscoInfo dinfo;
  QString fileName = capsFileName(ACaps,true);
  if (!QFile::exists(fileName))
    fileName = capsFileName(ACaps,false);
  QFile capsFile(fileName);
  if (capsFile.exists() && capsFile.open(QIODevice::ReadOnly))
  {
    QDomDocument doc;
    doc.setContent(capsFile.readAll(),true);
    capsFile.close();
    QDomElement capsElem = doc.documentElement();
    discoInfoFromElem(capsElem,dinfo);
  }
  return dinfo;
}

bool ServiceDiscovery::saveEntityCaps(const IDiscoInfo &AInfo) const
{
  if (AInfo.error.code==-1 && FEntityCaps.contains(AInfo.contactJid))
  {
    EntityCapabilities caps = FEntityCaps.value(AInfo.contactJid);
    QString capsNode = QString("%1#%2").arg(caps.node).arg(caps.ver);
    if (AInfo.node.isEmpty() || AInfo.node==capsNode)
    {
      if (!hasEntityCaps(caps))
      {
        bool checked = (caps.ver==calcCapsHash(AInfo,caps.hash));
        QDomDocument doc;
        QDomElement capsElem = doc.appendChild(doc.createElement(CAPS_FILE_TAG_NAME)).toElement();
        capsElem.setAttribute("node",caps.node);
        capsElem.setAttribute("ver",caps.ver);
        capsElem.setAttribute("hash",caps.hash);
        if (!checked)
          capsElem.setAttribute("jid",caps.entityJid.pFull());
        discoInfoToElem(AInfo,capsElem);
        QFile capsFile(capsFileName(caps,!checked));
        if (capsFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
        {
          capsFile.write(doc.toByteArray());
          capsFile.close();
        }
      }
      return true;
    }
  }
  return false;
}

QString ServiceDiscovery::calcCapsHash(const IDiscoInfo &AInfo, const QString &AHash) const
{
  if (AHash==CAPS_HASH_SHA1 || AHash==CAPS_HASH_MD5)
  {
    QStringList hashList;
    QStringList sortList;

    foreach(IDiscoIdentity identity, AInfo.identity)
      sortList.append(identity.category+"/"+identity.type+"/"+identity.lang+"/"+identity.name);
    qSort(sortList);
    hashList += sortList;
    
    sortList = AInfo.features;
    qSort(sortList);
    hashList += sortList;

    if (FDataForms && !AInfo.extensions.isEmpty())
    {
      QMultiMap<QString, int> sortForms;
      for (int index=0; index<AInfo.extensions.count();index++)
        sortForms.insertMulti(FDataForms->fieldValue("FORM_TYPE",AInfo.extensions.at(index).fields).toString(),index);
      
      QMultiMap<QString, int>::const_iterator iforms = sortForms.constBegin();
      while (iforms != sortForms.constEnd())
      {
        hashList += iforms.key();
        QMultiMap<QString,QStringList> sortFields;
        foreach(IDataField field, AInfo.extensions.at(iforms.value()).fields)
        {
          if (field.var != "FORM_TYPE")
          {
            QStringList values;
            if (field.value.type() == QVariant::StringList)
              values = field.value.toStringList();
            else if (field.value.type() == QVariant::Bool)
              values +=(field.value.toBool() ? "1" : "0");
            else
              values += field.value.toString();
            qSort(values);
            sortFields.insertMulti(field.var,values);
          }
        }
        QMultiMap<QString,QStringList>::const_iterator ifields = sortFields.constBegin();
        while (ifields != sortFields.constEnd())
        {
          hashList += ifields.key();
          hashList += ifields.value();
          ifields++;
        }
        iforms++;
      }
    }
    hashList.append("");
    QByteArray hashData = hashList.join("<").toUtf8();
    return QCryptographicHash::hash(hashData, AHash==CAPS_HASH_SHA1 ? QCryptographicHash::Sha1 : QCryptographicHash::Md5).toBase64();
  }
  return QString::null;
}

bool ServiceDiscovery::compareIdentities(const QList<IDiscoIdentity> &AIdentities, const IDiscoIdentity &AWith) const
{
  foreach(IDiscoIdentity identity,AIdentities)
    if (
        (AWith.category.isEmpty() || AWith.category==identity.category) &&
        (AWith.type.isEmpty() || AWith.type==identity.type) &&
        (AWith.lang.isEmpty() || AWith.lang==identity.lang) &&
        (AWith.name.isEmpty() || AWith.name==identity.name)
       )
      return true;
  return false;
}

bool ServiceDiscovery::compareFeatures(const QStringList &AFeatures, const QStringList &AWith) const
{
  if (!AWith.isEmpty())
    foreach(QString feature, AWith)
      if (!AFeatures.contains(feature))
        return false;
  return true;
}

Action *ServiceDiscovery::createDiscoInfoAction(const Jid &AStreamJid, const Jid &AContactJid, const QString &ANode, QObject *AParent) const
{
  Action *action = new Action(AParent);
  action->setText(tr("Discovery Info"));
  action->setIcon(RSR_STORAGE_MENUICONS,MNI_SDISCOVERY_DISCOINFO);
  action->setData(ADR_STREAMJID,AStreamJid.full());
  action->setData(ADR_CONTACTJID,AContactJid.full());
  action->setData(ADR_NODE,ANode);
  connect(action,SIGNAL(triggered(bool)),SLOT(onShowDiscoInfoByAction(bool)));
  return action;
}

Action *ServiceDiscovery::createDiscoItemsAction(const Jid &AStreamJid, const Jid &AContactJid, const QString &ANode, QObject *AParent) const
{
  Action *action = new Action(AParent);
  action->setText(tr("Service Discovery"));
  action->setIcon(RSR_STORAGE_MENUICONS,MNI_SDISCOVERY_DISCOVER);
  action->setData(ADR_STREAMJID,AStreamJid.full());
  action->setData(ADR_CONTACTJID,AContactJid.full());
  action->setData(ADR_NODE,ANode);
  connect(action,SIGNAL(triggered(bool)),SLOT(onShowDiscoItemsByAction(bool)));
  return action;
}

void ServiceDiscovery::onStreamStateChanged(const Jid &AStreamJid, bool AStateOnline)
{
  if (AStateOnline)
  {
    Action *action = new Action(FDiscoMenu);
    action->setText(AStreamJid.domain());
    action->setIcon(RSR_STORAGE_MENUICONS,MNI_SDISCOVERY_DISCOVER);
    action->setData(ADR_STREAMJID,AStreamJid.full());
    action->setData(ADR_CONTACTJID,AStreamJid.domain());
    action->setData(ADR_NODE,QString(""));
    connect(action,SIGNAL(triggered(bool)),SLOT(onShowDiscoItemsByAction(bool)));
    FDiscoMenu->addAction(action,AG_DEFAULT,true);
    FDiscoMenu->setEnabled(true);

    Jid streamDomane = AStreamJid.domain();
    if (!hasDiscoInfo(streamDomane) || discoInfo(streamDomane).error.code>0)
      requestDiscoInfo(AStreamJid,streamDomane);
    if (!hasDiscoItems(streamDomane) || discoItems(streamDomane).error.code>0)
      requestDiscoItems(AStreamJid,streamDomane);

    IRoster *roster = FRosterPlugin->getRoster(AStreamJid);
    QList<IRosterItem> ritems = roster!=NULL ? roster->rosterItems() : QList<IRosterItem>();
    foreach(IRosterItem ritem, ritems)
    {
      if (ritem.itemJid.node().isEmpty() && (!hasDiscoInfo(ritem.itemJid) || discoInfo(ritem.itemJid).error.code>0))
      {
        QueuedInfoRequest request;
        request.streamJid = AStreamJid;
        request.contactJid = ritem.itemJid;
        appendQueuedRequest(QUEUE_REQUEST_START,request);
      }
    }
  }
  else
  {
    QMultiHash<int,QVariant> data;
    data.insert(ADR_STREAMJID,AStreamJid.full());
    Action *action = FDiscoMenu->findActions(data).value(0,NULL);
    if (action)
    {
      FDiscoMenu->removeAction(action);
      FDiscoMenu->setEnabled(!FDiscoMenu->isEmpty());
    }

    QueuedInfoRequest request;
    request.streamJid = AStreamJid;
    removeQueuedRequest(request);
  }
}

void ServiceDiscovery::onContactStateChanged(const Jid &/*AStreamJid*/, const Jid &AContactJid, bool AStateOnline)
{
  if (!AStateOnline)
  {
    if (!AContactJid.node().isEmpty())
    {
      QueuedInfoRequest request;
      request.contactJid = AContactJid;
      removeQueuedRequest(request);
      removeDiscoInfo(AContactJid);
    }
    FEntityCaps.remove(AContactJid);
  }
}

void ServiceDiscovery::onMultiUserPresence(IMultiUser *AUser, int AShow, const QString &AStatus)
{
  Q_UNUSED(AStatus);
  if (AShow==IPresence::Offline || AShow==IPresence::Error)
  {
    bool isSingleUser = true;
    foreach(IMultiUserChat *mchat, FMultiUserChatPlugin->multiUserChats())
    {
      IMultiUser *muser = mchat->userByNick(AUser->nickName());
      if (muser!=NULL && muser!=AUser && mchat->roomJid()==AUser->roomJid())
      {
        isSingleUser = false;
        break;
      }
    }
    if (isSingleUser)
    {
      QueuedInfoRequest request;
      request.contactJid = AUser->contactJid();
      removeQueuedRequest(request);
      removeDiscoInfo(AUser->contactJid());
      FEntityCaps.remove(AUser->contactJid());
    }
  }
}

void ServiceDiscovery::onMultiUserChatCreated(IMultiUserChat *AMultiChat)
{
  connect(AMultiChat->instance(), SIGNAL(userPresence(IMultiUser *, int, const QString &)), SLOT(onMultiUserPresence(IMultiUser *, int, const QString &)));
}

void ServiceDiscovery::onRosterItemReceived(IRoster *ARoster, const IRosterItem &ARosterItem)
{
  if (ARosterItem.itemJid.node().isEmpty() && ARoster->isOpen() && !hasDiscoInfo(ARosterItem.itemJid))
  {
    QueuedInfoRequest request;
    request.streamJid = ARoster->streamJid();
    request.contactJid = ARosterItem.itemJid;
    appendQueuedRequest(QUEUE_REQUEST_START,request);
  }
}

void ServiceDiscovery::onStreamOpened(IXmppStream *AXmppStream)
{
  EntityCapabilities &myCaps = FSelfCaps[AXmppStream->streamJid()];
  myCaps.entityJid = AXmppStream->streamJid();
  myCaps.node = CLIENT_HOME_PAGE;
  myCaps.hash = CAPS_HASH_SHA1;
  myCaps.ver = calcCapsHash(selfDiscoInfo(myCaps.entityJid),myCaps.hash);

  if (FStanzaProcessor)
  {
    IStanzaHandle shandle;
    shandle.handler = this;
    shandle.order = SHO_DEFAULT;
    shandle.direction = IStanzaHandle::DirectionIn;
    shandle.streamJid = AXmppStream->streamJid();

    shandle.conditions.append(SHC_DISCO_INFO);
    FSHIInfo.insert(shandle.streamJid,FStanzaProcessor->insertStanzaHandle(shandle));

    shandle.conditions.clear();
    shandle.conditions.append(SHC_DISCO_ITEMS);
    FSHIItems.insert(shandle.streamJid,FStanzaProcessor->insertStanzaHandle(shandle));

    shandle.conditions.clear();
    shandle.conditions.append(SHC_PRESENCE);
    shandle.direction = IStanzaHandle::DirectionOut;
    FSHIPresenceOut.insert(shandle.streamJid,FStanzaProcessor->insertStanzaHandle(shandle));

    shandle.order = SHO_PI_SERVICEDISCOVERY;
    shandle.direction = IStanzaHandle::DirectionIn;
    FSHIPresenceIn.insert(shandle.streamJid,FStanzaProcessor->insertStanzaHandle(shandle));
  }
}

void ServiceDiscovery::onStreamClosed(IXmppStream *AXmppStream)
{
  FSelfCaps.remove(AXmppStream->streamJid());

  if (FStanzaProcessor)
  {
    FStanzaProcessor->removeStanzaHandle(FSHIInfo.take(AXmppStream->streamJid()));
    FStanzaProcessor->removeStanzaHandle(FSHIItems.take(AXmppStream->streamJid()));
    FStanzaProcessor->removeStanzaHandle(FSHIPresenceIn.take(AXmppStream->streamJid()));
    FStanzaProcessor->removeStanzaHandle(FSHIPresenceOut.take(AXmppStream->streamJid()));
  }
}

void ServiceDiscovery::onStreamRemoved(IXmppStream *AXmppStream)
{
  foreach(DiscoInfoWindow *infoWindow, FDiscoInfoWindows)
    if (infoWindow->streamJid() == AXmppStream->streamJid())
      infoWindow->deleteLater();

  foreach(DiscoItemsWindow *itemsWindow, FDiscoItemsWindows)
    if (itemsWindow->streamJid() == AXmppStream->streamJid())
      itemsWindow->deleteLater();
}

void ServiceDiscovery::onStreamJidChanged(IXmppStream *AXmppStream, const Jid &ABefour)
{
  QMultiHash<int,QVariant> data;
  data.insert(ADR_STREAMJID,AXmppStream->streamJid().full());
  Action *action = FDiscoMenu->findActions(data).value(0,NULL);
  if (action)
  {
    action->setData(ADR_STREAMJID,AXmppStream->streamJid().full());
    action->setData(ADR_CONTACTJID,AXmppStream->streamJid().domain());
  }
  emit streamJidChanged(ABefour,AXmppStream->streamJid());
}

void ServiceDiscovery::onRostersViewContextMenu(IRosterIndex *AIndex, Menu *AMenu)
{
  int itype = AIndex->type();
  if (itype == RIT_STREAM_ROOT || itype == RIT_CONTACT || itype == RIT_AGENT || itype == RIT_MY_RESOURCE)
  {
    Jid streamJid = AIndex->data(RDR_STREAM_JID).toString();
    Jid contactJid = itype == RIT_STREAM_ROOT ? Jid(AIndex->data(RDR_JID).toString()).domain() : AIndex->data(RDR_JID).toString();

    IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->getPresence(streamJid) : NULL;
    if (presence && presence->isOpen())
    {
      Action *action = createDiscoInfoAction(streamJid, contactJid, QString::null, AMenu);
      AMenu->addAction(action,AG_RVCM_DISCOVERY,true);

      if (itype == RIT_STREAM_ROOT || itype == RIT_AGENT)
      {
        action = createDiscoItemsAction(streamJid, contactJid, QString::null, AMenu);
        AMenu->addAction(action,AG_RVCM_DISCOVERY,true);
      }
    }

    IDiscoInfo dinfo = discoInfo(contactJid);
    foreach(QString feature, dinfo.features)
    {
      foreach(Action *action, createFeatureActions(presence->streamJid(),feature,dinfo,AMenu))
        AMenu->addAction(action,AG_RVCM_DISCOVERY_FEATURES,true);
    }
  }
}

void ServiceDiscovery::onRosterLabelToolTips(IRosterIndex *AIndex, int ALabelId, QMultiMap<int,QString> &AToolTips)
{
  if (ALabelId == RLID_DISPLAY && types().contains(AIndex->type()))
  {
    Jid contactJid = AIndex->type()==RIT_STREAM_ROOT ? Jid(AIndex->data(RDR_JID).toString()).domain() : AIndex->data(RDR_JID).toString();
    if (hasDiscoInfo(contactJid,""))
    {
      IDiscoInfo dinfo = discoInfo(contactJid,"");
      foreach(IDiscoIdentity identity, dinfo.identity)
        if (identity.category != DIC_CLIENT)
          AToolTips.insertMulti(RTTO_DISCO_IDENTITY,tr("Categoty: %1; Type: %2").arg(identity.category).arg(identity.type));
    }
  }
}

void ServiceDiscovery::onMultiUserContextMenu(IMultiUserChatWindow *AWindow, IMultiUser *AUser, Menu *AMenu)
{
  Action *action = createDiscoInfoAction(AWindow->streamJid(), AUser->contactJid(), QString::null, AMenu);
  AMenu->addAction(action, AG_MUCM_DISCOVERY, true);
}

void ServiceDiscovery::onShowDiscoInfoByAction(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    Jid streamJid = action->data(ADR_STREAMJID).toString();
    Jid contactJid = action->data(ADR_CONTACTJID).toString();
    QString node = action->data(ADR_NODE).toString();
    showDiscoInfo(streamJid,contactJid,node);
  }
}

void ServiceDiscovery::onShowDiscoItemsByAction(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    Jid streamJid = action->data(ADR_STREAMJID).toString();
    Jid contactJid = action->data(ADR_CONTACTJID).toString();
    QString node = action->data(ADR_NODE).toString();
    showDiscoItems(streamJid,contactJid,node);
  }
}

void ServiceDiscovery::onDiscoInfoReceived(const IDiscoInfo &ADiscoInfo)
{
  QueuedInfoRequest request;
  request.contactJid = ADiscoInfo.contactJid;
  request.node = ADiscoInfo.node;
  removeQueuedRequest(request);
}

void ServiceDiscovery::onDiscoInfoChanged(const IDiscoInfo &ADiscoInfo)
{
  QMultiHash<int,QVariant> dataValues;
  dataValues.insertMulti(RDR_TYPE,RIT_CONTACT);
  dataValues.insertMulti(RDR_TYPE,RIT_AGENT);
  dataValues.insertMulti(RDR_TYPE,RIT_MY_RESOURCE);
  dataValues.insertMulti(RDR_PJID,ADiscoInfo.contactJid.pFull());
  IRosterIndexList indexList = FRostersModel->rootIndex()->findChild(dataValues,true);
  foreach(Jid streamJid, FRostersModel->streams())
    if (streamJid.pDomain() == ADiscoInfo.contactJid.pDomain())
      indexList.append(FRostersModel->streamRoot(streamJid));
  foreach(IRosterIndex *index, indexList)
  {
    emit dataChanged(index,RDR_DISCO_IDENT_CATEGORY);
    emit dataChanged(index,RDR_DISCO_IDENT_TYPE);
    emit dataChanged(index,RDR_DISCO_IDENT_NAME);
    emit dataChanged(index,RDR_DISCO_FEATURES);
  }
}

void ServiceDiscovery::onDiscoInfoWindowDestroyed(QObject *AObject)
{
  DiscoInfoWindow *infoWindow = static_cast<DiscoInfoWindow *>(AObject);
  FDiscoInfoWindows.remove(FDiscoInfoWindows.key(infoWindow));
}

void ServiceDiscovery::onDiscoItemsWindowDestroyed(IDiscoItemsWindow *AWindow)
{
  DiscoItemsWindow *itemsWindow = static_cast<DiscoItemsWindow *>(AWindow->instance());
  if (itemsWindow && FSettingsPlugin)
  {
    ISettings *settings = FSettingsPlugin->settingsForPlugin(SERVICEDISCOVERY_UUID);
    QString dataId = BDI_ITEMS_GEOMETRY+itemsWindow->streamJid().pBare();
    settings->saveBinaryData(dataId,itemsWindow->saveGeometry());
  }
  FDiscoItemsWindows.removeAt(FDiscoItemsWindows.indexOf(itemsWindow));
  emit discoItemsWindowDestroyed(itemsWindow);
}

void ServiceDiscovery::onQueueTimerTimeout()
{
  bool sended = false;
  QMultiMap<QDateTime,QueuedInfoRequest>::iterator it = FQueuedRequests.begin();
  while (!sended && it!=FQueuedRequests.end() && it.key()<QDateTime::currentDateTime())
  {
    QueuedInfoRequest request = it.value();
    if (!hasDiscoInfo(request.contactJid,request.node))
      sended = requestDiscoInfo(request.streamJid,request.contactJid,request.node);
    it = FQueuedRequests.erase(it);
  }

  if (FQueuedRequests.isEmpty())
    FQueueTimer.stop();
}

void ServiceDiscovery::onSelfCapsChanged()
{
  foreach(Jid streamJid, FSelfCaps.keys())
  {
    EntityCapabilities &myCaps = FSelfCaps[streamJid];
    QString newVer = calcCapsHash(selfDiscoInfo(streamJid),myCaps.hash);
    if (myCaps.ver != newVer)
    {
      myCaps.ver = newVer;
      IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->getPresence(streamJid) : NULL;
      if (presence && presence->isOpen())
        presence->setPresence(presence->show(),presence->status(),presence->priority());
    }
  }
}

Q_EXPORT_PLUGIN2(ServiceDiscoveryPlugin, ServiceDiscovery)
