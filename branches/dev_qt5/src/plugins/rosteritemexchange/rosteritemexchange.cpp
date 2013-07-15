#include "rosteritemexchange.h"

#include <QDropEvent>
#include <QDataStream>
#include <QMessageBox>
#include <QTextDocument>
#include <QDragMoveEvent>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QMimeData>
#include <utils/QtEscape.h>

#define ADR_STREAM_JID         Action::DR_StreamJid
#define ADR_CONTACT_JID        Action::DR_Parametr1
#define ADR_ITEMS_JIDS         Action::DR_Parametr2
#define ADR_ITEMS_NAMES        Action::DR_Parametr3
#define ADR_ITEMS_GROUPS       Action::DR_Parametr4

#define SHC_ROSTERX_IQ         "/iq/x[@xmlns='" NS_ROSTERX "']"
#define SHC_ROSTERX_MESSAGE    "/message/x[@xmlns='" NS_ROSTERX "']"

RosterItemExchange::RosterItemExchange()
{
	FGateways = NULL;
	FRosterPlugin = NULL;
	FRosterChanger = NULL;
	FPresencePlugin = NULL;
	FDiscovery = NULL;
	FStanzaProcessor = NULL;
	FOptionsManager = NULL;
	FNotifications = NULL;
	FMessageWidgets = NULL;
	FRostersViewPlugin = NULL;

	FSHIExchangeRequest = -1;
}

RosterItemExchange::~RosterItemExchange()
{

}

