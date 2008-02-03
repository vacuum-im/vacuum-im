#include "servicediscovery.h"

#include <QDebug>
#include <QFile>
#include <QCryptographicHash>

#define SHC_DISCO_INFO          "/iq[@type='get']/query[@xmlns='" NS_DISCO_INFO "']"
#define SHC_DISCO_ITEMS         "/iq[@type='get']/query[@xmlns='" NS_DISCO_ITEMS "']"
#define SHC_PRESENCE_CAPS       "/presence/c[@xmlns='" NS_CAPS "']"
#define SHC_PRESENCE            "/presence"

#define DISCO_INFO_TIMEOUT      30000
#define DISCO_ITEMS_TIMEOUT     30000

#define ADR_StreamJid           Action::DR_StreamJid
#define ADR_ContactJid          Action::DR_Parametr1
#define ADR_Node                Action::DR_Parametr2

#define SVN_ITEMS_GEOMETRY      "itemsGeometry"

#define IN_CANCEL               "psi/cancel"
#define IN_DISCO                "psi/disco"
#define IN_INFO                 "psi/statusmsg"

#define QUEUE_TIMER_INTERVAL    2000
#define QUEUE_REQUEST_WAIT      10000
#define QUEUE_REQUEST_START     QDateTime::currentDateTime().addMSecs(QUEUE_REQUEST_WAIT)

#define CAPS_DIRNAME            "caps"
#define CAPS_FILE_TAG_NAME      "capabilities"

ServiceDiscovery::ServiceDiscovery()
{
  FPluginManager = NULL;
  FXmppStreams = NULL;
  FPresencePlugin = NULL;
  FStanzaProcessor = NULL;
  FRostersView = NULL;
  FTrayManager = NULL;
  FMainWindowPlugin = NULL;
  FRostersViewPlugin = NULL;
  FRostersModelPlugin = NULL;
  FStatusIcons = NULL;

  FDiscoMenu = NULL;
  FQueueTimer.setSingleShot(false);
  FQueueTimer.setInterval(QUEUE_TIMER_INTERVAL);
  connect(&FQueueTimer,SIGNAL(timeout()),SLOT(onQueueTimerTimeout()));
}

ServiceDiscovery::~ServiceDiscovery()
{
  delete FDiscoMenu;
}

void ServiceDiscovery::pluginInfo(PluginInfo *APluginInfo)
{
  APluginInfo->author = tr("Potapov S.A. aka Lion");
  APluginInfo->description = tr("Discovering information about Jabber entities and the items associated with such entities");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = "Service Discovery";
  APluginInfo->uid = SERVICEDISCOVERY_UUID;
  APluginInfo->version = "0.1";
}

bool ServiceDiscovery::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  FPluginManager = APluginManager;

  IPlugin *plugin = APluginManager->getPlugins("IXmppStreams").value(0,NULL);
  if (plugin) 
  {
    FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
    if (FXmppStreams)
    {
      connect(FXmppStreams->instance(),SIGNAL(added(IXmppStream *)),SLOT(onStreamAdded(IXmppStream *)));
      connect(FXmppStreams->instance(),SIGNAL(removed(IXmppStream *)),SLOT(onStreamRemoved(IXmppStream *)));
      connect(FXmppStreams->instance(),SIGNAL(jidChanged(IXmppStream *, const Jid &)),SLOT(onStreamJidChanged(IXmppStream *, const Jid &)));
    }
  }

  plugin = APluginManager->getPlugins("IPresencePlugin").value(0,NULL);
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

  plugin = APluginManager->getPlugins("IStanzaProcessor").value(0,NULL);
  if (plugin) 
    FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());

  plugin = APluginManager->getPlugins("IRostersViewPlugin").value(0,NULL);
  if (plugin) 
    FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());

  plugin = APluginManager->getPlugins("IRostersModelPlugin").value(0,NULL);
  if (plugin)
    FRostersModelPlugin = qobject_cast<IRostersModelPlugin *>(plugin->instance());

  plugin = APluginManager->getPlugins("IStatusIcons").value(0,NULL);
  if (plugin) 
    FStatusIcons = qobject_cast<IStatusIcons *>(plugin->instance());

  plugin = APluginManager->getPlugins("ITrayManager").value(0,NULL);
  if (plugin)
    FTrayManager = qobject_cast<ITrayManager *>(plugin->instance());

  plugin = APluginManager->getPlugins("IMainWindowPlugin").value(0,NULL);
  if (plugin)
    FMainWindowPlugin = qobject_cast<IMainWindowPlugin *>(plugin->instance());

  plugin = APluginManager->getPlugins("ISettingsPlugin").value(0,NULL);
  if (plugin)
    FSettingsPlugin = qobject_cast<ISettingsPlugin *>(plugin->instance());

  return true;
}

