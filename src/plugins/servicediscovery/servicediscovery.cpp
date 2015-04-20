#include "servicediscovery.h"

#include <QFile>
#include <QCryptographicHash>
#include <definitions/version.h>
#include <definitions/namespaces.h>
#include <definitions/rosterindexkinds.h>
#include <definitions/rosterindexroles.h>
#include <definitions/rosterclickhookerorders.h>
#include <definitions/multiuserdataroles.h>
#include <definitions/actiongroups.h>
#include <definitions/toolbargroups.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/serviceicons.h>
#include <definitions/shortcutgrouporders.h>
#include <definitions/stanzahandlerorders.h>
#include <definitions/xmppurihandlerorders.h>
#include <definitions/discofeaturehandlerorders.h>
#include <utils/widgetmanager.h>
#include <utils/iconstorage.h>
#include <utils/logger.h>

#define SHC_DISCO_INFO          "/iq[@type='get']/query[@xmlns='" NS_DISCO_INFO "']"
#define SHC_DISCO_ITEMS         "/iq[@type='get']/query[@xmlns='" NS_DISCO_ITEMS "']"
#define SHC_PRESENCE            "/presence"

#define DISCO_TIMEOUT           60000

#define ADR_STREAMJID           Action::DR_StreamJid
#define ADR_CONTACTJID          Action::DR_Parametr1
#define ADR_NODE                Action::DR_Parametr2

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
	FXmppStreamManager = NULL;
	FRosterManager = NULL;
	FPresenceManager = NULL;
	FStanzaProcessor = NULL;
	FRostersView = NULL;
	FRostersViewPlugin = NULL;
	FMultiChatManager = NULL;
	FTrayManager = NULL;
	FMainWindowPlugin = NULL;
	FStatusIcons = NULL;
	FDataForms = NULL;
	FXmppUriQueries = NULL;

	FUpdateSelfCapsStarted = false;

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
	APluginInfo->description = tr("Allows to receive information about Jabber entities");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(XMPPSTREAMS_UUID);
	APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool ServiceDiscovery::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
	FPluginManager = APluginManager;

	IPlugin *plugin = APluginManager->pluginInterface("IXmppStreamManager").value(0,NULL);
	if (plugin)
	{
		FXmppStreamManager = qobject_cast<IXmppStreamManager *>(plugin->instance());
		if (FXmppStreamManager)
		{
			connect(FXmppStreamManager->instance(),SIGNAL(streamOpened(IXmppStream *)),SLOT(onXmppStreamOpened(IXmppStream *)));
			connect(FXmppStreamManager->instance(),SIGNAL(streamClosed(IXmppStream *)),SLOT(onXmppStreamClosed(IXmppStream *)));
		}
	}

	plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0,NULL);
	if (plugin)
	{
		FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IPresenceManager").value(0,NULL);
	if (plugin)
	{
		FPresenceManager = qobject_cast<IPresenceManager *>(plugin->instance());
		if (FPresenceManager)
		{
			connect(FPresenceManager->instance(),SIGNAL(presenceItemReceived(IPresence *, const IPresenceItem &, const IPresenceItem &)),
				SLOT(onPresenceItemReceived(IPresence *, const IPresenceItem &, const IPresenceItem &)));
		}
	}

	plugin = APluginManager->pluginInterface("IRosterManager").value(0,NULL);
	if (plugin)
	{
		FRosterManager = qobject_cast<IRosterManager *>(plugin->instance());
		if (FRosterManager)
		{
			connect(FRosterManager->instance(),SIGNAL(rosterItemReceived(IRoster *, const IRosterItem &, const IRosterItem &)),
				SLOT(onRosterItemReceived(IRoster *, const IRosterItem &, const IRosterItem &)));
		}
	}

	plugin = APluginManager->pluginInterface("IMultiUserChatManager").value(0,NULL);
	if (plugin)
	{
		FMultiChatManager = qobject_cast<IMultiUserChatManager *>(plugin->instance());
		if (FMultiChatManager)
		{
			connect(FMultiChatManager->instance(),SIGNAL(multiUserChatCreated(IMultiUserChat *)),
				SLOT(onMultiUserChatCreated(IMultiUserChat *)));
			connect(FMultiChatManager->instance(),SIGNAL(multiUserContextMenu(IMultiUserChatWindow *, IMultiUser *, Menu *)),
				SLOT(onMultiUserContextMenu(IMultiUserChatWindow *, IMultiUser *, Menu *)));
		}
	}

	plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
	if (plugin)
	{
		FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
		if (FRostersViewPlugin)
		{
			FRostersView = FRostersViewPlugin->rostersView();
			connect(FRostersView->instance(),SIGNAL(indexContextMenu(const QList<IRosterIndex *> &, quint32, Menu *)), 
				SLOT(onRostersViewIndexContextMenu(const QList<IRosterIndex *> &, quint32, Menu *)));
		}
	}

	plugin = APluginManager->pluginInterface("IStatusIcons").value(0,NULL);
	if (plugin)
	{
		FStatusIcons = qobject_cast<IStatusIcons *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("ITrayManager").value(0,NULL);
	if (plugin)
	{
		FTrayManager = qobject_cast<ITrayManager *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IMainWindowPlugin").value(0,NULL);
	if (plugin)
	{
		FMainWindowPlugin = qobject_cast<IMainWindowPlugin *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IDataForms").value(0,NULL);
	if (plugin)
	{
		FDataForms = qobject_cast<IDataForms *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IXmppUriQueries").value(0,NULL);
	if (plugin)
	{
		FXmppUriQueries = qobject_cast<IXmppUriQueries *>(plugin->instance());
	}

	return FXmppStreamManager!=NULL && FStanzaProcessor!=NULL;
}

bool ServiceDiscovery::initObjects()
{
	FCapsFilesDir.setPath(FPluginManager->homePath());
	if (!FCapsFilesDir.exists(CAPS_DIRNAME))
		FCapsFilesDir.mkdir(CAPS_DIRNAME);
	FCapsFilesDir.cd(CAPS_DIRNAME);

	FDiscoMenu = new Menu;
	FDiscoMenu->setIcon(RSR_STORAGE_MENUICONS,MNI_SDISCOVERY_DISCOVER);
	FDiscoMenu->setTitle(tr("Service Discovery"));
	FDiscoMenu->setEnabled(false);

	registerFeatures();
	insertDiscoHandler(this);

	if (FRostersView)
	{
		FRostersView->insertClickHooker(RCHO_SERVICEDISCOVERY,this);
	}

	if (FTrayManager)
	{
		FTrayManager->contextMenu()->addAction(FDiscoMenu->menuAction(),AG_TMTM_DISCOVERY,true);
	}

	if (FMainWindowPlugin)
	{
		ToolBarChanger *changer = FMainWindowPlugin->mainWindow()->topToolBarChanger();
		QToolButton *button = changer->insertAction(FDiscoMenu->menuAction(),TBG_MWTTB_DISCOVERY);
		button->setPopupMode(QToolButton::InstantPopup);
	}

	if (FXmppUriQueries)
	{
		FXmppUriQueries->insertUriHandler(this, XUHO_DEFAULT);
	}

	insertFeatureHandler(NS_DISCO_INFO,this,DFO_DEFAULT);

	return true;
}

bool ServiceDiscovery::stanzaReadWrite(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept)
{
	if (FSHIPresenceOut.value(AStreamJid) == AHandlerId)
	{
		const EntityCapabilities selfCaps = FSelfCaps.value(AStreamJid);
		if (!selfCaps.ver.isEmpty())
		{
			QDomElement capsElem = AStanza.addElement("c",NS_CAPS);
			capsElem.setAttribute("ver",selfCaps.ver);
			capsElem.setAttribute("node",selfCaps.node);
			capsElem.setAttribute("hash",selfCaps.hash);
		}
		else
		{
			LOG_STRM_WARNING(AStreamJid,"Failed to insert self entity caps: Version is empty");
		}
	}
	else if (FSHIInfo.value(AStreamJid) == AHandlerId)
	{
		QDomElement query = AStanza.firstElement("query",NS_DISCO_INFO);
		IDiscoInfo dinfo = selfDiscoInfo(AStreamJid,query.attribute("node"));

		if (!dinfo.error.isNull())
		{
			AAccept = true;
			Stanza error = FStanzaProcessor->makeReplyError(AStanza,dinfo.error);
			FStanzaProcessor->sendStanzaOut(AStreamJid,error);
			LOG_STRM_WARNING(AStreamJid,QString("Failed to send self discovery info to=%1, node=%2: %3").arg(AStanza.from(),dinfo.node,dinfo.error.condition()));
		}
		else if (!dinfo.identity.isEmpty() || !dinfo.features.isEmpty() || !dinfo.extensions.isEmpty())
		{
			AAccept = true;
			Stanza result = FStanzaProcessor->makeReplyResult(AStanza);
			QDomElement query = result.addElement("query",NS_DISCO_INFO);
			if (!dinfo.node.isEmpty())
				query.setAttribute("node",dinfo.node);
			discoInfoToElem(dinfo,query);
			FStanzaProcessor->sendStanzaOut(AStreamJid,result);
			LOG_STRM_DEBUG(AStreamJid,QString("Self discovery info sent to=%1, node=%2").arg(AStanza.from(),dinfo.node));
		}
		else if (dinfo.node.isEmpty())
		{
			REPORT_ERROR("Failed to send self discovery info: Invalid params");
		}
	}
	else if (FSHIItems.value(AStreamJid) == AHandlerId)
	{
		QDomElement query = AStanza.firstElement("query",NS_DISCO_ITEMS);

		IDiscoItems ditems;
		ditems.streamJid = AStreamJid;
		ditems.contactJid = AStanza.from();
		ditems.node = query.attribute("node");

		foreach(IDiscoHandler *AHandler, FDiscoHandlers)
			AHandler->fillDiscoItems(ditems);

		if (!ditems.error.isNull())
		{
			AAccept = true;
			Stanza error = FStanzaProcessor->makeReplyError(AStanza,ditems.error);
			FStanzaProcessor->sendStanzaOut(AStreamJid,error);
			LOG_STRM_WARNING(AStreamJid,QString("Failed to send self discovery items to=%1, node=%2: %3").arg(AStanza.from(),ditems.node,ditems.error.condition()));
		}
		else
		{
			AAccept = true;
			Stanza result = FStanzaProcessor->makeReplyResult(AStanza);
			QDomElement query = result.addElement("query",NS_DISCO_ITEMS);
			if (!ditems.node.isEmpty())
				query.setAttribute("node",ditems.node);
			foreach(const IDiscoItem &ditem, ditems.items)
			{
				QDomElement elem = query.appendChild(result.createElement("item")).toElement();
				elem.setAttribute("jid",ditem.itemJid.full());
				if (!ditem.node.isEmpty())
					elem.setAttribute("node",ditem.node);
				if (!ditem.name.isEmpty())
					elem.setAttribute("name",ditem.name);
			}
			FStanzaProcessor->sendStanzaOut(AStreamJid,result);
			LOG_STRM_DEBUG(AStreamJid,QString("Self discovery items sent to=%1, node=%2: %3").arg(AStanza.from(),ditems.node,ditems.error.condition()));
		}
	}
	else if (FSHIPresenceIn.value(AStreamJid) == AHandlerId)
	{
		if (AStanza.type().isEmpty())
		{
			Jid contactJid = AStanza.from();
			QDomElement capsElem = AStanza.firstElement("c",NS_CAPS);
			bool isMucPresence = !AStanza.firstElement("x",NS_MUC_USER).isNull();

			EntityCapabilities newCaps;
			newCaps.streamJid = AStreamJid;
			newCaps.entityJid = contactJid;
			newCaps.owner = isMucPresence ? contactJid.pFull() : contactJid.pBare();
			newCaps.node = capsElem.attribute("node");
			newCaps.ver = capsElem.attribute("ver");
			newCaps.hash = capsElem.attribute("hash");

			EntityCapabilities oldCaps = FEntityCaps.value(AStreamJid).value(contactJid);
			bool capsChanged = !capsElem.isNull() && (oldCaps.ver!=newCaps.ver || oldCaps.node!=newCaps.node);

			// Some gates can send back presence from your or another connection with EntityCaps!!!
			// So we should ignore entity caps from all agents
			if (!capsElem.isNull() && contactJid.node().isEmpty())
			{
				newCaps.node.clear();
				newCaps.ver.clear();
				newCaps.hash.clear();
				capsChanged = true;
			}

			if (capsElem.isNull() || capsChanged)
			{
				if (hasEntityCaps(newCaps))
				{
					IDiscoInfo dinfo = loadCapsInfo(newCaps);
					dinfo.streamJid = AStreamJid;
					dinfo.contactJid = contactJid;
					FDiscoInfo[dinfo.streamJid][dinfo.contactJid].insert(dinfo.node,dinfo);
					LOG_STRM_DEBUG(AStreamJid,QString("Discovery info received from caps, from=%1, node=%2").arg(dinfo.contactJid.full(),dinfo.node));
					emit discoInfoReceived(dinfo);
				}
				else if (capsChanged || !hasDiscoInfo(AStreamJid,contactJid))
				{
					DiscoveryRequest request;
					request.streamJid = AStreamJid;
					request.contactJid = contactJid;
					appendQueuedRequest(QUEUE_REQUEST_START,request);
				}
				if (!capsElem.isNull() && !newCaps.node.isEmpty() && !newCaps.ver.isEmpty())
				{
					FEntityCaps[AStreamJid].insert(contactJid,newCaps);
				}
			}
		}
	}
	return false;
}

void ServiceDiscovery::stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza)
{
	Q_UNUSED(AStreamJid);
	if (FInfoRequestsId.contains(AStanza.id()))
	{
		DiscoveryRequest drequest = FInfoRequestsId.take(AStanza.id());
		IDiscoInfo dinfo = parseDiscoInfo(AStanza,drequest);
		FDiscoInfo[dinfo.streamJid][dinfo.contactJid].insert(dinfo.node,dinfo);
		saveCapsInfo(dinfo);
		LOG_STRM_DEBUG(AStreamJid,QString("Discovery info received, from=%1, node=%2, id=%3").arg(dinfo.contactJid.full(),dinfo.node,AStanza.id()));
		emit discoInfoReceived(dinfo);
	}
	else if (FItemsRequestsId.contains(AStanza.id()))
	{
		DiscoveryRequest drequest = FItemsRequestsId.take(AStanza.id());
		IDiscoItems ditems = parseDiscoItems(AStanza,drequest);
		LOG_STRM_DEBUG(AStreamJid,QString("Discovery items received, from=%1, node=%2, id=%3").arg(ditems.contactJid.full(),ditems.node,AStanza.id()));
		emit discoItemsReceived(ditems);
	}
}

bool ServiceDiscovery::xmppUriOpen(const Jid &AStreamJid, const Jid &AContactJid, const QString &AAction, const QMultiMap<QString, QString> &AParams)
{
	if (AAction == "disco")
	{
		QString node = AParams.value("node");
		QString request = AParams.value("request");
		QString type = AParams.value("type");
		if (request=="info" && type=="get")
			showDiscoInfo(AStreamJid,AContactJid,node);
		else if (request=="items" && type=="get")
			showDiscoItems(AStreamJid,AContactJid,node);
		else
			LOG_STRM_WARNING(AStreamJid,QString("Failed to process XMPP URI, request=%1, type=%2: Invalid params").arg(request,type));
		return true;
	}
	return false;
}

bool ServiceDiscovery::rosterIndexSingleClicked(int AOrder, IRosterIndex *AIndex, const QMouseEvent *AEvent)
{
	Q_UNUSED(AOrder); Q_UNUSED(AIndex); Q_UNUSED(AEvent);
	return false;
}

bool ServiceDiscovery::rosterIndexDoubleClicked(int AOrder, IRosterIndex *AIndex, const QMouseEvent *AEvent)
{
	Q_UNUSED(AOrder); Q_UNUSED(AEvent);
	Jid streamJid = AIndex->data(RDR_STREAM_JID).toString();
	if (isReady(streamJid) && AIndex->kind()==RIK_AGENT)
	{
		showDiscoItems(streamJid,AIndex->data(RDR_FULL_JID).toString(),QString::null);
		return true;
	}
	return false;
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

		foreach(const IDiscoFeature &feature, FDiscoFeatures)
			if (feature.active)
				ADiscoInfo.features.append(feature.var);
	}
}

void ServiceDiscovery::fillDiscoItems(IDiscoItems &ADiscoItems)
{
	Q_UNUSED(ADiscoItems);
}

bool ServiceDiscovery::execDiscoFeature(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo)
{
	if (AFeature == NS_DISCO_INFO)
	{
		showDiscoInfo(AStreamJid,ADiscoInfo.contactJid,ADiscoInfo.node);
		return true;
	}
	return false;
}

Action *ServiceDiscovery::createDiscoFeatureAction(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo, QWidget *AParent)
{
	if (AFeature == NS_DISCO_INFO)
	{
		if (isReady(AStreamJid))
			return createDiscoInfoAction(AStreamJid,ADiscoInfo.contactJid,ADiscoInfo.node,AParent);
	}
	return NULL;
}

bool ServiceDiscovery::isReady(const Jid &AStreamJid) const
{
	return FSelfCaps.contains(AStreamJid);
}

IDiscoInfo ServiceDiscovery::selfDiscoInfo(const Jid &AStreamJid, const QString &ANode) const
{
	IDiscoInfo dinfo;
	dinfo.streamJid = AStreamJid;
	dinfo.contactJid = AStreamJid;

	const EntityCapabilities myCaps = FSelfCaps.value(AStreamJid);
	QString capsNode = QString("%1#%2").arg(myCaps.node).arg(myCaps.ver);
	dinfo.node = ANode!=capsNode ? ANode : QString::null;

	foreach(IDiscoHandler *handler, FDiscoHandlers)
		handler->fillDiscoInfo(dinfo);

	dinfo.node = ANode;

	return dinfo;
}

void ServiceDiscovery::showDiscoInfo(const Jid &AStreamJid, const Jid &AContactJid, const QString &ANode, QWidget *AParent)
{
	if (isReady(AStreamJid))
	{
		if (FDiscoInfoWindows.contains(AContactJid))
			FDiscoInfoWindows.take(AContactJid)->close();
		DiscoInfoWindow *infoWindow = new DiscoInfoWindow(this,AStreamJid,AContactJid,ANode,AParent);
		connect(infoWindow,SIGNAL(destroyed(QObject *)),SLOT(onDiscoInfoWindowDestroyed(QObject *)));
		FDiscoInfoWindows.insert(AContactJid,infoWindow);
		infoWindow->show();
	}
}

void ServiceDiscovery::showDiscoItems(const Jid &AStreamJid, const Jid &AContactJid, const QString &ANode, QWidget *AParent)
{
	if (isReady(AStreamJid))
	{
		DiscoItemsWindow *itemsWindow = new DiscoItemsWindow(this,AStreamJid,AParent);
		WidgetManager::setWindowSticky(itemsWindow,true);
		connect(itemsWindow,SIGNAL(windowDestroyed(IDiscoItemsWindow *)),SLOT(onDiscoItemsWindowDestroyed(IDiscoItemsWindow *)));
		FDiscoItemsWindows.append(itemsWindow);
		emit discoItemsWindowCreated(itemsWindow);
		itemsWindow->discover(AContactJid,ANode);
		itemsWindow->show();
	}
}

bool ServiceDiscovery::checkDiscoFeature(const Jid &AStreamJid, const Jid &AContactJid, const QString &ANode, const QString &AFeature, bool ADefault)
{
	IDiscoInfo dinfo = discoInfo(AStreamJid,AContactJid,ANode);
	return !dinfo.error.isNull() || !dinfo.contactJid.isValid() ? ADefault : dinfo.features.contains(AFeature);
}

QList<IDiscoInfo> ServiceDiscovery::findDiscoInfo(const Jid &AStreamJid, const IDiscoIdentity &AIdentity, const QStringList &AFeatures, const IDiscoItem &AParent) const
{
	QList<IDiscoInfo> result;
	QList<Jid> searchJids = AParent.itemJid.isValid() ? QList<Jid>()<<AParent.itemJid : FDiscoInfo.value(AStreamJid).keys();
	foreach(const Jid &itemJid, searchJids)
	{
		QMap<QString, IDiscoInfo> itemInfos = FDiscoInfo.value(AStreamJid).value(itemJid);
		QList<QString> searchNodes = !AParent.node.isEmpty() ? QList<QString>()<<AParent.node : itemInfos.keys();
		foreach(const QString &itemNode, searchNodes)
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

QIcon ServiceDiscovery::serviceIcon(const Jid &AStreamJid, const Jid &AItemJid, const QString &ANode) const
{
	QIcon icon;
	IDiscoInfo dinfo = discoInfo(AStreamJid,AItemJid,ANode);
	IconStorage *storage = IconStorage::staticStorage(RSR_STORAGE_SERVICEICONS);
	DiscoveryRequest drequest;
	drequest.streamJid = AStreamJid;
	drequest.contactJid = AItemJid;
	drequest.node = ANode;
	if (FInfoRequestsId.values().contains(drequest))
		icon = storage->getIcon(SRI_SERVICE_WAIT);
	else if (dinfo.identity.isEmpty())
		icon = storage->getIcon(dinfo.error.isNull() ? SRI_SERVICE_EMPTY : SRI_SERVICE_ERROR);
	else
		icon = identityIcon(dinfo.identity);
	return icon;
}

void ServiceDiscovery::updateSelfEntityCapabilities()
{
	if (!FUpdateSelfCapsStarted)
	{
		FUpdateSelfCapsStarted = true;
		QTimer::singleShot(0,this,SLOT(onSelfCapsChanged()));
	}
}

void ServiceDiscovery::insertDiscoHandler(IDiscoHandler *AHandler)
{
	if (!FDiscoHandlers.contains(AHandler))
	{
		LOG_DEBUG(QString("Discovery handler inserted, address=%1").arg((quint64)AHandler));
		FDiscoHandlers.append(AHandler);
		emit discoHandlerInserted(AHandler);
	}
}

void ServiceDiscovery::removeDiscoHandler(IDiscoHandler *AHandler)
{
	if (FDiscoHandlers.contains(AHandler))
	{
		LOG_DEBUG(QString("Discovery handler removed, address=%1").arg((quint64)AHandler));
		FDiscoHandlers.removeAll(AHandler);
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
		LOG_DEBUG(QString("Feature handler inserted, order=%1, feature=%2, address=%3").arg(AOrder).arg(AFeature).arg((quint64)AHandler));
		FFeatureHandlers[AFeature].insertMulti(AOrder,AHandler);
		emit featureHandlerInserted(AFeature,AHandler);
	}
}

void ServiceDiscovery::removeFeatureHandler(const QString &AFeature, IDiscoFeatureHandler *AHandler)
{
	if (FFeatureHandlers.value(AFeature).values().contains(AHandler))
	{
		LOG_DEBUG(QString("Feature handler removed, feature=%1, address=%2").arg(AFeature).arg((quint64)AHandler));
		FFeatureHandlers[AFeature].remove(FFeatureHandlers[AFeature].key(AHandler),AHandler);
		if (FFeatureHandlers.value(AFeature).isEmpty())
			FFeatureHandlers.remove(AFeature);
		emit featureHandlerRemoved(AFeature,AHandler);
	}
}

bool ServiceDiscovery::execFeatureAction(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo)
{
	QList<IDiscoFeatureHandler *> handlers = FFeatureHandlers.value(AFeature).values();
	foreach(IDiscoFeatureHandler *handler, handlers)
		if (handler->execDiscoFeature(AStreamJid,AFeature,ADiscoInfo))
			return true;
	return false;
}

Action * ServiceDiscovery::createFeatureAction(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo, QWidget *AParent)
{
	foreach(IDiscoFeatureHandler *handler, FFeatureHandlers.value(AFeature).values())
		if (Action *action = handler->createDiscoFeatureAction(AStreamJid,AFeature,ADiscoInfo,AParent))
			return action;
	return NULL;
}

void ServiceDiscovery::insertDiscoFeature(const IDiscoFeature &AFeature)
{
	if (!AFeature.var.isEmpty())
	{
		removeDiscoFeature(AFeature.var);

		LOG_DEBUG(QString("Discovery feature inserted, var=%1, active=%2").arg(AFeature.var).arg(AFeature.active));
		FDiscoFeatures.insert(AFeature.var,AFeature);
		emit discoFeatureInserted(AFeature);
		updateSelfEntityCapabilities();
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
		LOG_DEBUG(QString("Discovery feature removed, var=%1").arg(AFeatureVar));
		IDiscoFeature dfeature = FDiscoFeatures.take(AFeatureVar);
		emit discoFeatureRemoved(dfeature);
		updateSelfEntityCapabilities();
	}
}

bool ServiceDiscovery::hasDiscoInfo(const Jid &AStreamJid, const Jid &AContactJid, const QString &ANode) const
{
	return FDiscoInfo.value(AStreamJid).value(AContactJid).contains(ANode);
}

IDiscoInfo ServiceDiscovery::discoInfo(const Jid &AStreamJid, const Jid &AContactJid, const QString &ANode) const
{
	return FDiscoInfo.value(AStreamJid).value(AContactJid).value(ANode);
}

bool ServiceDiscovery::requestDiscoInfo(const Jid &AStreamJid, const Jid &AContactJid, const QString &ANode)
{
	if (FStanzaProcessor && isReady(AStreamJid) && AStreamJid.isValid() && AContactJid.isValid())
	{
		DiscoveryRequest drequest;
		drequest.streamJid = AStreamJid;
		drequest.contactJid = AContactJid;
		drequest.node = ANode;

		if (!FInfoRequestsId.values().contains(drequest))
		{
			Stanza stanza("iq");
			stanza.setTo(AContactJid.full()).setId(FStanzaProcessor->newId()).setType("get");
			QDomElement query =  stanza.addElement("query",NS_DISCO_INFO);
			if (!ANode.isEmpty())
				query.setAttribute("node",ANode);
			if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,stanza,DISCO_TIMEOUT))
			{
				LOG_STRM_DEBUG(AStreamJid,QString("Discovery info request sent, to=%1, node=%2, id=%3").arg(AContactJid.full(),ANode,stanza.id()));
				FInfoRequestsId.insert(stanza.id(),drequest);
				return true;
			}
			else
			{
				LOG_STRM_WARNING(AStreamJid,QString("Failed to send discovery info request, to=%1, node=%2").arg(AContactJid.full(),ANode));
			}
		}
		else
		{
			return true;
		}
	}
	else if (!isReady(AStreamJid))
	{
		LOG_STRM_WARNING(AStreamJid,QString("Failed to request discovery info, from=%1, node=%2: Stream is not ready").arg(AContactJid.full(),ANode));
	}
	else if (FStanzaProcessor)
	{
		REPORT_ERROR("Failed to request discovery info: Invalid params");
	}
	return false;
}

void ServiceDiscovery::removeDiscoInfo(const Jid &AStreamJid, const Jid &AContactJid, const QString &ANode)
{
	if (hasDiscoInfo(AStreamJid,AContactJid,ANode))
	{
		QMap<QString, IDiscoInfo> &dnodeInfo = FDiscoInfo[AStreamJid][AContactJid];
		IDiscoInfo dinfo = dnodeInfo.take(ANode);
		if (dnodeInfo.isEmpty())
			FDiscoInfo[AStreamJid].remove(AContactJid);
		LOG_STRM_DEBUG(AStreamJid,QString("Discovery info removed, from=%1, node=%2").arg(AContactJid.full(),ANode));
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

bool ServiceDiscovery::requestDiscoItems(const Jid &AStreamJid, const Jid &AContactJid, const QString &ANode)
{
	if (FStanzaProcessor && isReady(AStreamJid) && AStreamJid.isValid() && AContactJid.isValid())
	{
		DiscoveryRequest drequest;
		drequest.streamJid = AStreamJid;
		drequest.contactJid = AContactJid;
		drequest.node = ANode;

		if (!FItemsRequestsId.values().contains(drequest))
		{
			Stanza stanza("iq");
			stanza.setTo(AContactJid.full()).setId(FStanzaProcessor->newId()).setType("get");
			QDomElement query =  stanza.addElement("query",NS_DISCO_ITEMS);
			if (!ANode.isEmpty())
				query.setAttribute("node",ANode);
			if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,stanza,DISCO_TIMEOUT))
			{
				LOG_STRM_DEBUG(AStreamJid,QString("Discovery items request sent, to=%1, node=%2, id=%3").arg(AContactJid.full(),ANode,stanza.id()));
				FItemsRequestsId.insert(stanza.id(),drequest);
				return true;
			}
			else
			{
				LOG_STRM_WARNING(AStreamJid,QString("Failed to send discovery items request, to=%1, node=%2").arg(AContactJid.full(),ANode));
			}
		}
		else
		{
			return true;
		}
	}
	else if (!isReady(AStreamJid))
	{
		LOG_STRM_WARNING(AStreamJid,QString("Failed to request discovery items, from=%1, node=%2: Stream is not ready").arg(AContactJid.full(),ANode));
	}
	else if (FStanzaProcessor)
	{
		REPORT_ERROR("Failed to request discovery items: Invalid params");
	}
	return false;
}

void ServiceDiscovery::discoInfoToElem(const IDiscoInfo &AInfo, QDomElement &AElem) const
{
	QDomDocument doc = AElem.ownerDocument();
	foreach(const IDiscoIdentity &identity, AInfo.identity)
	{
		QDomElement elem = AElem.appendChild(doc.createElement("identity")).toElement();
		elem.setAttribute("category",identity.category);
		elem.setAttribute("type",identity.type);
		if (!identity.name.isEmpty())
			elem.setAttribute("name",identity.name);
		if (!identity.lang.isEmpty())
			elem.setAttribute("xml:lang",identity.lang);

	}
	foreach(const QString &feature, AInfo.features)
	{
		QDomElement elem = AElem.appendChild(doc.createElement("feature")).toElement();
		elem.setAttribute("var",feature);
	}
	if (FDataForms)
	{
		foreach(const IDataForm &form, AInfo.extensions)
			FDataForms->xmlForm(form,AElem);
	}
}

void ServiceDiscovery::discoInfoFromElem(const QDomElement &AElem, IDiscoInfo &AInfo) const
{
	AInfo.identity.clear();
	QDomElement elem = AElem.firstChildElement("identity");
	while (!elem.isNull())
	{
		IDiscoIdentity identity;
		identity.category = elem.attribute("category").toLower();
		identity.type = elem.attribute("type").toLower();
		identity.lang = elem.attribute("lang");
		identity.name = elem.attribute("name");
		AInfo.identity.append(identity);
		elem = elem.nextSiblingElement("identity");
	}

	AInfo.features.clear();
	elem = AElem.firstChildElement("feature");
	while (!elem.isNull())
	{
		QString feature = elem.attribute("var").toLower();
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

IDiscoInfo ServiceDiscovery::parseDiscoInfo(const Stanza &AStanza, const DiscoveryRequest &ADiscoRequest) const
{
	IDiscoInfo result;
	result.streamJid = ADiscoRequest.streamJid;
	result.contactJid = ADiscoRequest.contactJid;
	result.node = ADiscoRequest.node;

	QDomElement query = AStanza.firstElement("query",NS_DISCO_INFO);
	if (AStanza.type() == "error")
		result.error = XmppStanzaError(AStanza);
	else if (result.contactJid!=AStanza.from() || result.node!=query.attribute("node"))
		result.error = XmppStanzaError(XmppStanzaError::EC_FEATURE_NOT_IMPLEMENTED);
	else
		discoInfoFromElem(query,result);
	return result;
}

IDiscoItems ServiceDiscovery::parseDiscoItems(const Stanza &AStanza, const DiscoveryRequest &ADiscoRequest) const
{
	IDiscoItems result;
	result.streamJid = ADiscoRequest.streamJid;
	result.contactJid = ADiscoRequest.contactJid;
	result.node = ADiscoRequest.node;

	QDomElement query = AStanza.firstElement("query",NS_DISCO_ITEMS);
	if (AStanza.type() == "error")
	{
		result.error = XmppStanzaError(AStanza);
	}
	else if (result.contactJid!=AStanza.from() || result.node!=query.attribute("node"))
	{
		result.error = XmppStanzaError(XmppStanzaError::EC_FEATURE_NOT_IMPLEMENTED);
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
	dfeature.description = tr("Supports the exchange of the discovery information and items");
	insertDiscoFeature(dfeature);

	dfeature.var = NS_DISCO_INFO;
	dfeature.active = true;
	dfeature.icon = storage->getIcon(MNI_SDISCOVERY_DISCOINFO);
	dfeature.name = tr("Discovery Information");
	dfeature.description = tr("Supports the exchange of the discovery information");
	insertDiscoFeature(dfeature);

	dfeature.var = NS_DISCO_ITEMS;
	dfeature.active = false;
	dfeature.icon = storage->getIcon(MNI_SDISCOVERY_DISCOINFO);
	dfeature.name = tr("Discovery Items");
	dfeature.description = tr("Supports the exchange of the discovery items");
	insertDiscoFeature(dfeature);

	dfeature.var = NS_DISCO_PUBLISH;
	dfeature.active = false;
	dfeature.icon = QIcon();
	dfeature.name = tr("Publish Items");
	dfeature.description = tr("Supports the publishing of the discovery items");
	insertDiscoFeature(dfeature);

	dfeature.var = NS_CAPS;
	dfeature.active = true;
	dfeature.icon = QIcon();
	dfeature.name = tr("Entity Capabilities");
	dfeature.description = tr("Supports the caching of the discovery information");
	insertDiscoFeature(dfeature);

	dfeature.var = "jid\\20escaping";
	dfeature.active = true;
	dfeature.icon = QIcon();
	dfeature.name = tr("JID Escaping");
	dfeature.description = tr("Supports the displaying of the jabber identifiers with disallowed characters");
	insertDiscoFeature(dfeature);
}

void ServiceDiscovery::appendQueuedRequest(const QDateTime &ATimeStart, const DiscoveryRequest &ARequest)
{
	QMultiMap<QDateTime,DiscoveryRequest>::const_iterator it = FQueuedRequests.constBegin();
	while (it != FQueuedRequests.constEnd())
	{
		if (it.value().contactJid==ARequest.contactJid && it.value().node==ARequest.node)
			return;
		++it;
	}

	if (!FQueueTimer.isActive())
		FQueueTimer.start();
	FQueuedRequests.insert(ATimeStart,ARequest);
}

void ServiceDiscovery::removeQueuedRequest(const DiscoveryRequest &ARequest)
{
	QMultiMap<QDateTime,DiscoveryRequest>::iterator it = FQueuedRequests.begin();
	while (it != FQueuedRequests.end())
	{
		if ((ARequest.streamJid.isEmpty() || it.value().streamJid == ARequest.streamJid) &&
		    (ARequest.contactJid.isEmpty() || it.value().contactJid == ARequest.contactJid) &&
		    (ARequest.node.isEmpty() || it.value().node == ARequest.node))
		{
			it = FQueuedRequests.erase(it);
		}
		else
		{
			++it;
		}
	}
}

bool ServiceDiscovery::hasEntityCaps(const EntityCapabilities &ACaps) const
{
	return QFile::exists(capsFileName(ACaps,false)) || QFile::exists(capsFileName(ACaps,true));
}

QString ServiceDiscovery::capsFileName(const EntityCapabilities &ACaps, bool AWithOwner) const
{
	QString hashString = ACaps.hash.isEmpty() ? ACaps.node+ACaps.ver : ACaps.ver+ACaps.hash;
	hashString += AWithOwner ? ACaps.owner : QString::null;
	QString fileName = QCryptographicHash::hash(hashString.toUtf8(),QCryptographicHash::Md5).toHex().toLower() + ".xml";
	return FCapsFilesDir.absoluteFilePath(fileName);
}

IDiscoInfo ServiceDiscovery::loadCapsInfo(const EntityCapabilities &ACaps) const
{
	// Check if present another user with same correct calculated caps and loaded discovery info
	QHash<Jid, EntityCapabilities> streamEntityCaps = FEntityCaps.value(ACaps.streamJid);
	for(QHash<Jid, EntityCapabilities>::const_iterator it=streamEntityCaps.constBegin(); it!=streamEntityCaps.constEnd(); ++it)
	{
		if ((!ACaps.hash.isEmpty() || ACaps.node==it->node) && ACaps.ver==it->ver && ACaps.hash==it->hash && hasDiscoInfo(it->streamJid,it->entityJid))
		{
			IDiscoInfo dinfo = discoInfo(it->streamJid,it->entityJid);
			if (!it->ver.isEmpty() && it->ver==calcCapsHash(dinfo,it->hash))
				return dinfo;
		}
	}

	// Load discovery info from file
	IDiscoInfo dinfo;
	foreach(const QString &fileName, QStringList() << capsFileName(ACaps,true) << capsFileName(ACaps,false))
	{
		QFile file(fileName);
		if (file.open(QIODevice::ReadOnly))
		{
			QString xmlError;
			QDomDocument doc;
			if (doc.setContent(&file,true,&xmlError))
			{
				QDomElement capsElem = doc.documentElement();
				discoInfoFromElem(capsElem,dinfo);
				break;
			}
			else
			{
				REPORT_ERROR(QString("Failed to load caps info from file content: %1").arg(xmlError));
				file.remove();
			}
		}
		else if (file.exists())
		{
			REPORT_ERROR(QString("Failed to load caps info from file: %1").arg(file.errorString()));
		}
	}

	return dinfo;
}

bool ServiceDiscovery::saveCapsInfo(const IDiscoInfo &AInfo) const
{
	if (AInfo.error.isNull() && FEntityCaps.value(AInfo.streamJid).contains(AInfo.contactJid))
	{
		EntityCapabilities caps = FEntityCaps.value(AInfo.streamJid).value(AInfo.contactJid);
		QString capsNode = QString("%1#%2").arg(caps.node).arg(caps.ver);
		if (AInfo.node.isEmpty() || AInfo.node==capsNode)
		{
			if (!hasEntityCaps(caps))
			{
				QDomDocument doc;
				QDomElement capsElem = doc.appendChild(doc.createElement(CAPS_FILE_TAG_NAME)).toElement();
				capsElem.setAttribute("node",caps.node);
				capsElem.setAttribute("ver",caps.ver);
				capsElem.setAttribute("hash",caps.hash);
				discoInfoToElem(AInfo,capsElem);

				bool checked = (!caps.ver.isEmpty() && caps.ver==calcCapsHash(AInfo,caps.hash));
				if (!checked)
					capsElem.setAttribute("jid",caps.owner);

				QFile file(capsFileName(caps,!checked));
				if (file.open(QIODevice::WriteOnly|QIODevice::Truncate))
				{
					file.write(doc.toByteArray());
					file.flush();
				}
				else
				{
					REPORT_ERROR(QString("Failed to save caps info to file: %1").arg(file.errorString()));
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

		foreach(const IDiscoIdentity &identity, AInfo.identity)
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
				foreach(const IDataField &field, AInfo.extensions.at(iforms.value()).fields)
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
					++ifields;
				}
				++iforms;
			}
		}
		hashList.append(QString::null);
		QByteArray hashData = hashList.join("<").toUtf8();
		return QCryptographicHash::hash(hashData, AHash==CAPS_HASH_SHA1 ? QCryptographicHash::Sha1 : QCryptographicHash::Md5).toBase64();
	}
	else if (!AHash.isEmpty())
	{
		LOG_STRM_WARNING(AInfo.streamJid,QString("Failed to calculate caps hash, jid=%1: Invalid type=%2").arg(AInfo.contactJid.full(),AHash));
	}
	return QString::null;
}

bool ServiceDiscovery::compareIdentities(const QList<IDiscoIdentity> &AIdentities, const IDiscoIdentity &AWith) const
{
	foreach(const IDiscoIdentity &identity, AIdentities)
	{
		if ((AWith.category.isEmpty() || AWith.category==identity.category) &&
			  (AWith.type.isEmpty() || AWith.type==identity.type) &&
			  (AWith.lang.isEmpty() || AWith.lang==identity.lang) &&
			  (AWith.name.isEmpty() || AWith.name==identity.name))
		{
			return true;
		}
	}
	return false;
}

bool ServiceDiscovery::compareFeatures(const QStringList &AFeatures, const QStringList &AWith) const
{
	if (!AWith.isEmpty())
	{
		foreach(const QString &feature, AWith)
			if (!AFeatures.contains(feature))
				return false;
	}
	return true;
}

void ServiceDiscovery::insertStreamMenu(const Jid &AStreamJid)
{
	Action *action = new Action(FDiscoMenu);
	action->setText(AStreamJid.uFull());
	action->setIcon(RSR_STORAGE_MENUICONS,MNI_SDISCOVERY_DISCOVER);
	action->setData(ADR_STREAMJID,AStreamJid.full());
	action->setData(ADR_CONTACTJID,AStreamJid.domain());
	action->setData(ADR_NODE,QString());
	connect(action,SIGNAL(triggered(bool)),SLOT(onShowDiscoItemsByAction(bool)));
	FDiscoMenu->addAction(action,AG_DEFAULT,true);
	FDiscoMenu->setEnabled(true);
}

void ServiceDiscovery::removeStreamMenu(const Jid &AStreamJid)
{
	QMultiHash<int,QVariant> data;
	data.insert(ADR_STREAMJID,AStreamJid.full());
	Action *action = FDiscoMenu->findActions(data).value(0,NULL);
	if (action)
	{
		FDiscoMenu->removeAction(action);
		FDiscoMenu->setEnabled(!FDiscoMenu->isEmpty());
		action->deleteLater();
	}
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

void ServiceDiscovery::onXmppStreamOpened(IXmppStream *AXmppStream)
{
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

	insertStreamMenu(AXmppStream->streamJid());

	EntityCapabilities &myCaps = FSelfCaps[AXmppStream->streamJid()];
	myCaps.streamJid = AXmppStream->streamJid();
	myCaps.entityJid = AXmppStream->streamJid();
	myCaps.node = CLIENT_HOME_PAGE;
	myCaps.hash = CAPS_HASH_SHA1;
	myCaps.ver = calcCapsHash(selfDiscoInfo(myCaps.streamJid),myCaps.hash);

	Jid server = AXmppStream->streamJid().domain();
	requestDiscoInfo(AXmppStream->streamJid(),server);
	requestDiscoItems(AXmppStream->streamJid(),server);

	IRoster *roster = FRosterManager!=NULL ? FRosterManager->findRoster(AXmppStream->streamJid()) : NULL;
	QList<IRosterItem> ritems = roster!=NULL ? roster->items() : QList<IRosterItem>();
	foreach(const IRosterItem &ritem, ritems)
	{
		if (ritem.itemJid.node().isEmpty())
		{
			DiscoveryRequest request;
			request.streamJid = AXmppStream->streamJid();
			request.contactJid = ritem.itemJid;
			appendQueuedRequest(QUEUE_REQUEST_START,request);
		}
	}

	emit discoOpened(AXmppStream->streamJid());
}

void ServiceDiscovery::onXmppStreamClosed(IXmppStream *AXmppStream)
{
	if (FStanzaProcessor)
	{
		FStanzaProcessor->removeStanzaHandle(FSHIInfo.take(AXmppStream->streamJid()));
		FStanzaProcessor->removeStanzaHandle(FSHIItems.take(AXmppStream->streamJid()));
		FStanzaProcessor->removeStanzaHandle(FSHIPresenceIn.take(AXmppStream->streamJid()));
		FStanzaProcessor->removeStanzaHandle(FSHIPresenceOut.take(AXmppStream->streamJid()));
	}

	DiscoveryRequest request;
	request.streamJid = AXmppStream->streamJid();
	removeQueuedRequest(request);

	foreach(DiscoInfoWindow *infoWindow, FDiscoInfoWindows)
		if (infoWindow->streamJid() == AXmppStream->streamJid())
			infoWindow->deleteLater();

	foreach(DiscoItemsWindow *itemsWindow, FDiscoItemsWindows)
		if (itemsWindow->streamJid() == AXmppStream->streamJid())
			itemsWindow->deleteLater();

	removeStreamMenu(AXmppStream->streamJid());

	foreach(const Jid &contactJid, FDiscoInfo.value(AXmppStream->streamJid()).keys())
		foreach(const QString &node, FDiscoInfo.value(AXmppStream->streamJid()).value(contactJid).keys())
			removeDiscoInfo(AXmppStream->streamJid(),contactJid,node);

	FSelfCaps.remove(AXmppStream->streamJid());
	FEntityCaps.remove(AXmppStream->streamJid());
	FDiscoInfo.remove(AXmppStream->streamJid());

	emit discoClosed(AXmppStream->streamJid());
}

void ServiceDiscovery::onPresenceItemReceived(IPresence *APresence, const IPresenceItem &AItem, const IPresenceItem &ABefore)
{
	Q_UNUSED(ABefore);
	if (AItem.show==IPresence::Offline || AItem.show==IPresence::Error)
	{
		if (!AItem.itemJid.node().isEmpty())
		{
			DiscoveryRequest request;
			request.streamJid = APresence->streamJid();
			request.contactJid = AItem.itemJid;
			removeQueuedRequest(request);
			removeDiscoInfo(APresence->streamJid(),AItem.itemJid);
		}
		FEntityCaps[APresence->streamJid()].remove(AItem.itemJid);
	}
}

void ServiceDiscovery::onRosterItemReceived(IRoster *ARoster, const IRosterItem &AItem, const IRosterItem &ABefore)
{
	Q_UNUSED(ABefore);
	if (AItem.subscription!=SUBSCRIPTION_REMOVE && AItem.itemJid.node().isEmpty() && ARoster->isOpen() && !hasDiscoInfo(ARoster->streamJid(), AItem.itemJid))
	{
		DiscoveryRequest request;
		request.streamJid = ARoster->streamJid();
		request.contactJid = AItem.itemJid;
		appendQueuedRequest(QUEUE_REQUEST_START,request);
	}
}

void ServiceDiscovery::onDiscoInfoReceived(const IDiscoInfo &ADiscoInfo)
{
	DiscoveryRequest request;
	request.streamJid = ADiscoInfo.streamJid;
	request.contactJid = ADiscoInfo.contactJid;
	request.node = ADiscoInfo.node;
	removeQueuedRequest(request);
}

void ServiceDiscovery::onMultiUserPresence(IMultiUser *AUser, int AShow, const QString &AStatus)
{
	Q_UNUSED(AStatus);
	if (AShow==IPresence::Offline || AShow==IPresence::Error)
	{
		bool isSingleUser = true;
		Jid userStreamJid = AUser->data(MUDR_STREAM_JID).toString();
		foreach(IMultiUserChat *mchat, FMultiChatManager->multiUserChats())
		{
			IMultiUser *muser = mchat->userByNick(AUser->nickName());
			if (muser!=NULL && muser!=AUser && mchat->roomJid()==AUser->roomJid() && mchat->streamJid()==userStreamJid)
			{
				isSingleUser = false;
				break;
			}
		}
		if (isSingleUser)
		{
			DiscoveryRequest request;
			request.streamJid = userStreamJid;
			request.contactJid = AUser->contactJid();
			removeQueuedRequest(request);
			removeDiscoInfo(userStreamJid,AUser->contactJid());
			FEntityCaps[userStreamJid].remove(AUser->contactJid());
		}
	}
}

void ServiceDiscovery::onMultiUserChatCreated(IMultiUserChat *AMultiChat)
{
	connect(AMultiChat->instance(), SIGNAL(userPresence(IMultiUser *, int, const QString &)), SLOT(onMultiUserPresence(IMultiUser *, int, const QString &)));
}

void ServiceDiscovery::onMultiUserContextMenu(IMultiUserChatWindow *AWindow, IMultiUser *AUser, Menu *AMenu)
{
	if (isReady(AWindow->streamJid()))
	{
		IDiscoInfo dinfo = discoInfo(AWindow->streamJid(),AUser->contactJid());

		// Many clients support version info but don`t show it in disco info
		if (dinfo.streamJid.isValid() && !dinfo.features.contains(NS_JABBER_VERSION))
			dinfo.features.append(NS_JABBER_VERSION);

		foreach(const QString &feature, dinfo.features)
		{
			Action *action = createFeatureAction(AWindow->streamJid(),feature,dinfo,AMenu);
			if (action)
				AMenu->addAction(action, AG_MUCM_DISCOVERY_FEATURES, true);
		}
	}
}

void ServiceDiscovery::onRostersViewIndexContextMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu)
{
	if (ALabelId==AdvancedDelegateItem::DisplayId && AIndexes.count()==1)
	{
		IRosterIndex *index = AIndexes.first();
		Jid streamJid = index->data(RDR_STREAM_JID).toString();
		if (isReady(streamJid))
		{
			int indexKind = index->kind();
			Jid contactJid = indexKind!=RIK_STREAM_ROOT ? index->data(RDR_FULL_JID).toString() : streamJid.domain();

			IRoster *roster = FRosterManager!=NULL ? FRosterManager->findRoster(streamJid) : NULL;

			if (indexKind==RIK_STREAM_ROOT || indexKind==RIK_AGENT)
			{
				Action *action = createDiscoItemsAction(streamJid,contactJid,QString::null,AMenu);
				AMenu->addAction(action,AG_RVCM_DISCOVERY,true);
			}

			QStringList resources = index->data(RDR_RESOURCES).toStringList();
			if (resources.isEmpty())
				resources.append(contactJid.pFull());
			bool multiResorces = resources.count()>1;

			QMap<QString, Menu *> resMenu;
			foreach(const Jid &itemJid, resources)
			{
				IDiscoInfo dinfo = discoInfo(streamJid,itemJid);

				IRosterItem ritem = roster!=NULL ? roster->findItem(itemJid) : IRosterItem();
				QString resName = (!ritem.name.isEmpty() ? ritem.name : itemJid.uBare()) + (!itemJid.resource().isEmpty() ? QString("/")+itemJid.resource() : QString::null);

				// Many clients support version info but don`t show it in disco info
				if (dinfo.streamJid.isValid() && !dinfo.features.contains(NS_JABBER_VERSION))
					dinfo.features.append(NS_JABBER_VERSION);

				foreach(const QString &feature, dinfo.features)
				{
					Action *action = createFeatureAction(streamJid,feature,dinfo,AMenu);
					if (action)
					{
						if (multiResorces)
						{
							Menu *menu = resMenu.value(action->text());
							if (menu == NULL)
							{
								menu = new Menu(AMenu);
								menu->setIcon(action->icon());
								menu->setTitle(action->text());
								resMenu.insert(action->text(),menu);
								AMenu->addAction(menu->menuAction(),AG_RVCM_DISCOVERY_FEATURES,true);
							}
							action->setText(resName);
							action->setParent(action->parent()==AMenu ? menu : action->parent());
							action->setIcon(FStatusIcons!=NULL ? FStatusIcons->iconByJid(streamJid,itemJid) : QIcon());
							menu->addAction(action,AG_RVCM_DISCOVERY_FEATURES,false); 
						}
						else
						{
							AMenu->addAction(action,AG_RVCM_DISCOVERY_FEATURES,true);
						}
					}
				}
			}
		}
	}
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

void ServiceDiscovery::onDiscoInfoWindowDestroyed(QObject *AObject)
{
	DiscoInfoWindow *infoWindow = static_cast<DiscoInfoWindow *>(AObject);
	FDiscoInfoWindows.remove(FDiscoInfoWindows.key(infoWindow));
}

void ServiceDiscovery::onDiscoItemsWindowDestroyed(IDiscoItemsWindow *AWindow)
{
	DiscoItemsWindow *itemsWindow = static_cast<DiscoItemsWindow *>(AWindow->instance());
	if (itemsWindow)
	{
		FDiscoItemsWindows.removeAll(itemsWindow);
		emit discoItemsWindowDestroyed(itemsWindow);
	}
}

void ServiceDiscovery::onQueueTimerTimeout()
{
	bool sent = false;
	QMultiMap<QDateTime,DiscoveryRequest>::iterator it = FQueuedRequests.begin();
	while (!sent && it!=FQueuedRequests.end() && it.key()<QDateTime::currentDateTime())
	{
		DiscoveryRequest request = it.value();
		sent = requestDiscoInfo(request.streamJid,request.contactJid,request.node);
		it = FQueuedRequests.erase(it);
	}

	if (FQueuedRequests.isEmpty())
		FQueueTimer.stop();
}

void ServiceDiscovery::onSelfCapsChanged()
{
	foreach(const Jid &streamJid, FSelfCaps.keys())
	{
		EntityCapabilities &myCaps = FSelfCaps[streamJid];
		QString newVer = calcCapsHash(selfDiscoInfo(streamJid),myCaps.hash);
		if (myCaps.ver != newVer)
		{
			myCaps.ver = newVer;
			IPresence *presence = FPresenceManager!=NULL ? FPresenceManager->findPresence(streamJid) : NULL;
			if (presence && presence->isOpen())
				presence->setPresence(presence->show(),presence->status(),presence->priority());
		}
	}
	FUpdateSelfCapsStarted = false;
}

Q_EXPORT_PLUGIN2(plg_servicediscovery, ServiceDiscovery)