void RosterItemExchange::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Roster Item Exchange");
	APluginInfo->description = tr("Allows to exchange contact list items");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(ROSTER_UUID);
	APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool RosterItemExchange::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
	IPlugin *plugin = APluginManager->pluginInterface("IRosterPlugin").value(0,NULL);
	if (plugin)
	{
		FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IRosterChanger").value(0,NULL);
	if (plugin)
	{
		FRosterChanger = qobject_cast<IRosterChanger *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IPresencePlugin").value(0,NULL);
	if (plugin)
	{
		FPresencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IServiceDiscovery").value(0,NULL);
	if (plugin)
	{
		FDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0,NULL);
	if (plugin)
	{
		FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IOptionsManager").value(0,NULL);
	if (plugin)
	{
		FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("INotifications").value(0,NULL);
	if (plugin)
	{
		FNotifications = qobject_cast<INotifications *>(plugin->instance());
		if (FNotifications)
		{
			connect(FNotifications->instance(),SIGNAL(notificationActivated(int)), SLOT(onNotificationActivated(int)));
			connect(FNotifications->instance(),SIGNAL(notificationRemoved(int)), SLOT(onNotificationRemoved(int)));
		}
	}

	plugin = APluginManager->pluginInterface("IMessageWidgets").value(0,NULL);
	if (plugin)
	{
		FMessageWidgets = qobject_cast<IMessageWidgets *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
	if (plugin)
	{
		FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IGateways").value(0,NULL);
	if (plugin)
	{
		FGateways = qobject_cast<IGateways *>(plugin->instance());
	}

	return FRosterPlugin!=NULL && FStanzaProcessor!=NULL;
}

bool RosterItemExchange::initObjects()
{
	if (FDiscovery)
	{
		IDiscoFeature feature;
		feature.var = NS_ROSTERX;
		feature.active = true;
		feature.icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_ROSTEREXCHANGE_REQUEST);
		feature.name = tr("Roster Item Exchange");
		feature.description = tr("Supports the exchanging of contact list items");
		FDiscovery->insertDiscoFeature(feature);
	}

	if (FStanzaProcessor)
	{
		IStanzaHandle handle;
		handle.handler = this;
		handle.order = SHO_IMI_ROSTEREXCHANGE;
		handle.direction = IStanzaHandle::DirectionIn;
		handle.conditions.append(SHC_ROSTERX_IQ);
		handle.conditions.append(SHC_ROSTERX_MESSAGE);
		FSHIExchangeRequest = FStanzaProcessor->insertStanzaHandle(handle);
	}

	if (FOptionsManager)
	{
		FOptionsManager->insertOptionsHolder(this);
	}

	if (FNotifications)
	{
		INotificationType notifyType;
		notifyType.order = NTO_ROSTEREXCHANGE_REQUEST;
		notifyType.icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_ROSTEREXCHANGE_REQUEST);
		notifyType.title = tr("When receiving roster modification request");
		notifyType.kindMask = INotification::RosterNotify|INotification::TrayNotify|INotification::TrayAction|INotification::PopupWindow|INotification::SoundPlay|INotification::AlertWidget|INotification::ShowMinimized|INotification::AutoActivate;
		notifyType.kindDefs = notifyType.kindMask & ~(INotification::AutoActivate);
		FNotifications->registerNotificationType(NNT_ROSTEREXCHANGE_REQUEST,notifyType);
	}

	if (FMessageWidgets)
	{
		FMessageWidgets->insertViewDropHandler(this);
	}
	
	if (FRostersViewPlugin)
	{
		FRostersViewPlugin->rostersView()->insertDragDropHandler(this);
	}

	return true;
}

bool RosterItemExchange::initSettings()
{
	Options::setDefaultValue(OPV_ROSTER_EXCHANGE_AUTOAPPROVEENABLED,true);
	return true;
}

bool RosterItemExchange::stanzaReadWrite(int AHandleId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept)
{
	if (FSHIExchangeRequest==AHandleId && AStanza.type()!="error")
	{
		QDomElement xElem = AStanza.firstElement("x",NS_ROSTERX);
		if (!xElem.isNull() && !xElem.firstChildElement("item").isNull())
		{
			AAccept = true;

			IRosterExchangeRequest request;
			request.streamJid = AStreamJid;
			request.contactJid = AStanza.from();
			request.id = AStanza.tagName()=="iq" ? AStanza.id() : QString::null;
			request.message = AStanza.tagName()=="message" ? Message(AStanza).body() : QString::null;

			QList<Jid> existItems;
			QDomElement itemElem = xElem.firstChildElement("item");

			bool isItemsValid = true;
			while (isItemsValid && !itemElem.isNull())
			{
				IRosterExchangeItem item;
				item.itemJid = Jid(itemElem.attribute("jid")).bare();
				item.name = itemElem.attribute("name");
				item.action = itemElem.attribute("action",ROSTEREXCHANGE_ACTION_ADD);

				QDomElement groupElem = itemElem.firstChildElement("group");
				while(!groupElem.isNull())
				{
					item.groups += groupElem.text();
					groupElem = groupElem.nextSiblingElement("group");
				}

				if (item.itemJid.isValid() && !existItems.contains(item.itemJid) && 
					(item.action==ROSTEREXCHANGE_ACTION_ADD || item.action==ROSTEREXCHANGE_ACTION_DELETE || item.action==ROSTEREXCHANGE_ACTION_MODIFY))
				{
					request.items.append(item);
					existItems.append(item.itemJid);
				}
				else
				{
					isItemsValid = false;
				}

				itemElem = itemElem.nextSiblingElement("item");
			}

			if (isItemsValid && !request.items.isEmpty())
			{
				processRequest(request);
			}
			else if (!request.id.isEmpty())
			{
				replyRequestError(request,XmppStanzaError::EC_BAD_REQUEST);
			}

			return true;
		}
	}
	return false;
}

void RosterItemExchange::stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza)
{
	Q_UNUSED(AStreamJid);
	if (FSentRequests.contains(AStanza.id()))
	{
		IRosterExchangeRequest request = FSentRequests.take(AStanza.id());
		if (AStanza.type()=="result")
			emit exchangeRequestApproved(request);
		else
			emit exchangeRequestFailed(request,XmppStanzaError(AStanza));
	}
}

QMultiMap<int, IOptionsWidget *> RosterItemExchange::optionsWidgets(const QString &ANodeId, QWidget *AParent)
{
	QMultiMap<int, IOptionsWidget *> widgets;
	if (ANodeId == OPN_ROSTER)
	{
		widgets.insertMulti(OWO_ROSTER_EXCHANGE,FOptionsManager->optionsNodeWidget(Options::node(OPV_ROSTER_EXCHANGE_AUTOAPPROVEENABLED),tr("Automatically accept roster modifications from gateways and group services"),AParent));
	}
	return widgets;
}

bool RosterItemExchange::messagaeViewDragEnter(IMessageViewWidget *AWidget, const QDragEnterEvent *AEvent)
{
	return !dropDataContacts(AWidget->messageWindow()->streamJid(),AWidget->messageWindow()->contactJid(),AEvent->mimeData()).isEmpty();
}

bool RosterItemExchange::messageViewDragMove(IMessageViewWidget *AWidget, const QDragMoveEvent *AEvent)
{
	Q_UNUSED(AWidget);
	Q_UNUSED(AEvent);
	return true;
}

void RosterItemExchange::messageViewDragLeave(IMessageViewWidget *AWidget, const QDragLeaveEvent *AEvent)
{
	Q_UNUSED(AWidget);
	Q_UNUSED(AEvent);
}

bool RosterItemExchange::messageViewDropAction(IMessageViewWidget *AWidget, const QDropEvent *AEvent, Menu *AMenu)
{
	return AEvent->dropAction()!=Qt::IgnoreAction ? insertDropActions(AWidget->messageWindow()->streamJid(),AWidget->messageWindow()->contactJid(),AEvent->mimeData(),AMenu) : false;
}

Qt::DropActions RosterItemExchange::rosterDragStart(const QMouseEvent *AEvent, IRosterIndex *AIndex, QDrag *ADrag)
{
	Q_UNUSED(AEvent); Q_UNUSED(ADrag);
	int indexKind = AIndex->data(RDR_KIND).toInt();
	if (indexKind==RIK_CONTACT || indexKind==RIK_AGENT || indexKind==RIK_GROUP)
		return Qt::CopyAction|Qt::MoveAction;
	return Qt::IgnoreAction;
}

bool RosterItemExchange::rosterDragEnter(const QDragEnterEvent *AEvent)
{
	if (AEvent->source()==FRostersViewPlugin->rostersView()->instance() && AEvent->mimeData()->hasFormat(DDT_ROSTERSVIEW_INDEX_DATA))
	{
		QMap<int, QVariant> indexData;
		QDataStream stream(AEvent->mimeData()->data(DDT_ROSTERSVIEW_INDEX_DATA));
		operator>>(stream,indexData);

		int indexKind = indexData.value(RDR_KIND).toInt();
		if (indexKind==RIK_CONTACT || indexKind==RIK_AGENT || indexKind==RIK_GROUP)
		{
			Jid indexJid = indexData.value(RDR_PREP_BARE_JID).toString();
			if (!indexJid.node().isEmpty())
			{
				QList<Jid> services = FGateways!=NULL ? FGateways->streamServices(indexData.value(RDR_STREAM_JID).toString()) : QList<Jid>();
				return !services.contains(indexJid.domain());
			}
			return true;
		}
	}
	return false;
}

bool RosterItemExchange::rosterDragMove(const QDragMoveEvent *AEvent, IRosterIndex *AHover)
{
	return !dropDataContacts(AHover->data(RDR_STREAM_JID).toString(),AHover->data(RDR_FULL_JID).toString(),AEvent->mimeData()).isEmpty();
}

void RosterItemExchange::rosterDragLeave(const QDragLeaveEvent *AEvent)
{
	Q_UNUSED(AEvent);
}

bool RosterItemExchange::rosterDropAction(const QDropEvent *AEvent, IRosterIndex *AHover, Menu *AMenu)
{
	if (AEvent->dropAction() != Qt::IgnoreAction)
		return insertDropActions(AHover->data(RDR_STREAM_JID).toString(),AHover->data(RDR_FULL_JID).toString(),AEvent->mimeData(),AMenu);
	return false;
}

bool RosterItemExchange::isSupported(const Jid &AStreamJid, const Jid &AContactJid) const
{
	return FDiscovery!=NULL && FDiscovery->discoInfo(AStreamJid,AContactJid).features.contains(NS_ROSTERX);
}

QString RosterItemExchange::sendExchangeRequest(const IRosterExchangeRequest &ARequest, bool AIqQuery)
{
	if (FStanzaProcessor && ARequest.streamJid.isValid() && ARequest.contactJid.isValid() && !ARequest.items.isEmpty())
	{
		Stanza request(AIqQuery ? "iq" : "message");
		request.setId(FStanzaProcessor->newId()).setTo(ARequest.contactJid.full());

		if (AIqQuery)
			request.setType("set");
		else if (!ARequest.message.isEmpty())
			request.addElement("body").appendChild(request.createTextNode(ARequest.message));

		bool isItemsValid = !ARequest.items.isEmpty();
		QDomElement xElem = request.addElement("x",NS_ROSTERX);
		for (QList<IRosterExchangeItem>::const_iterator it=ARequest.items.constBegin(); it!=ARequest.items.constEnd(); ++it)
		{
			if (it->itemJid.isValid() && (it->action==ROSTEREXCHANGE_ACTION_ADD || it->action==ROSTEREXCHANGE_ACTION_DELETE || it->action==ROSTEREXCHANGE_ACTION_MODIFY))
			{
				QDomElement itemElem = xElem.appendChild(request.createElement("item")).toElement();
				itemElem.setAttribute("action",it->action);
				itemElem.setAttribute("jid",it->itemJid.full());
				if (!it->name.isEmpty())
					itemElem.setAttribute("name",it->name);
				foreach(QString group, it->groups)
				{
					if (!group.isEmpty())
						itemElem.appendChild(request.createElement("group")).appendChild(request.createTextNode(group));
				}
			}
			else
			{
				isItemsValid = false;
			}
		}

		if (isItemsValid)
		{
			IRosterExchangeRequest sentRequest = ARequest;
			sentRequest.id = request.id();

			if (AIqQuery && FStanzaProcessor->sendStanzaRequest(this,ARequest.streamJid,request,0))
			{
				FSentRequests.insert(sentRequest.id,sentRequest);
				emit exchangeRequestSent(sentRequest);
				return request.id();
			}
			else if (!AIqQuery && FStanzaProcessor->sendStanzaOut(ARequest.streamJid,request))
			{
				emit exchangeRequestSent(sentRequest);
				return request.id();
			}
		}
	}
	return QString::null;
}

QList<IRosterItem> RosterItemExchange::dragDataContacts(const QMimeData *AData) const
{
	QList<IRosterItem> contactList;
	if (AData->hasFormat(DDT_ROSTERSVIEW_INDEX_DATA))
	{
		QMap<int, QVariant> indexData;
		QDataStream stream(AData->data(DDT_ROSTERSVIEW_INDEX_DATA));
		operator>>(stream,indexData);
		
		int indexKind = indexData.value(RDR_KIND).toInt();
		if (indexKind == RIK_GROUP)
		{
			QList<Jid> totalContacts;
			foreach(const Jid &streamJid, indexData.value(RDR_STREAMS).toStringList())
			{
				IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(streamJid) : NULL;
				QList<IRosterItem> ritems = roster!=NULL ? roster->groupItems(indexData.value(RDR_GROUP).toString()) : QList<IRosterItem>();
				for (QList<IRosterItem>::iterator it = ritems.begin(); it!=ritems.end(); ++it)
				{
					if (!totalContacts.contains(it->itemJid))
					{
						it->groups.clear();
						it->groups += indexData.value(RDR_NAME).toString();
						contactList.append(*it);
						totalContacts.append(it->itemJid);
					}
				}
			}
		}
		else if (indexKind==RIK_CONTACT || indexKind==RIK_AGENT)
		{
			IRosterItem ritem;
			ritem.isValid = true;
			ritem.itemJid = indexData.value(RDR_PREP_BARE_JID).toString();
			ritem.name = indexData.value(RDR_NAME).toString();
			contactList.append(ritem);
		}

		if (!contactList.isEmpty())
		{
			QList<Jid> services = FGateways!=NULL ? FGateways->streamServices(indexData.value(RDR_STREAM_JID).toString()) : QList<Jid>();
			for (QList<IRosterItem>::iterator it=contactList.begin(); it!=contactList.end(); )
			{
				if (!it->itemJid.node().isEmpty() && services.contains(it->itemJid.domain()))
					it = contactList.erase(it);
				else
					++it;
			}
		}
	}
	return contactList;
}

QList<IRosterItem> RosterItemExchange::dropDataContacts(const Jid &AStreamJid, const Jid &AContactJid, const QMimeData *AData) const
{
	QList<IRosterItem> contactList;
	if (isSupported(AStreamJid,AContactJid) && AData->hasFormat(DDT_ROSTERSVIEW_INDEX_DATA))
	{
		QMap<int, QVariant> indexData;
		QDataStream stream(AData->data(DDT_ROSTERSVIEW_INDEX_DATA));
		operator>>(stream,indexData);
		
		if (AStreamJid!=AContactJid || AStreamJid!=indexData.value(RDR_STREAM_JID).toString())
		{
			contactList = dragDataContacts(AData);
			for (QList<IRosterItem>::iterator it = contactList.begin(); it!=contactList.end(); )
			{
				if (AContactJid.pBare() == it->itemJid.pBare())
					it = contactList.erase(it);
				else
					++it;
			}
		}
	}
	return contactList;
}

bool RosterItemExchange::insertDropActions(const Jid &AStreamJid, const Jid &AContactJid, const QMimeData *AData, Menu *AMenu) const
{
	QList<IRosterItem> contactList = dropDataContacts(AStreamJid,AContactJid,AData);

	QStringList itemsJids;
	QStringList itemsNames;
	QStringList itemsGroups;
	for (QList<IRosterItem>::const_iterator it = contactList.constBegin(); it!=contactList.constEnd(); ++it)
	{
		itemsJids.append(it->itemJid.pBare());
		itemsNames.append(it->name);
		itemsGroups.append(it->groups.toList().value(0));
	}

	if (!itemsJids.isEmpty())
	{
		Action *action = new Action(AMenu);
		action->setText(tr("Send %n Contact(s)","",itemsJids.count()));
		action->setIcon(RSR_STORAGE_MENUICONS,MNI_ROSTEREXCHANGE_REQUEST);
		action->setData(ADR_STREAM_JID,AStreamJid.full());
		action->setData(ADR_CONTACT_JID,AContactJid.full());
		action->setData(ADR_ITEMS_JIDS, itemsJids);
		action->setData(ADR_ITEMS_NAMES, itemsNames);
		action->setData(ADR_ITEMS_GROUPS, itemsGroups);
		connect(action,SIGNAL(triggered()),SLOT(onSendExchangeRequestByAction()));
		AMenu->addAction(action, AG_DEFAULT, true);
		return true;
	}
	
	return false;
}

void RosterItemExchange::processRequest(const IRosterExchangeRequest &ARequest)
{
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(ARequest.streamJid) : NULL;
	if (roster && roster->rosterItem(ARequest.contactJid.bare()).isValid)
	{
		bool isGateway = false;
		bool isDirectory = false;
		if (ARequest.contactJid.node().isEmpty())
		{
			if (!ARequest.contactJid.isEmpty() && ARequest.contactJid!=ARequest.streamJid.bare() && ARequest.contactJid!=ARequest.streamJid.domain())
			{
				isGateway = true;
				if (FDiscovery && FDiscovery->hasDiscoInfo(ARequest.contactJid,ARequest.contactJid))
				{
					IDiscoInfo dinfo = FDiscovery->discoInfo(ARequest.streamJid,ARequest.contactJid);
					isDirectory = FDiscovery->findIdentity(dinfo.identity,"directory","group")>=0;
				}
			}
			else
			{
				isDirectory = true;
			}
		}

		bool isForbidden = false;
		QList<IRosterExchangeItem> approveList;
		bool autoApprove = (isGateway || isDirectory) && Options::node(OPV_ROSTER_EXCHANGE_AUTOAPPROVEENABLED).value().toBool();
		for(QList<IRosterExchangeItem>::const_iterator it=ARequest.items.constBegin(); it!=ARequest.items.constEnd(); ++it)
		{
			if (autoApprove && !isDirectory && isGateway && it->itemJid.pDomain()!=ARequest.contactJid.pDomain())
				autoApprove = false;

			IRosterItem ritem = roster->rosterItem(it->itemJid);
			if (!isGateway && !isDirectory && it->action!=ROSTEREXCHANGE_ACTION_ADD)
			{
				isForbidden = true;
			}
			else if (it->itemJid!=ARequest.streamJid.bare() && it->action==ROSTEREXCHANGE_ACTION_ADD)
			{
				if (!ritem.isValid)
					approveList.append(*it);
#if QT_VERSION >= QT_VERSION_CHECK(4,6,0)
				else if (!it->groups.isEmpty() && !ritem.groups.contains(it->groups))
#else
				else if (!it->groups.isEmpty())
#endif
					approveList.append(*it);
			}
			else if (ritem.isValid && it->action==ROSTEREXCHANGE_ACTION_DELETE)
			{
				approveList.append(*it);
			}
			else if (ritem.isValid && it->action==ROSTEREXCHANGE_ACTION_MODIFY)
			{
				if (ritem.name!=it->name || ritem.groups!=it->groups)
					approveList.append(*it);
			}
		}

		if (isForbidden)
		{
			replyRequestError(ARequest,XmppStanzaError::EC_FORBIDDEN);
		}
		else if (!approveList.isEmpty())
		{
			IRosterExchangeRequest request = ARequest;
			request.items = approveList;
			emit exchangeRequestReceived(request);

			if (!autoApprove)
			{
				ExchangeApproveDialog *dialog = new ExchangeApproveDialog(roster,request);
				dialog->installEventFilter(this);
				connect(dialog,SIGNAL(accepted()),SLOT(onExchangeApproveDialogAccepted()));
				connect(dialog,SIGNAL(rejected()),SLOT(onExchangeApproveDialogRejected()));
				connect(dialog,SIGNAL(dialogDestroyed()),SLOT(onExchangeApproveDialogDestroyed()));
				notifyExchangeRequest(dialog);
			}
			else
			{
				applyRequest(request,true,true);
				replyRequestResult(request);
			}
		}
		else
		{
			replyRequestResult(ARequest);
		}
	}
	else
	{
		replyRequestError(ARequest,XmppStanzaError::EC_NOT_AUTHORIZED);
	}
}

void RosterItemExchange::notifyExchangeRequest(ExchangeApproveDialog *ADialog)
{
	if (FNotifications)
	{
		IRosterExchangeRequest request = ADialog->receivedRequest();

		INotification notify;
		notify.kinds =  FNotifications->enabledTypeNotificationKinds(NNT_ROSTEREXCHANGE_REQUEST);
		if (notify.kinds > 0)
		{
			notify.typeId = NNT_ROSTEREXCHANGE_REQUEST;
			notify.data.insert(NDR_ICON,IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_ROSTEREXCHANGE_REQUEST));
			notify.data.insert(NDR_TOOLTIP,tr("Roster modification request from %1").arg(FNotifications->contactName(request.streamJid,request.contactJid)));
			notify.data.insert(NDR_STREAM_JID,request.streamJid.full());
			notify.data.insert(NDR_CONTACT_JID,request.contactJid.full());
			notify.data.insert(NDR_ROSTER_ORDER,RNO_ROSTEREXCHANGE_REQUEST);
			notify.data.insert(NDR_ROSTER_FLAGS,IRostersNotify::Blink|IRostersNotify::AllwaysVisible|IRostersNotify::HookClicks);
			notify.data.insert(NDR_ROSTER_CREATE_INDEX,false);
			notify.data.insert(NDR_POPUP_CAPTION, tr("Roster modification"));
			notify.data.insert(NDR_POPUP_TITLE,FNotifications->contactName(request.streamJid,request.contactJid));
			notify.data.insert(NDR_POPUP_IMAGE,FNotifications->contactAvatar(request.contactJid));
			notify.data.insert(NDR_POPUP_HTML,Qt::escape(tr("%1 offers you to make some changes in your contact list.").arg(FNotifications->contactName(request.streamJid,request.contactJid))));
			notify.data.insert(NDR_SOUND_FILE,SDF_ROSTEREXCHANGE_REQUEST);
			notify.data.insert(NDR_ALERT_WIDGET,(qint64)ADialog);
			notify.data.insert(NDR_SHOWMINIMIZED_WIDGET,(qint64)ADialog);
			FNotifyApproveDialog.insert(FNotifications->appendNotification(notify),ADialog);
		}
		else
		{
			ADialog->reject();
		}
	}
	else
	{
		WidgetManager::showActivateRaiseWindow(ADialog);
	}
}

bool RosterItemExchange::applyRequest(const IRosterExchangeRequest &ARequest, bool ASubscribe, bool ASilent)
{
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(ARequest.streamJid) : NULL;
	if (roster && roster->isOpen())
	{
		bool applied = false;
		for(QList<IRosterExchangeItem>::const_iterator it=ARequest.items.constBegin(); it!=ARequest.items.constEnd(); ++it)
		{
			IRosterItem ritem = roster->rosterItem(it->itemJid);
			if (it->action == ROSTEREXCHANGE_ACTION_ADD)
			{
				if (!ritem.isValid)
				{
					applied = true;
					roster->setItem(it->itemJid,it->name,it->groups);
					if (ASubscribe)
					{
						if (FRosterChanger)
							FRosterChanger->subscribeContact(ARequest.streamJid,it->itemJid,QString::null,ASilent);
						else
							roster->sendSubscription(it->itemJid,IRoster::Subscribe);
					}
				}
#if QT_VERSION >= QT_VERSION_CHECK(4,6,0)
				else if (!it->groups.isEmpty() && !ritem.groups.contains(it->groups))
#else
				else if (!it->groups.isEmpty())
#endif
				{
					applied = true;
					roster->setItem(ritem.itemJid,ritem.name,ritem.groups+it->groups);
				}
			}
			else if (ritem.isValid && it->action==ROSTEREXCHANGE_ACTION_DELETE)
			{
				applied = true;
				if (!it->groups.isEmpty())
					roster->setItem(ritem.itemJid,ritem.name,ritem.groups-it->groups);
				else
					roster->removeItem(ritem.itemJid);
			}
			else if (ritem.isValid && it->action==ROSTEREXCHANGE_ACTION_MODIFY)
			{
				if (ritem.name!=it->name || ritem.groups!=it->groups)
				{
					applied =true;
					roster->setItem(ritem.itemJid,it->name,it->groups);
				}
			}
		}
		emit exchangeRequestApplied(ARequest);
		return applied;
	}
	return false;
}

void RosterItemExchange::replyRequestResult(const IRosterExchangeRequest &ARequest)
{
	if (FStanzaProcessor && !ARequest.id.isEmpty())
	{
		Stanza result("iq");
		result.setType("result").setId(ARequest.id).setTo(ARequest.contactJid.full());
		FStanzaProcessor->sendStanzaOut(ARequest.streamJid,result);
	}
	emit exchangeRequestApproved(ARequest);
}

void RosterItemExchange::replyRequestError(const IRosterExchangeRequest &ARequest, const XmppStanzaError &AError)
{
	if (FStanzaProcessor && !ARequest.id.isEmpty())
	{
		Stanza error("iq");
		error.setId(ARequest.id).setTo(ARequest.streamJid.full()).setFrom(ARequest.contactJid.full());
		error = FStanzaProcessor->makeReplyError(error,AError);
		FStanzaProcessor->sendStanzaOut(ARequest.streamJid,error);
	}
	emit exchangeRequestFailed(ARequest,AError);
}

void RosterItemExchange::notifyInChatWindow(const Jid &AStreamJid, const Jid &AContactJid, const QString &AMessage) const
{
	IMessageChatWindow *window = FMessageWidgets!=NULL ? FMessageWidgets->findChatWindow(AStreamJid,AContactJid) : NULL;
	if (window)
	{
		IMessageContentOptions options;
		options.kind = IMessageContentOptions::KindStatus;
		options.type |= IMessageContentOptions::TypeEvent;
		options.direction = IMessageContentOptions::DirectionIn;
		options.time = QDateTime::currentDateTime();
		window->viewWidget()->appendText(AMessage,options);
	}
}

bool RosterItemExchange::eventFilter( QObject *AObject, QEvent *AEvent )
{
	if (AEvent->type() == QEvent::WindowActivate)
	{
		if (FNotifications)
		{
			int notifyId = FNotifyApproveDialog.key(qobject_cast<ExchangeApproveDialog *>(AObject));
			FNotifications->activateNotification(notifyId);
		}
	}
	return QObject::eventFilter(AObject,AEvent);
}

void RosterItemExchange::onSendExchangeRequestByAction()
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		IRosterExchangeRequest request;
		request.streamJid = action->data(ADR_STREAM_JID).toString();
		request.contactJid = action->data(ADR_CONTACT_JID).toString();

		QStringList itemJids = action->data(ADR_ITEMS_JIDS).toStringList();
		QStringList itemNames = action->data(ADR_ITEMS_NAMES).toStringList();
		QStringList itemGroups = action->data(ADR_ITEMS_GROUPS).toStringList();
		for (int i=0; i<itemJids.count(); i++)
		{
			IRosterExchangeItem item;
			item.action = ROSTEREXCHANGE_ACTION_ADD;
			item.itemJid = itemJids.value(i);
			item.name = itemNames.value(i);
			if (!itemGroups.value(i).isEmpty())
				item.groups += itemGroups.value(i);
			request.items.append(item);
		}

		if (!sendExchangeRequest(request,false).isEmpty())
			notifyInChatWindow(request.streamJid,request.contactJid,tr("%n contact(s) have been sent","",request.items.count()));
		else
			notifyInChatWindow(request.streamJid,request.contactJid,tr("Failed to send %n contact(s)","",request.items.count()));
	}
}