bool ServiceDiscovery::initObjects()
{
  FDiscoMenu = new Menu;
  FDiscoMenu->setIcon(SYSTEM_ICONSETFILE,IN_DISCO);
  FDiscoMenu->setTitle(tr("Service Discovery"));

  registerFeatures();
  insertDiscoHandler(this);

  if (FRostersViewPlugin)
  {
    FRostersView = FRostersViewPlugin->rostersView();
    FRostersView->insertClickHooker(RCHO_SERVICEDISCOVERY,this);
    connect(FRostersView,SIGNAL(contextMenu(IRosterIndex *, Menu *)),SLOT(onRostersViewContextMenu(IRosterIndex *, Menu *)));
    connect(FRostersView,SIGNAL(labelToolTips(IRosterIndex *, int , QMultiMap<int,QString> &)),
      SLOT(onRosterLabelToolTips(IRosterIndex *, int , QMultiMap<int,QString> &)));
  }
  if (FRostersModelPlugin)
  {
    FRostersModelPlugin->rostersModel()->insertDefaultDataHolder(this);
    connect(this,SIGNAL(discoInfoReceived(const IDiscoInfo &)),SLOT(onDiscoInfoChanged(const IDiscoInfo &)));
    connect(this,SIGNAL(discoInfoRemoved(const IDiscoInfo &)),SLOT(onDiscoInfoChanged(const IDiscoInfo &)));
  }
  if (FTrayManager)
  {
    FTrayManager->addAction(FDiscoMenu->menuAction(),AG_SERVICEDISCOVERY_TRAY,true);
  }
  if (FMainWindowPlugin)
  {
    FMainWindowPlugin->mainWindow()->topToolBarChanger()->addAction(FDiscoMenu->menuAction(),AG_SERVICEDISCOVERY_MWTTB,false);
  }

  FDiscoMenu->setEnabled(false);
  return true;
}

bool ServiceDiscovery::editStanza(int AHandlerId, const Jid &/*AStreamJid*/, Stanza * AStanza, bool &/*AAccept*/)
{
  if (FCapsStanzaHandlerOutIds.values().contains(AHandlerId))
  {
    if (FCapsHash.isEmpty())
      FCapsHash = calcCapsHash(selfDiscoInfo()).toBase64();
    QDomElement capsElem = AStanza->addElement("c",NS_CAPS);
    capsElem.setAttribute("node","http://vacuum/#0.0.0");
    capsElem.setAttribute("ver",FCapsHash);
    capsElem.setAttribute("hash","sha-1");
  }
  return false;
}