void RosterItemExchange::onNotificationActivated(int ANotifyId)
{
	if (FNotifyApproveDialog.contains(ANotifyId))
	{
		ExchangeApproveDialog *dialog = FNotifyApproveDialog.take(ANotifyId);
		WidgetManager::showActivateRaiseWindow(dialog);
		FNotifications->removeNotification(ANotifyId);
	}
}

void RosterItemExchange::onNotificationRemoved(int ANotifyId)
{
	if (FNotifyApproveDialog.contains(ANotifyId))
	{
		ExchangeApproveDialog *dialog = FNotifyApproveDialog.take(ANotifyId);
		dialog->reject();
	}
}

void RosterItemExchange::onExchangeApproveDialogAccepted()
{
	ExchangeApproveDialog *dialog = qobject_cast<ExchangeApproveDialog *>(sender());
	if (dialog)
	{
		IRosterExchangeRequest request = dialog->approvedRequest();
		applyRequest(request,dialog->subscribeNewContacts(),false);
		replyRequestResult(request);
	}
}

void RosterItemExchange::onExchangeApproveDialogRejected()
{
	ExchangeApproveDialog *dialog = qobject_cast<ExchangeApproveDialog *>(sender());
	if (dialog)
	{
		replyRequestError(dialog->receivedRequest(),XmppStanzaError::EC_NOT_ALLOWED);
	}
}

void RosterItemExchange::onExchangeApproveDialogDestroyed()
{
	ExchangeApproveDialog *dialog = qobject_cast<ExchangeApproveDialog *>(sender());
	if (FNotifications && dialog)
	{
		int notifyId = FNotifyApproveDialog.key(dialog);
		FNotifications->removeNotification(notifyId);
	}
}

#ifndef HAVE_QT5
Q_EXPORT_PLUGIN2(plg_rosteritemexchange, RosterItemExchange)
#endif