bool ServiceDiscovery::readStanza(int AHandlerId, const Jid &AStreamJid, const Stanza &AStanza, bool &AAccept)
{
  bool hooked = false;
  if (FInfoStanzaHandlerIds.values().contains(AHandlerId))
  {
    IDiscoInfo dinfo;
    QDomElement query = AStanza.firstElement("query",NS_DISCO_INFO);
    dinfo.contactJid = AStreamJid;
    dinfo.node = query.attribute("node");
    foreach(IDiscoHandler *AHandler, FDiscoHandlers)
      AHandler->fillDiscoInfo(dinfo);
    
    if (dinfo.error.code > 0)
    {
      AAccept = true;
      Stanza reply = AStanza.replyError(dinfo.error.condition,EHN_DEFAULT,dinfo.error.code,dinfo.error.message);
      FStanzaProcessor->sendStanzaOut(AStreamJid,reply);
    }
    else if (!dinfo.identity.isEmpty() || !dinfo.features.isEmpty())
    {
      AAccept = true;
      Stanza reply("iq");
      reply.setTo(AStanza.from()).setId(AStanza.id()).setType("result");
      QDomElement query = reply.addElement("query",NS_DISCO_INFO);
      foreach(IDiscoIdentity identity, dinfo.identity)
      {
        QDomElement elem = query.appendChild(reply.createElement("identity")).toElement();
        elem.setAttribute("category",identity.category);
        elem.setAttribute("type",identity.type);
        if (!identity.name.isEmpty())
          elem.setAttribute("name",identity.name);
      }
      foreach(QString feature, dinfo.features)
      {
        QDomElement elem = query.appendChild(reply.createElement("feature")).toElement();
        elem.setAttribute("var",feature);
      }
      FStanzaProcessor->sendStanzaOut(AStreamJid,reply);
    }
  }
  else if (FItemsStanzaHandlerIds.values().contains(AHandlerId))
  {
    IDiscoItems ditems;
    QDomElement query = AStanza.firstElement("query",NS_DISCO_INFO);
    ditems.contactJid = AStreamJid;
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
  else if (FCapsStanzaHandlerInIds.values().contains(AHandlerId) && AStanza.type().isEmpty())
  {
    EntityCapabilities entityCaps;
    QDomElement capsElem = AStanza.firstElement("c",NS_CAPS);
    entityCaps.contactJid = AStanza.from();
    entityCaps.node = capsElem.attribute("node");
    entityCaps.ver = capsElem.attribute("ver");
    entityCaps.hash = capsElem.attribute("hash");
    entityCaps.ext = capsElem.attribute("ext");
    if (!entityCaps.node.isEmpty() && !entityCaps.ver.isEmpty())
    {
      EntityCapabilities oldCaps = FEntityCaps.value(entityCaps.contactJid);
      if (oldCaps.ver!=entityCaps.ver || oldCaps.node!=entityCaps.node)
      {
        if (hasEntityCaps(entityCaps.node,entityCaps.ver,entityCaps.hash))
        {
          IDiscoInfo dinfo = loadEntityCaps(entityCaps.node,entityCaps.ver,entityCaps.hash);
          dinfo.streamJid = AStreamJid;
          dinfo.contactJid = entityCaps.contactJid;
          FDiscoInfo[dinfo.contactJid].insert(dinfo.node,dinfo);
          emit discoInfoReceived(dinfo);
        }
        else
        {
          QueuedRequest request;
          request.streamJid = AStreamJid;
          request.contactJid = entityCaps.contactJid;
          appendQueuedRequest(QUEUE_REQUEST_START,request);
          removeDiscoInfo(entityCaps.contactJid);
        }
        FEntityCaps.insert(entityCaps.contactJid,entityCaps);
      }
    }
  }
  return hooked;
}

void ServiceDiscovery::iqStanza(const Jid &/*AStreamJid*/, const Stanza &AStanza)
{
  if (FInfoRequestsId.contains(AStanza.id()))
  {
    QPair<Jid,QString> jidnode = FInfoRequestsId.take(AStanza.id());
    IDiscoInfo dinfo = parseDiscoInfo(AStanza,jidnode);
    FDiscoInfo[dinfo.contactJid].insert(dinfo.node,dinfo);
    if (FEntityCaps.contains(dinfo.contactJid))
      saveEntityCaps(dinfo,FEntityCaps.value(dinfo.contactJid));
    emit discoInfoReceived(dinfo);
  }
  else if (FItemsRequestsId.contains(AStanza.id()))
  {
    QPair<Jid,QString> jidnode = FItemsRequestsId.take(AStanza.id());
    IDiscoItems ditems = parseDiscoItems(AStanza,jidnode);
    FDiscoItems[ditems.contactJid].insert(ditems.node,ditems);
    emit discoItemsReceived(ditems);
  }
  else if (FPublishRequestsId.contains(AStanza.id()))
  {
    IDiscoPublish dpublish = FPublishRequestsId.take(AStanza.id());
    if (AStanza.type() == "error")
    {
      ErrorHandler err(AStanza.element());
      dpublish.error.code = err.code();
      dpublish.error.condition = err.condition();
      dpublish.error.message = err.message();
    }
    emit discoItemsPublished(dpublish);
  }
}

void ServiceDiscovery::iqStanzaTimeOut(const QString &AId)
{
  if (FInfoRequestsId.contains(AId))
  {
    IDiscoInfo dinfo;
    QPair<Jid,QString> jidnode = FInfoRequestsId.take(AId);
    ErrorHandler err(ErrorHandler::REMOTE_SERVER_TIMEOUT);
    dinfo.contactJid = jidnode.first;
    dinfo.node = jidnode.second;
    dinfo.error.code = err.code();
    dinfo.error.condition = err.condition();
    dinfo.error.message = err.message();
    FDiscoInfo[dinfo.contactJid].insert(dinfo.node,dinfo);
    emit discoInfoReceived(dinfo);
  }
  else if (FItemsRequestsId.contains(AId))
  {
    IDiscoItems ditems;
    QPair<Jid,QString> jidnode = FItemsRequestsId.take(AId);
    ErrorHandler err(ErrorHandler::REMOTE_SERVER_TIMEOUT);
    ditems.contactJid = jidnode.first;
    ditems.node = jidnode.second;
    ditems.error.code = err.code();
    ditems.error.condition = err.condition();
    ditems.error.message = err.message();
    FDiscoItems[ditems.contactJid].insert(ditems.node,ditems);
    emit discoItemsReceived(ditems);
  }
  else if (FPublishRequestsId.contains(AId))
  {
    IDiscoPublish dpublish = FPublishRequestsId.take(AId);
    ErrorHandler err(ErrorHandler::REMOTE_SERVER_TIMEOUT);
    dpublish.error.code = err.code();
    dpublish.error.condition = err.condition();
    dpublish.error.message = err.message();
    emit discoItemsPublished(dpublish);
  }
}

void ServiceDiscovery::fillDiscoInfo(IDiscoInfo &ADiscoInfo)
{
  if (ADiscoInfo.node.isEmpty())
  {
    IDiscoInfo dinfo = selfDiscoInfo();
    ADiscoInfo.identity += dinfo.identity;
    ADiscoInfo.features += dinfo.features;
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
    << RIT_StreamRoot << RIT_Contact << RIT_Agent << RIT_MyResource;
  return indexTypes;
}

QVariant ServiceDiscovery::data(const IRosterIndex *AIndex, int ARole) const
{
  Jid contactJid = AIndex->type()==RIT_StreamRoot ? Jid(AIndex->data(RDR_Jid).toString()).domane() : AIndex->data(RDR_Jid).toString();
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
  if (AIndex->type() == RIT_Agent)
  {
    IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->getPresence(AIndex->data(RDR_StreamJid).toString()) : NULL;
    if(presence && presence->isOpen())
      showDiscoItems(presence->streamJid(),AIndex->data(RDR_Jid).toString(),"");
  }
  return false;
}

IDiscoInfo ServiceDiscovery::selfDiscoInfo() const
{
  IDiscoInfo dinfo;
  IDiscoIdentity didentity;
  didentity.category = "client";
  didentity.type = "pc";
  didentity.name = "Vacuum";
  dinfo.identity.append(didentity);
  foreach(IDiscoFeature feature, FDiscoFeatures)
    if (feature.active)
      dinfo.features.append(feature.var);
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
  connect(itemsWindow,SIGNAL(destroyed(QObject *)),SLOT(onDiscoItemsWindowDestroyed(QObject *)));
  FDiscoItemsWindows.append(itemsWindow);
  if (FSettingsPlugin)
  {
    ISettings *settings = FSettingsPlugin->settingsForPlugin(SERVICEDISCOVERY_UUID);
    itemsWindow->restoreGeometry(settings->value(SVN_ITEMS_GEOMETRY).toByteArray());
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
    QHash<QString,IDiscoInfo> itemInfos = FDiscoInfo.value(itemJid);
    QList<QString> searchNodes = !AParent.node.isEmpty() ? QList<QString>()<<AParent.node : itemInfos.keys();
    foreach(QString itemNode, searchNodes)
    {
      IDiscoInfo itemInfo = itemInfos.value(itemNode);
      if (compareIdentities(itemInfo.identity,AIdentity)&&compareFeatures(itemInfo.features,AFeatures))
        result.append(itemInfo);
    }
  }
  return result;
}

QIcon ServiceDiscovery::discoInfoIcon(const IDiscoInfo &ADiscoInfo) const
{
  QIcon icon;
  if (ADiscoInfo.error.code >= 0)
  {
    SkinIconset *iconset = Skin::getSkinIconset(SYSTEM_ICONSETFILE);
    icon = iconset->iconByName(IN_CANCEL);
  }
  else if (FStatusIcons)
    icon = FStatusIcons->iconByJidStatus(ADiscoInfo.contactJid,IPresence::Online,"both",false);
  return icon;
}

QIcon ServiceDiscovery::discoItemIcon(const IDiscoItem &ADiscoItem) const
{
  QIcon icon;
  if (FStatusIcons)
    icon = FStatusIcons->iconByJidStatus(ADiscoItem.itemJid,IPresence::Online,"both",false);
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

bool ServiceDiscovery::execFeatureHandler(const Jid &AStreamJid, const QString &AFeature, const IDiscoItem &ADiscoItem)
{
  QList<IDiscoFeatureHandler *> handlers = FFeatureHandlers.value(AFeature).values();
  foreach(IDiscoFeatureHandler *handler, handlers)
    if (handler->execDiscoFeature(AStreamJid,AFeature,ADiscoItem))
      return true;
  return false;
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
  removeDiscoFeature(AFeature.var);
  if (!AFeature.var.isEmpty())
  {
    FCapsHash.clear();
    FDiscoFeatures.insert(AFeature.var,AFeature);
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
    FCapsHash.clear();
    IDiscoFeature dfeature = FDiscoFeatures.take(AFeatureVar);
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
    sended = FStanzaProcessor->sendIqStanza(this,AStreamJid,iq,DISCO_INFO_TIMEOUT);
    if (sended)
      FInfoRequestsId.insert(iq.id(),jidnode);
  }
  return sended;
}

void ServiceDiscovery::removeDiscoInfo(const Jid &AContactJid, const QString &ANode)
{
  if (hasDiscoInfo(AContactJid,ANode))
  {
    QHash<QString,IDiscoInfo> &dnodeInfo = FDiscoInfo[AContactJid];
    IDiscoInfo dinfo = dnodeInfo.take(ANode);
    if (dnodeInfo.isEmpty())
      FDiscoInfo.remove(AContactJid);
    emit discoInfoRemoved(dinfo);
  }
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
    sended = FStanzaProcessor->sendIqStanza(this,AStreamJid,iq,DISCO_ITEMS_TIMEOUT);
    if (sended)
      FItemsRequestsId.insert(iq.id(),jidnode);
  }
  return sended;
}

void ServiceDiscovery::removeDiscoItems(const Jid &AContactJid, const QString &ANode)
{
  if (hasDiscoItems(AContactJid,ANode))
  {
    QHash<QString,IDiscoItems> &dnodeItems = FDiscoItems[AContactJid];
    IDiscoItems ditems = dnodeItems.take(ANode);
    if (dnodeItems.isEmpty())
      FDiscoItems.remove(AContactJid);
    emit discoItemsRemoved(ditems);
  }
}

bool ServiceDiscovery::publishDiscoItems(const IDiscoPublish &ADiscoPublish)
{
  bool sended = false;
  if (FStanzaProcessor && ADiscoPublish.streamJid.isValid())
  {
    Stanza iq("iq");
    iq.setTo(ADiscoPublish.streamJid.eFull()).setId(FStanzaProcessor->newId()).setType("set");
    QDomElement query = iq.addElement("query",NS_DISCO_ITEMS);
    if (!ADiscoPublish.node.isEmpty())
      query.setAttribute("node",ADiscoPublish.node);
    foreach(IDiscoItem discoItem, ADiscoPublish.items)
    {
      QDomElement itemElem = query.appendChild(iq.createElement("item")).toElement();
      itemElem.setAttribute("action",ADiscoPublish.action);
      itemElem.setAttribute("jid",discoItem.itemJid.eFull());
      if (!discoItem.node.isEmpty())
        itemElem.setAttribute("node",discoItem.node);
      if (!discoItem.name.isEmpty())
        itemElem.setAttribute("name",discoItem.name);
    }
    sended = FStanzaProcessor->sendIqStanza(this,ADiscoPublish.streamJid,iq,DISCO_ITEMS_TIMEOUT);
    if (sended)
      FPublishRequestsId.insert(iq.id(),ADiscoPublish);
  }
  return sended;
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
  else if (query.attribute("node")!=result.node)
  {
    ErrorHandler err("item-not-found");
    result.error.code = err.code();
    result.error.condition = err.condition();
    result.error.message = err.message();
  }
  else 
  {
    QDomElement elem = query.firstChildElement("identity");
    while (!elem.isNull())
    {
      IDiscoIdentity identity;
      identity.category = elem.attribute("category");
      identity.type = elem.attribute("type");
      identity.name = elem.attribute("name");
      result.identity.append(identity);
      elem = elem.nextSiblingElement("identity");
    }

    elem = query.firstChildElement("feature");
    while (!elem.isNull())
    {
      result.features.append(elem.attribute("var"));
      elem = elem.nextSiblingElement("feature");
    }
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
  else if (query.attribute("node")!=result.node)
  {
    ErrorHandler err("item-not-found");
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
  SkinIconset *iconset = Skin::getSkinIconset(SYSTEM_ICONSETFILE);
  IDiscoFeature dfeature;

  dfeature.var = NS_DISCO;
  dfeature.active = false; 
  dfeature.icon = iconset->iconByName(IN_DISCO);
  dfeature.name = tr("Service Discovery");
  dfeature.actionName = "";
  dfeature.description = tr("Discover information and items associated with a Jabber Entity");
  insertDiscoFeature(dfeature);

  dfeature.var = NS_DISCO_INFO;
  dfeature.active = true;
  dfeature.icon = iconset->iconByName(IN_INFO);
  dfeature.name = tr("Discovery information");
  dfeature.actionName = "";
  dfeature.description = tr("Discover information about another entity on the network");
  insertDiscoFeature(dfeature);

  dfeature.var = NS_DISCO_ITEMS;
  dfeature.active = false; 
  dfeature.icon = iconset->iconByName(IN_DISCO);
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

  dfeature.var = "jid\\20escaping";
  dfeature.active = true;
  dfeature.icon = QIcon();
  dfeature.name = tr("JID Escaping");
  dfeature.actionName = "";
  dfeature.description = tr("Enables the display of Jabber Identifiers with disallowed characters");
  insertDiscoFeature(dfeature);
}

void ServiceDiscovery::appendQueuedRequest(const QDateTime &ATimeStart, const QueuedRequest &ARequest)
{
  QMultiMap<QDateTime,QueuedRequest>::const_iterator it = FQueuedRequests.constBegin();
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

bool ServiceDiscovery::hasEntityCaps(const QString &ANode, const QString &AVer, const QString &AHash) const
{
  QString fileName = capsFileName(ANode,AVer,AHash);
  return QFile::exists(fileName);
}

QString ServiceDiscovery::capsFileName(const QString &ANode, const QString &AVer, const QString &/*AHash*/) const
{
  QString hashData = ANode+AVer; //AHash.isEmpty() ? ANode+AVer : AVer;
  QString fileName = QCryptographicHash::hash(hashData.toUtf8(),QCryptographicHash::Md5).toHex().toLower()+".xml";
  QDir dir;
  if (FSettingsPlugin)
    dir.setPath(FSettingsPlugin->homeDir().path());
  if (!dir.exists(CAPS_DIRNAME))
    dir.mkdir(CAPS_DIRNAME);
  fileName = dir.path()+"/"CAPS_DIRNAME"/"+fileName;
  return fileName;
}

IDiscoInfo ServiceDiscovery::loadEntityCaps(const QString &ANode, const QString &AVer, const QString &AHash) const
{
  IDiscoInfo dinfo;
  
  QHash<Jid,EntityCapabilities>::const_iterator it = FEntityCaps.constBegin();
  while(it!=FEntityCaps.constEnd())
  {
    if (it.value().node==ANode && it.value().ver==AVer && hasDiscoInfo(it.key(),""))
    {
      dinfo = discoInfo(it.key(),"");
      return dinfo;
    }
    it++;
  }

  QFile capsFile(capsFileName(ANode,AVer,AHash));
  if (capsFile.exists() && capsFile.open(QIODevice::ReadOnly))
  {
    QDomDocument doc;
    doc.setContent(capsFile.readAll());
    capsFile.close();

    QDomElement capsElem = doc.documentElement();
    QDomElement elem = capsElem.firstChildElement("identity");
    while (!elem.isNull())
    {
      IDiscoIdentity identity;
      identity.category = elem.attribute("category");
      identity.type = elem.attribute("type");
      identity.name = elem.attribute("name");
      dinfo.identity.append(identity);
      elem = elem.nextSiblingElement("identity");
    }

    elem = capsElem.firstChildElement("feature");
    while (!elem.isNull())
    {
      dinfo.features.append(elem.attribute("var"));
      elem = elem.nextSiblingElement("feature");
    }
  }
  return dinfo;
}

bool ServiceDiscovery::saveEntityCaps(const IDiscoInfo &AInfo, const EntityCapabilities &ACaps) const
{
  if (AInfo.error.code==-1 && !ACaps.node.isEmpty() && !ACaps.ver.isEmpty())
  {
    QDomDocument doc;
    QDomElement capsEelem = doc.appendChild(doc.createElement(CAPS_FILE_TAG_NAME)).toElement();
    capsEelem.setAttribute("node",ACaps.node);
    capsEelem.setAttribute("ver",ACaps.ver);
    capsEelem.setAttribute("hash",ACaps.hash);
    capsEelem.setAttribute("ext",ACaps.ext);
    foreach(IDiscoIdentity identity, AInfo.identity)
    {
      QDomElement elem = capsEelem.appendChild(doc.createElement("identity")).toElement();
      elem.setAttribute("category",identity.category);
      elem.setAttribute("type",identity.type);
      elem.setAttribute("name",identity.name);
    }
    foreach(QString feature, AInfo.features)
    {
      QDomElement elem = capsEelem.appendChild(doc.createElement("feature")).toElement();
      elem.setAttribute("var",feature);
    }
    QFile capsFile(capsFileName(ACaps.node,ACaps.ver,ACaps.hash));
    if (capsFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
      capsFile.write(doc.toByteArray());
      capsFile.close();
      return true;
    }
  }
  return false;
}

QByteArray ServiceDiscovery::calcCapsHash(const IDiscoInfo &AInfo) const
{
  QStringList hashList;
  QStringList sortList;
  foreach(IDiscoIdentity identity, AInfo.identity)
    sortList.append(identity.category+"/"+identity.type);
  qSort(sortList);
  hashList += sortList;
  sortList = AInfo.features;
  qSort(sortList);
  hashList += sortList;
  hashList.append("");
  return QCryptographicHash::hash(hashList.join("<").toUtf8(),QCryptographicHash::Sha1);
}

bool ServiceDiscovery::compareIdentities(const QList<IDiscoIdentity> &AIdentities, const IDiscoIdentity &AWith) const
{
  foreach(IDiscoIdentity identity,AIdentities)
    if (
        (AWith.category.isEmpty() || AWith.category==identity.category) &&
        (AWith.type.isEmpty() || AWith.type==identity.type) &&
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

void ServiceDiscovery::onStreamAdded(IXmppStream *AXmppStream)
{
  if (FStanzaProcessor && !FInfoStanzaHandlerIds.contains(AXmppStream))
  {
    int handler = FStanzaProcessor->insertHandler(this,SHC_DISCO_INFO,IStanzaProcessor::DirectionIn,SHP_DEFAULT,AXmppStream->jid());
    FInfoStanzaHandlerIds.insert(AXmppStream,handler);

    handler = FStanzaProcessor->insertHandler(this,SHC_DISCO_ITEMS,IStanzaProcessor::DirectionIn,SHP_DEFAULT,AXmppStream->jid());
    FItemsStanzaHandlerIds.insert(AXmppStream,handler);

    handler = FStanzaProcessor->insertHandler(this,SHC_PRESENCE_CAPS,IStanzaProcessor::DirectionIn,SHP_ENTITYCAPS,AXmppStream->jid());
    FCapsStanzaHandlerInIds.insert(AXmppStream,handler);

    handler = FStanzaProcessor->insertHandler(this,SHC_PRESENCE,IStanzaProcessor::DirectionOut,SHP_DEFAULT,AXmppStream->jid());
    FCapsStanzaHandlerOutIds.insert(AXmppStream,handler);
  }
}

void ServiceDiscovery::onStreamStateChanged(const Jid &AStreamJid, bool AStateOnline)
{
  if (AStateOnline)
  {
    Action *action = new Action(FDiscoMenu);
    action->setText(AStreamJid.domane());
    action->setIcon(SYSTEM_ICONSETFILE,IN_DISCO);
    action->setData(ADR_StreamJid,AStreamJid.full());
    action->setData(ADR_ContactJid,AStreamJid.domane());
    action->setData(ADR_Node,QString(""));
    connect(action,SIGNAL(triggered(bool)),SLOT(onShowDiscoItemsByAction(bool)));
    FDiscoMenu->addAction(action,AG_DEFAULT,true);
    FDiscoMenu->setEnabled(true);

    if (!hasDiscoInfo(AStreamJid.domane()))
      requestDiscoInfo(AStreamJid,AStreamJid.domane());
    if (!hasDiscoItems(AStreamJid.domane()))
      requestDiscoItems(AStreamJid,AStreamJid.domane());
  }
  else
  {
    QMultiHash<int,QVariant> data;
    data.insert(ADR_StreamJid,AStreamJid.full());
    Action *action = FDiscoMenu->findActions(data).value(0,NULL);
    if (action)
    {
      FDiscoMenu->removeAction(action);
      FDiscoMenu->setEnabled(!FDiscoMenu->isEmpty());
    }

    QMultiMap<QDateTime,QueuedRequest>::iterator it = FQueuedRequests.begin();
    while (it!=FQueuedRequests.end())
    {
      if (it.value().streamJid == AStreamJid)
        it = FQueuedRequests.erase(it);
      else
        it++;
    }
  }
}

void ServiceDiscovery::onContactStateChanged(const Jid &AStreamJid, const Jid &AContactJid, bool AStateOnline)
{
  if (AStateOnline)
  {
    if (!FEntityCaps.contains(AContactJid))
    {
      QueuedRequest request;
      request.streamJid = AStreamJid;
      request.contactJid = AContactJid;
      appendQueuedRequest(QUEUE_REQUEST_START,request);
    }
  }
  else
  {
    QMultiMap<QDateTime,QueuedRequest>::iterator it = FQueuedRequests.begin();
    while (it!=FQueuedRequests.end())
    {
      if (it.value().contactJid == AContactJid)
        it = FQueuedRequests.erase(it);
      else
        it++;
    }
    FEntityCaps.remove(AContactJid);
    removeDiscoInfo(AContactJid);
  }
}

void ServiceDiscovery::onStreamRemoved(IXmppStream *AXmppStream)
{
  if (FStanzaProcessor && FInfoStanzaHandlerIds.contains(AXmppStream))
  {
    int handler = FInfoStanzaHandlerIds.take(AXmppStream);
    FStanzaProcessor->removeHandler(handler);

    handler = FItemsStanzaHandlerIds.take(AXmppStream);
    FStanzaProcessor->removeHandler(handler);

    handler = FCapsStanzaHandlerInIds.take(AXmppStream);
    FStanzaProcessor->removeHandler(handler);

    handler = FCapsStanzaHandlerOutIds.take(AXmppStream);
    FStanzaProcessor->removeHandler(handler);
  }

  foreach(DiscoInfoWindow *infoWindow, FDiscoInfoWindows)
    if (infoWindow->streamJid() == AXmppStream->jid())
      infoWindow->deleteLater();

  foreach(DiscoItemsWindow *itemsWindow, FDiscoItemsWindows)
    if (itemsWindow->streamJid() == AXmppStream->jid())
      itemsWindow->deleteLater();
}

void ServiceDiscovery::onStreamJidChanged(IXmppStream *AXmppStream, const Jid &ABefour)
{
  QMultiHash<int,QVariant> data;
  data.insert(ADR_StreamJid,AXmppStream->jid().full());
  Action *action = FDiscoMenu->findActions(data).value(0,NULL);
  if (action)
  {
    action->setData(ADR_StreamJid,AXmppStream->jid().full());
    action->setData(ADR_ContactJid,AXmppStream->jid().domane());
  }
  emit streamJidChanged(ABefour,AXmppStream->jid());
}

void ServiceDiscovery::onRostersViewContextMenu(IRosterIndex *AIndex, Menu *AMenu)
{
  int itype = AIndex->type();
  if (itype == RIT_StreamRoot || itype == RIT_Contact || itype == RIT_Agent || itype == RIT_MyResource)
  {
    IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->getPresence(AIndex->data(RDR_StreamJid).toString()) : NULL;
    if (presence && presence->isOpen())
    {
      Action *action = new Action(AMenu);
      action->setText(tr("Discovery Info"));
      action->setIcon(SYSTEM_ICONSETFILE,IN_INFO);
      action->setData(ADR_StreamJid,presence->streamJid().full());
      action->setData(ADR_Node,QString(""));
      if (itype == RIT_StreamRoot)
        action->setData(ADR_ContactJid,Jid(AIndex->data(RDR_Jid).toString()).domane());
      else
        action->setData(ADR_ContactJid,AIndex->data(RDR_Jid));
      connect(action,SIGNAL(triggered(bool)),SLOT(onShowDiscoInfoByAction(bool)));
      AMenu->addAction(action,AG_SERVICE_DISCOVERY_ROSTER,true);

      if (itype == RIT_StreamRoot || itype == RIT_Agent)
      {
        action = new Action(AMenu);
        action->setText(tr("Service Discovery"));
        action->setIcon(SYSTEM_ICONSETFILE,IN_DISCO);
        action->setData(ADR_StreamJid,presence->streamJid().full());
        action->setData(ADR_ContactJid,Jid(AIndex->data(RDR_Jid).toString()).domane());
        action->setData(ADR_Node,QString(""));
        connect(action,SIGNAL(triggered(bool)),SLOT(onShowDiscoItemsByAction(bool)));
        AMenu->addAction(action,AG_SERVICE_DISCOVERY_ROSTER,true);
      }
    }
  }
}

void ServiceDiscovery::onRosterLabelToolTips(IRosterIndex *AIndex, int ALabelId, QMultiMap<int,QString> &AToolTips)
{
  if ((ALabelId == RLID_DISPLAY || ALabelId == RLID_FOOTER_TEXT) && types().contains(AIndex->type()))
  {
    Jid contactJid = AIndex->type()==RIT_StreamRoot ? Jid(AIndex->data(RDR_Jid).toString()).domane() : AIndex->data(RDR_Jid).toString();
    if (hasDiscoInfo(contactJid,""))
    {
      IDiscoInfo dinfo = discoInfo(contactJid,"");
      if (dinfo.identity.value(0).category != "client")
        foreach(IDiscoIdentity identity, dinfo.identity)
          AToolTips.insertMulti(TTO_DISCO_IDENTITY,tr("Categoty: %1; Type: %2").arg(identity.category).arg(identity.type));
    }
  }
}

void ServiceDiscovery::onShowDiscoInfoByAction(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    Jid streamJid = action->data(ADR_StreamJid).toString();
    Jid contactJid = action->data(ADR_ContactJid).toString();
    QString node = action->data(ADR_Node).toString();
    showDiscoInfo(streamJid,contactJid,node);
  }
}

void ServiceDiscovery::onShowDiscoItemsByAction(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    Jid streamJid = action->data(ADR_StreamJid).toString();
    Jid contactJid = action->data(ADR_ContactJid).toString();
    QString node = action->data(ADR_Node).toString();
    showDiscoItems(streamJid,contactJid,node);
  }
}

void ServiceDiscovery::onDiscoInfoChanged(const IDiscoInfo &ADiscoInfo)
{
  QMultiHash<int,QVariant> dataValues;
  dataValues.insertMulti(RDR_Type,RIT_Contact);
  dataValues.insertMulti(RDR_Type,RIT_Agent);
  dataValues.insertMulti(RDR_Type,RIT_MyResource);
  dataValues.insertMulti(RDR_PJid,ADiscoInfo.contactJid.pFull());
  IRosterIndexList indexList = FRostersModelPlugin->rostersModel()->rootIndex()->findChild(dataValues,true);
  foreach(QString streamJid, FRostersModelPlugin->rostersModel()->streams())
    if (Jid(streamJid).pDomane() == ADiscoInfo.contactJid.pDomane())
      indexList.append(FRostersModelPlugin->rostersModel()->getStreamRoot(streamJid));
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

void ServiceDiscovery::onDiscoItemsWindowDestroyed(QObject *AObject)
{
  DiscoItemsWindow *itemsWindow = static_cast<DiscoItemsWindow *>(AObject);
  if (itemsWindow && FSettingsPlugin)
  {
    ISettings *settings = FSettingsPlugin->settingsForPlugin(SERVICEDISCOVERY_UUID);
    settings->setValue(SVN_ITEMS_GEOMETRY,itemsWindow->saveGeometry());
  }
  FDiscoItemsWindows.removeAt(FDiscoItemsWindows.indexOf(itemsWindow));
  emit discoItemsWindowDestroyed(itemsWindow);
}

void ServiceDiscovery::onQueueTimerTimeout()
{
  bool sended = false;
  QMultiMap<QDateTime,QueuedRequest>::iterator it = FQueuedRequests.begin();
  while (!sended && it!=FQueuedRequests.end() && it.key()<QDateTime::currentDateTime())
  {
    QueuedRequest request = it.value();
    if (!hasDiscoInfo(request.contactJid,request.node))
      sended = requestDiscoInfo(request.streamJid,request.contactJid,request.node);
    it = FQueuedRequests.erase(it);
  }

  if (FQueuedRequests.isEmpty())
    FQueueTimer.stop();
}

Q_EXPORT_PLUGIN2(ServiceDiscoveryPlugin, ServiceDiscovery)
