#include "privacylists.h"

#define SHC_PRIVACY         "/iq[@type='set']/query[@xmlns='"NS_JABBER_PRIVACY"']"
#define SHC_ROSTER          "/iq/query[@xmlns='"NS_JABBER_ROSTER"']"

#define PRIVACY_TIMEOUT     60000
#define AUTO_LISTS_TIMEOUT  2000

#define ADR_STREAM_JID      Action::DR_StreamJid
#define ADR_CONTACT_JID     Action::DR_Parametr1
#define ADR_GROUP_NAME      Action::DR_Parametr2
#define ADR_LISTNAME        Action::DR_Parametr3

QStringList AutoLists = QStringList() << PRIVACY_LIST_VISIBLE << PRIVACY_LIST_CONFERENCES << PRIVACY_LIST_INVISIBLE << PRIVACY_LIST_IGNORE << PRIVACY_LIST_SUBSCRIPTION;

PrivacyLists::PrivacyLists()
{
	FXmppStreams = NULL;
	FRostersModel = NULL;
	FRostersView = NULL;
	FRostersViewPlugin = NULL;
	FStanzaProcessor = NULL;
	FRosterPlugin = NULL;

	FPrivacyLabelId = 0;
	FApplyAutoListsTimer.setSingleShot(true);
	FApplyAutoListsTimer.setInterval(AUTO_LISTS_TIMEOUT);
	connect(&FApplyAutoListsTimer,SIGNAL(timeout()),SLOT(onApplyAutoLists()));

	connect(this,SIGNAL(listAboutToBeChanged(const Jid &, const IPrivacyList &)),SLOT(onListAboutToBeChanged(const Jid &, const IPrivacyList &)));
	connect(this,SIGNAL(listLoaded(const Jid &, const QString &)),SLOT(onListChanged(const Jid &, const QString &)));
	connect(this,SIGNAL(listRemoved(const Jid &, const QString &)),SLOT(onListChanged(const Jid &, const QString &)));
	connect(this,SIGNAL(activeListAboutToBeChanged(const Jid &, const QString &)),SLOT(onActiveListAboutToBeChanged(const Jid &, const QString &)));
	connect(this,SIGNAL(activeListChanged(const Jid &, const QString &)),SLOT(onActiveListChanged(const Jid &, const QString &)));
}

PrivacyLists::~PrivacyLists()
{

}

void PrivacyLists::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Privacy Lists");
	APluginInfo->description = tr("Allows to block unwanted contacts");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(XMPPSTREAMS_UUID);
	APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool PrivacyLists::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
	IPlugin *plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0,NULL);
	if (plugin)
		FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
	if (plugin)
	{
		FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
		if (FXmppStreams)
		{
			connect(FXmppStreams->instance(), SIGNAL(opened(IXmppStream *)), SLOT(onStreamOpened(IXmppStream *)));
			connect(FXmppStreams->instance(), SIGNAL(closed(IXmppStream *)), SLOT(onStreamClosed(IXmppStream *)));
		}
	}

	plugin = APluginManager->pluginInterface("IRostersModel").value(0,NULL);
	if (plugin)
	{
		FRostersModel = qobject_cast<IRostersModel *>(plugin->instance());
		if (FRostersModel)
		{
			connect(FRostersModel->instance(),SIGNAL(indexCreated(IRosterIndex *)),SLOT(onRosterIndexCreated(IRosterIndex *)));
		}
	}

	plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
	if (plugin)
		FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IRosterPlugin").value(0,NULL);
	if (plugin)
		FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IPresencePlugin").value(0,NULL);
	if (plugin)
		FPresencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IMultiUserChatPlugin").value(0,NULL);
	if (plugin)
	{
		connect(plugin->instance(),SIGNAL(multiUserChatCreated(IMultiUserChat *)),SLOT(onMultiUserChatCreated(IMultiUserChat *)));
	}

	return FXmppStreams!=NULL && FStanzaProcessor!=NULL;
}

bool PrivacyLists::initObjects()
{
	if (FRostersViewPlugin)
	{
		AdvancedDelegateItem label(RLID_PRIVACY_STATUS);
		label.d->kind = AdvancedDelegateItem::CustomData;
		label.d->data = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_PRIVACYLISTS_INVISIBLE);
		FPrivacyLabelId = FRostersViewPlugin->rostersView()->registerLabel(label);

		FRostersView = FRostersViewPlugin->rostersView();
		connect(FRostersView->instance(),SIGNAL(indexMultiSelection(const QList<IRosterIndex *> &, bool &)), 
			SLOT(onRostersViewIndexMultiSelection(const QList<IRosterIndex *> &, bool &)));
		connect(FRostersView->instance(),SIGNAL(indexContextMenu(const QList<IRosterIndex *> &, quint32, Menu *)), 
			SLOT(onRostersViewIndexContextMenu(const QList<IRosterIndex *> &, quint32, Menu *)));
		connect(FRostersView->instance(),SIGNAL(indexToolTips(IRosterIndex *, quint32, QMap<int,QString> &)),
			SLOT(onRostersViewIndexToolTips(IRosterIndex *, quint32, QMap<int,QString> &)));
	}
	return true;
}

bool PrivacyLists::stanzaReadWrite(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept)
{
	if (FSHIPrivacy.value(AStreamJid)==AHandlerId && AStanza.isFromServer())
	{
		QDomElement queryElem = AStanza.firstElement("query",NS_JABBER_PRIVACY);
		QDomElement listElem = queryElem.firstChildElement("list");
		QString listName = listElem.attribute("name");
		if (!listName.isEmpty())
		{
			bool needLoad = FRemoveRequests.key(listName).isEmpty();
			for (QMap<QString,IPrivacyList>::const_iterator it = FSaveRequests.constBegin(); needLoad && it!=FSaveRequests.constEnd(); ++it)
				needLoad = it.value().name != listName;
			if (needLoad)
				loadPrivacyList(AStreamJid,listName);

			AAccept = true;
			Stanza result = FStanzaProcessor->makeReplyResult(AStanza);
			FStanzaProcessor->sendStanzaOut(AStreamJid,result);
		}
	}
	else if (FSHIRosterIn.value(AStreamJid)==AHandlerId || FSHIRosterOut.value(AStreamJid)==AHandlerId)
	{
		IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->findPresence(AStreamJid) : NULL;
		if (presence && presence->isOpen() && !activeList(AStreamJid).isEmpty())
		{
			bool directionIn = FSHIRosterIn.value(AStreamJid)==AHandlerId;
			QDomElement itemElem = AStanza.firstElement("query",NS_JABBER_ROSTER).firstChildElement("item");
			while (!itemElem.isNull())
			{
				IRosterItem ritem;
				ritem.itemJid = itemElem.attribute("jid");
				ritem.isValid = ritem.itemJid.isValid() && ritem.itemJid.resource().isEmpty();
				if (ritem.isValid)
				{
					ritem.subscription = itemElem.attribute("subscription");
					QDomElement groupElem = itemElem.firstChildElement("group");
					while (!groupElem.isNull())
					{
						ritem.groups += groupElem.text();
						groupElem = groupElem.nextSiblingElement("group");
					}

					int stanzas = denyedStanzas(ritem,privacyList(AStreamJid,activeList(AStreamJid)));
					bool denied = (stanzas & IPrivacyRule::PresencesOut)>0;
					if (denied && !FOfflinePresences.value(AStreamJid).contains(ritem.itemJid))
					{
						presence->sendPresence(ritem.itemJid,IPresence::Offline,QString::null,0);
						FOfflinePresences[AStreamJid]+=ritem.itemJid;
					}
					else if (!denied && directionIn && FOfflinePresences.value(AStreamJid).contains(ritem.itemJid))
					{
						if (ritem.subscription==SUBSCRIPTION_BOTH || ritem.subscription==SUBSCRIPTION_FROM)
							presence->sendPresence(ritem.itemJid,presence->show(),presence->status(),presence->priority());
						FOfflinePresences[AStreamJid]-=ritem.itemJid;
					}

					if (directionIn)
					{
						denied = (stanzas & IPrivacyRule::AnyStanza)>0;
						if (FLabeledContacts.value(AStreamJid).contains(ritem.itemJid)!=denied)
							setPrivacyLabel(AStreamJid,ritem.itemJid,denied);
					}
				}

				itemElem = itemElem.nextSiblingElement("item");
			}
		}
	}
	return false;
}

void PrivacyLists::stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza)
{
	if (FLoadRequests.contains(AStanza.id()))
	{
		if (AStanza.type() == "result")
		{
			QMap<QString,IPrivacyList> &lists = FPrivacyLists[AStreamJid];
			QDomElement queryElem = AStanza.firstElement("query",NS_JABBER_PRIVACY);

			QDomElement listElem = queryElem.firstChildElement("list");
			while (!listElem.isNull())
			{
				QString listName = listElem.attribute("name");
				IPrivacyList &list = lists[listName];
				list.name = listName;
				list.rules.clear();

				QDomElement ruleElem = listElem.firstChildElement("item");
				while (!ruleElem.isNull())
				{
					IPrivacyRule rule;
					rule.order = ruleElem.attribute("order").toInt();
					rule.type = ruleElem.attribute("type");
					rule.value = ruleElem.attribute("value");
					if (rule.type == PRIVACY_TYPE_JID)
						rule.value = Jid(rule.value).pFull();
					rule.action = ruleElem.attribute("action");
					rule.stanzas = IPrivacyRule::EmptyType;
					if (!ruleElem.firstChildElement("message").isNull())
						rule.stanzas |= IPrivacyRule::Messages;
					if (!ruleElem.firstChildElement("iq").isNull())
						rule.stanzas |= IPrivacyRule::Queries;
					if (!ruleElem.firstChildElement("presence-in").isNull())
						rule.stanzas |= IPrivacyRule::PresencesIn;
					if (!ruleElem.firstChildElement("presence-out").isNull())
						rule.stanzas |= IPrivacyRule::PresencesOut;
					if (rule.stanzas == IPrivacyRule::EmptyType)
						rule.stanzas = IPrivacyRule::AnyStanza;
					list.rules.append(rule);
					ruleElem = ruleElem.nextSiblingElement("item");
				}
				qSort(list.rules);

				if (!list.rules.isEmpty())
					emit listLoaded(AStreamJid,list.name);
				else
					loadPrivacyList(AStreamJid,listName);

				listElem = listElem.nextSiblingElement("list");
			}

			QDomElement activeElem = queryElem.firstChildElement("active");
			if (!activeElem.isNull())
			{
				QString activeListName = activeElem.attribute("name");
				FActiveLists.insert(AStreamJid,activeListName);
				emit activeListChanged(AStreamJid,activeListName);
			}

			QDomElement defaultElem =queryElem.firstChildElement("default");
			if (!defaultElem.isNull())
			{
				QString defaultListName = defaultElem.attribute("name");
				FDefaultLists.insert(AStreamJid,defaultListName);
				emit defaultListChanged(AStreamJid,defaultListName);
			}

			if (FLoadRequests.value(AStanza.id()).isEmpty())
			{
				if (lists.isEmpty())
					setAutoPrivacy(AStreamJid,PRIVACY_LIST_AUTO_VISIBLE);
				else if (defaultList(AStreamJid)!=activeList(AStreamJid))
					setActiveList(AStreamJid,defaultList(AStreamJid));
			}
		}
		else if (AStanza.type() == "error")
		{
			QString listName = FLoadRequests.value(AStanza.id());
			if (FPrivacyLists.value(AStreamJid).contains(listName))
			{
				FPrivacyLists[AStreamJid].remove(listName);
				emit listRemoved(AStreamJid,listName);
			}
		}
		FLoadRequests.remove(AStanza.id());
	}
	else if (FSaveRequests.contains(AStanza.id()))
	{
		if (AStanza.type() == "result")
		{
			IPrivacyList list = FSaveRequests.value(AStanza.id());
			FPrivacyLists[AStreamJid].insert(list.name,list);
			emit listLoaded(AStreamJid,list.name);
		}
		FSaveRequests.remove(AStanza.id());
	}
	else if (FActiveRequests.contains(AStanza.id()))
	{
		if (AStanza.type() == "result")
		{
			QString activeListName = FActiveRequests.value(AStanza.id());
			FActiveLists.insert(AStreamJid,activeListName);
			emit activeListChanged(AStreamJid,activeListName);
		}
		FActiveRequests.remove(AStanza.id());
	}
	else if (FDefaultRequests.contains(AStanza.id()))
	{
		if (AStanza.type() == "result")
		{
			QString defaultListName = FDefaultRequests.value(AStanza.id());
			FDefaultLists.insert(AStreamJid,defaultListName);
			emit defaultListChanged(AStreamJid,defaultListName);
		}
		FDefaultRequests.remove(AStanza.id());
	}
	else if (FRemoveRequests.contains(AStanza.id()))
	{
		if (AStanza.type() == "result")
		{
			QString listName = FRemoveRequests.value(AStanza.id());
			FPrivacyLists[AStreamJid].remove(listName);
			emit listRemoved(AStreamJid,listName);
		}
		FRemoveRequests.remove(AStanza.id());
	}
	FStreamRequests[AStreamJid].removeAt(FStreamRequests[AStreamJid].indexOf(AStanza.id()));

	if (AStanza.type() == "result")
		emit requestCompleted(AStanza.id());
	else
		emit requestFailed(AStanza.id(),XmppStanzaError(AStanza));
}

bool PrivacyLists::isReady(const Jid &AStreamJid) const
{
	return FPrivacyLists.contains(AStreamJid);
}

IPrivacyRule PrivacyLists::groupAutoListRule(const QString &AGroup, const QString &AAutoList) const
{
	IPrivacyRule rule = contactAutoListRule(Jid::null,AAutoList);
	rule.type = PRIVACY_TYPE_GROUP;
	rule.value = AGroup;
	return rule;
}

IPrivacyRule PrivacyLists::contactAutoListRule(const Jid &AContactJid, const QString &AList) const
{
	IPrivacyRule rule;
	rule.order = 0;
	rule.type = PRIVACY_TYPE_JID;
	rule.value = AContactJid.pFull();
	rule.stanzas = IPrivacyRule::EmptyType;
	if (AList == PRIVACY_LIST_VISIBLE)
	{
		rule.action = PRIVACY_ACTION_ALLOW;
		rule.stanzas = IPrivacyRule::PresencesOut;
	}
	else if (AList == PRIVACY_LIST_INVISIBLE)
	{
		rule.action = PRIVACY_ACTION_DENY;
		rule.stanzas = IPrivacyRule::PresencesOut;
	}
	else if (AList == PRIVACY_LIST_IGNORE)
	{
		rule.action = PRIVACY_ACTION_DENY;
		rule.stanzas = IPrivacyRule::AnyStanza;
	}
	else if (AList == PRIVACY_LIST_CONFERENCES)
	{
		rule.action = PRIVACY_ACTION_ALLOW;
		rule.stanzas = IPrivacyRule::AnyStanza;
	}
	return rule;
}

bool PrivacyLists::isGroupAutoListed(const Jid &AStreamJid, const QString &AGroup, const QString &AList) const
{
	IPrivacyRule rule = groupAutoListRule(AGroup,AList);
	return privacyList(AStreamJid,AList,true).rules.contains(rule);
}

bool PrivacyLists::isContactAutoListed(const Jid &AStreamJid, const Jid &AContactJid, const QString &AList) const
{
	IPrivacyRule rule = contactAutoListRule(AContactJid,AList);
	return privacyList(AStreamJid,AList,true).rules.contains(rule);
}

void PrivacyLists::setGroupAutoListed(const Jid &AStreamJid, const QString &AGroup, const QString &AListName, bool APresent)
{
	IPrivacyRule rule = groupAutoListRule(AGroup,AListName);
	if (isReady(AStreamJid) && !AGroup.isEmpty() && rule.stanzas!=IPrivacyRule::EmptyType)
	{
		IPrivacyList list = privacyList(AStreamJid,AListName,true);
		list.name = AListName;

		if (APresent != list.rules.contains(rule))
		{
			if (APresent)
			{
				setGroupAutoListed(AStreamJid,AGroup,PRIVACY_LIST_VISIBLE,false);
				setGroupAutoListed(AStreamJid,AGroup,PRIVACY_LIST_INVISIBLE,false);
				setGroupAutoListed(AStreamJid,AGroup,PRIVACY_LIST_IGNORE,false);
			}

			IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AStreamJid) : NULL;
			QStringList groups = roster!=NULL ? (roster->allGroups()<<AGroup).toList() : QStringList(AGroup);
			qSort(groups);

			foreach(QString group, groups)
			{
				if (roster->isSubgroup(AGroup,group))
				{
					rule.value = group;
					if (APresent)
					{
						if (!isGroupAutoListed(AStreamJid,group,PRIVACY_LIST_VISIBLE)    &&
						    !isGroupAutoListed(AStreamJid,group,PRIVACY_LIST_INVISIBLE)  &&
						    !isGroupAutoListed(AStreamJid,group,PRIVACY_LIST_IGNORE))
						{
							list.rules.append(rule);
						}
					}
					else
					{
						list.rules.removeAt(list.rules.indexOf(rule));
					}
				}
			}

			for (int i=0; i<list.rules.count();i++)
				list.rules[i].order=i;

			!list.rules.isEmpty() ? savePrivacyList(AStreamJid,list) : removePrivacyList(AStreamJid,AListName);
		}
	}
}

void PrivacyLists::setContactAutoListed(const Jid &AStreamJid, const Jid &AContactJid, const QString &AList, bool APresent)
{
	IPrivacyRule rule = contactAutoListRule(AContactJid,AList);
	if (isReady(AStreamJid) && rule.stanzas!=IPrivacyRule::EmptyType)
	{
		IPrivacyList list = privacyList(AStreamJid,AList,true);
		list.name = AList;

		if (APresent != list.rules.contains(rule))
		{
			if (APresent)
			{
				setContactAutoListed(AStreamJid,AContactJid,PRIVACY_LIST_VISIBLE,false);
				setContactAutoListed(AStreamJid,AContactJid,PRIVACY_LIST_INVISIBLE,false);
				setContactAutoListed(AStreamJid,AContactJid,PRIVACY_LIST_IGNORE,false);
				setContactAutoListed(AStreamJid,AContactJid,PRIVACY_LIST_CONFERENCES,false);
				list.rules.append(rule);
			}
			else
			{
				list.rules.removeAt(list.rules.indexOf(rule));
			}

			for (int i=0; i<list.rules.count();i++)
				list.rules[i].order=i;

			!list.rules.isEmpty() ? savePrivacyList(AStreamJid,list) : removePrivacyList(AStreamJid,AList);
		}
	}
}

IPrivacyRule PrivacyLists::offRosterRule() const
{
	IPrivacyRule rule;
	rule.type = PRIVACY_TYPE_SUBSCRIPTION;
	rule.value = SUBSCRIPTION_NONE;
	rule.action = PRIVACY_ACTION_DENY;
	rule.stanzas = IPrivacyRule::AnyStanza;
	return rule;
}

bool PrivacyLists::isOffRosterBlocked(const Jid &AStreamJid) const
{
	IPrivacyRule rule = offRosterRule();
	IPrivacyList list = privacyList(AStreamJid,PRIVACY_LIST_SUBSCRIPTION,true);
	return list.rules.contains(rule);
}

void PrivacyLists::setOffRosterBlocked(const Jid &AStreamJid, bool ABlocked)
{
	IPrivacyRule rule = offRosterRule();
	IPrivacyList list = privacyList(AStreamJid,PRIVACY_LIST_SUBSCRIPTION,true);
	if (list.rules.contains(rule) != ABlocked)
	{
		list.name = PRIVACY_LIST_SUBSCRIPTION;
		if (ABlocked)
			list.rules.append(rule);
		else
			list.rules.removeAt(list.rules.indexOf(rule));

		for (int i=0; i<list.rules.count();i++)
			list.rules[i].order = i;

		!list.rules.isEmpty() ? savePrivacyList(AStreamJid,list) : removePrivacyList(AStreamJid,list.name);
	}
}

bool PrivacyLists::isAutoPrivacy(const Jid &AStreamJid) const
{
	if (isReady(AStreamJid))
	{
		QString listName = activeList(AStreamJid,true);
		return listName==PRIVACY_LIST_AUTO_VISIBLE || listName==PRIVACY_LIST_AUTO_INVISIBLE;
	}
	return false;
}

void PrivacyLists::setAutoPrivacy(const Jid &AStreamJid, const QString &AAutoList)
{
	if (isReady(AStreamJid) && activeList(AStreamJid,true)!=AAutoList)
	{
		if (AAutoList == PRIVACY_LIST_AUTO_VISIBLE)
		{
			FApplyAutoLists.insert(AStreamJid,AAutoList);
			onApplyAutoLists();
			setDefaultList(AStreamJid,AAutoList);
			setActiveList(AStreamJid,AAutoList);
		}
		else if (AAutoList == PRIVACY_LIST_AUTO_INVISIBLE)
		{
			FApplyAutoLists.insert(AStreamJid,AAutoList);
			onApplyAutoLists();
			setDefaultList(AStreamJid,AAutoList);
			setActiveList(AStreamJid,AAutoList);
		}
		else
		{
			FApplyAutoLists.remove(AStreamJid);
			setDefaultList(AStreamJid,QString::null);
			setActiveList(AStreamJid,QString::null);
		}
	}
}

int PrivacyLists::denyedStanzas(const IRosterItem &AItem, const IPrivacyList &AList) const
{
	int denied = 0;
	int allowed = 0;
	foreach(IPrivacyRule rule, AList.rules)
	{
		int stanzas = 0;
		if (rule.type == PRIVACY_TYPE_ALWAYS)
		{
			stanzas = rule.stanzas;
		}
		else if (rule.type == PRIVACY_TYPE_GROUP)
		{
			if (AItem.groups.contains(rule.value))
				stanzas = rule.stanzas;
		}
		else if (rule.type == PRIVACY_TYPE_SUBSCRIPTION)
		{
			if (AItem.subscription == rule.value)
				stanzas = rule.stanzas;
		}
		else if (rule.type == PRIVACY_TYPE_JID)
		{
			if (isMatchedJid(rule.value,AItem.itemJid))
				stanzas = rule.stanzas;
		}

		if (rule.action == PRIVACY_ACTION_DENY)
			denied |= stanzas & (~allowed);
		else
			allowed |= stanzas & (~denied);
	}
	return denied;
}

QHash<Jid,int> PrivacyLists::denyedContacts(const Jid &AStreamJid, const IPrivacyList &AList, int AFilter) const
{
	QHash<Jid,int> denied;
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AStreamJid) : NULL;
	QList<IRosterItem> ritems = roster!=NULL ? roster->rosterItems() : QList<IRosterItem>();
	foreach(IRosterItem ritem,ritems)
	{
		int stanzas = denyedStanzas(ritem,AList);
		if ((stanzas & AFilter) > 0)
			denied[ritem.itemJid] = stanzas;
	}
	return denied;
}

QString PrivacyLists::activeList(const Jid &AStreamJid, bool APending) const
{
	if (APending)
	{
		foreach(QString id, FStreamRequests.value(AStreamJid))
		{
			if (FActiveRequests.contains(id))
				return FActiveRequests.value(id);
		}
	}
	return FActiveLists.value(AStreamJid);
}

QString PrivacyLists::setActiveList(const Jid &AStreamJid, const QString &AList)
{
	if (isReady(AStreamJid) && AList!=activeList(AStreamJid))
	{
		Stanza set("iq");
		set.setType("set").setId(FStanzaProcessor->newId());
		QDomElement queryElem = set.addElement("query",NS_JABBER_PRIVACY);
		QDomElement activeElem = queryElem.appendChild(set.createElement("active")).toElement();
		if (!AList.isEmpty())
			activeElem.setAttribute("name",AList);

		emit activeListAboutToBeChanged(AStreamJid,AList);
		if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,set,PRIVACY_TIMEOUT))
		{
			FStreamRequests[AStreamJid].prepend(set.id());
			FActiveRequests.insert(set.id(),AList);
			return set.id();
		}
	}
	return QString::null;
}

QString PrivacyLists::defaultList(const Jid &AStreamJid, bool APending) const
{
	if (APending)
	{
		foreach(QString id, FStreamRequests.value(AStreamJid))
		{
			if (FDefaultRequests.contains(id))
				return FDefaultRequests.value(id);
		}
	}
	return FDefaultLists.value(AStreamJid);
}

QString PrivacyLists::setDefaultList(const Jid &AStreamJid, const QString &AList)
{
	if (isReady(AStreamJid) && AList!=defaultList(AStreamJid))
	{
		Stanza set("iq");
		set.setType("set").setId(FStanzaProcessor->newId());
		QDomElement queryElem = set.addElement("query",NS_JABBER_PRIVACY);
		QDomElement defaultElem = queryElem.appendChild(set.createElement("default")).toElement();
		if (!AList.isEmpty())
			defaultElem.setAttribute("name",AList);
		if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,set,PRIVACY_TIMEOUT))
		{
			FStreamRequests[AStreamJid].prepend(set.id());
			FDefaultRequests.insert(set.id(),AList);
			return set.id();
		}
	}
	return QString::null;
}

IPrivacyList PrivacyLists::privacyList(const Jid &AStreamJid, const QString &AList, bool APending) const
{
	if (APending)
	{
		foreach(QString id, FStreamRequests.value(AStreamJid))
		{
			if (FSaveRequests.value(id).name == AList)
				return FSaveRequests.value(id);
			else if (FRemoveRequests.value(id) == AList)
				return IPrivacyList();
		}
	}
	return FPrivacyLists.value(AStreamJid).value(AList);
}

QList<IPrivacyList> PrivacyLists::privacyLists(const Jid &AStreamJid, bool APending) const
{
	if (APending)
	{
		QList<IPrivacyList> lists;
		QList<QString> listNames = FPrivacyLists.value(AStreamJid).keys();
		foreach(QString listName, listNames)
		{
			IPrivacyList list = privacyList(AStreamJid,listName,APending);
			if (list.name == listName)
				lists.append(list);
		}
		foreach(QString id, FStreamRequests.value(AStreamJid))
		{
			if (FSaveRequests.contains(id) && !listNames.contains(FSaveRequests.value(id).name))
				lists.append(FSaveRequests.value(id));
		}
	}
	return FPrivacyLists.value(AStreamJid).values();
}

QString PrivacyLists::loadPrivacyList(const Jid &AStreamJid, const QString &AList)
{
	if (isReady(AStreamJid) && !AList.isEmpty())
	{
		Stanza load("iq");
		load.setType("get").setId(FStanzaProcessor->newId());
		QDomElement elem = load.addElement("query",NS_JABBER_PRIVACY);
		elem.appendChild(load.createElement("list")).toElement().setAttribute("name",AList);
		if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,load,PRIVACY_TIMEOUT))
		{
			FStreamRequests[AStreamJid].prepend(load.id());
			FLoadRequests.insert(load.id(),AList);
			return load.id();
		}
	}
	return QString::null;
}

QString PrivacyLists::savePrivacyList(const Jid &AStreamJid, const IPrivacyList &AList)
{
	if (isReady(AStreamJid) && !AList.name.isEmpty() && !AList.rules.isEmpty())
	{
		if (privacyList(AStreamJid,AList.name,true) != AList)
		{
			Stanza save("iq");
			save.setType("set").setId(FStanzaProcessor->newId());
			QDomElement queryElem = save.addElement("query",NS_JABBER_PRIVACY);
			QDomElement listElem =  queryElem.appendChild(save.createElement("list")).toElement();
			listElem.setAttribute("name",AList.name);

			foreach(IPrivacyRule item, AList.rules)
			{
				QDomElement itemElem = listElem.appendChild(save.createElement("item")).toElement();
				itemElem.setAttribute("order",item.order);
				itemElem.setAttribute("action",item.action);
				if (!item.type.isEmpty())
					itemElem.setAttribute("type",item.type);
				if (item.type!=PRIVACY_TYPE_ALWAYS && !item.value.isEmpty())
					itemElem.setAttribute("value",item.value);
				if (item.stanzas != IPrivacyRule::AnyStanza)
				{
					if (item.stanzas & IPrivacyRule::Messages)
						itemElem.appendChild(save.createElement("message"));
					if (item.stanzas & IPrivacyRule::Queries)
						itemElem.appendChild(save.createElement("iq"));
					if (item.stanzas & IPrivacyRule::PresencesIn)
						itemElem.appendChild(save.createElement("presence-in"));
					if (item.stanzas & IPrivacyRule::PresencesOut)
						itemElem.appendChild(save.createElement("presence-out"));
				}
			}

			emit listAboutToBeChanged(AStreamJid,AList);
			if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,save,PRIVACY_TIMEOUT))
			{
				FStreamRequests[AStreamJid].prepend(save.id());
				FSaveRequests.insert(save.id(),AList);
				return save.id();
			}
		}
		else
			return QString("");
	}
	return QString::null;
}

QString PrivacyLists::removePrivacyList(const Jid &AStreamJid, const QString &AList)
{
	if (isReady(AStreamJid) && !AList.isEmpty())
	{
		Stanza remove("iq");
		remove.setType("set").setId(FStanzaProcessor->newId());
		QDomElement queryElem = remove.addElement("query",NS_JABBER_PRIVACY);
		queryElem.appendChild(remove.createElement("list")).toElement().setAttribute("name",AList);
		if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,remove,PRIVACY_TIMEOUT))
		{
			FStreamRequests[AStreamJid].prepend(remove.id());
			FRemoveRequests.insert(remove.id(),AList);
			return remove.id();
		}
	}
	return QString::null;
}

QDialog *PrivacyLists::showEditListsDialog(const Jid &AStreamJid, QWidget *AParent)
{
	EditListsDialog *dialog = FEditListsDialogs.value(AStreamJid);
	if (isReady(AStreamJid))
	{
		if (!dialog)
		{
			dialog = new EditListsDialog(this,FRosterPlugin!= NULL ? FRosterPlugin->findRoster(AStreamJid) : NULL, AStreamJid,AParent);
			connect(dialog,SIGNAL(destroyed(const Jid &)),SLOT(onEditListsDialogDestroyed(const Jid &)));
			FEditListsDialogs.insert(AStreamJid,dialog);
		}
		dialog->show();
	}
	return dialog;
}

QString PrivacyLists::loadPrivacyLists(const Jid &AStreamJid)
{
	if (FStanzaProcessor)
	{
		Stanza load("iq");
		load.setType("get").setId(FStanzaProcessor->newId());
		load.addElement("query",NS_JABBER_PRIVACY);
		if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,load,PRIVACY_TIMEOUT))
		{
			FLoadRequests.insert(load.id(),QString::null);
			return load.id();
		}
	}
	return QString::null;
}

Menu *PrivacyLists::createPrivacyMenu(Menu *AMenu) const
{
	Menu *pmenu = new Menu(AMenu);
	pmenu->setTitle(tr("Privacy"));
	pmenu->setIcon(RSR_STORAGE_MENUICONS,MNI_PRIVACYLISTS);
	AMenu->addAction(pmenu->menuAction(),AG_RVCM_PRIVACYLISTS,true);
	return pmenu;
}

void PrivacyLists::createAutoPrivacyStreamActions(const QStringList &AStreams, Menu *AMenu) const
{
	if (!AStreams.isEmpty())
	{
		QStringList activeLists;
		bool isAllBlockOffRoster = true;
		foreach(const Jid &streamJid, AStreams)
		{
			QString listName = activeList(streamJid);
			if (!activeLists.contains(listName))
				activeLists.append(listName);
			isAllBlockOffRoster = isAllBlockOffRoster && isAutoPrivacy(streamJid) && isOffRosterBlocked(streamJid);
		}

		Action *visibleAction = new Action(AMenu);
		visibleAction->setText(tr("Visible Mode"));
		visibleAction->setData(ADR_STREAM_JID,AStreams);
		visibleAction->setData(ADR_LISTNAME,PRIVACY_LIST_AUTO_VISIBLE);
		visibleAction->setCheckable(true);
		visibleAction->setChecked(activeLists.count()==1 && activeLists.value(0)==PRIVACY_LIST_AUTO_VISIBLE);
		connect(visibleAction,SIGNAL(triggered(bool)),SLOT(onChangeStreamsAutoPrivacy(bool)));
		AMenu->addAction(visibleAction,AG_DEFAULT);

		Action *invisibleAction = new Action(AMenu);
		invisibleAction->setText(tr("Invisible Mode"));
		invisibleAction->setData(ADR_STREAM_JID,AStreams);
		invisibleAction->setData(ADR_LISTNAME,PRIVACY_LIST_AUTO_INVISIBLE);
		invisibleAction->setCheckable(true);
		invisibleAction->setChecked(activeLists.count()==1 && activeLists.value(0)==PRIVACY_LIST_AUTO_INVISIBLE);
		connect(invisibleAction,SIGNAL(triggered(bool)),SLOT(onChangeStreamsAutoPrivacy(bool)));
		AMenu->addAction(invisibleAction,AG_DEFAULT);

		Action *disableAction = new Action(AMenu);
		disableAction->setText(tr("Disable Privacy Lists"));
		disableAction->setData(ADR_STREAM_JID,AStreams);
		disableAction->setData(ADR_LISTNAME,QString());
		disableAction->setCheckable(true);
		disableAction->setChecked(activeLists.count()==1 && activeLists.value(0).isEmpty());
		connect(disableAction,SIGNAL(triggered(bool)),SLOT(onChangeStreamsAutoPrivacy(bool)));
		AMenu->addAction(disableAction,AG_DEFAULT);

		QActionGroup *modeGroup = new QActionGroup(AMenu);
		modeGroup->addAction(visibleAction);
		modeGroup->addAction(invisibleAction);
		modeGroup->addAction(disableAction);

		Action *blockAction = new Action(AMenu);
		blockAction->setText(tr("Block Contacts Without Subscription"));
		blockAction->setData(ADR_STREAM_JID,AStreams);
		blockAction->setData(ADR_LISTNAME,PRIVACY_LIST_SUBSCRIPTION);
		blockAction->setCheckable(true);
		blockAction->setChecked(isAllBlockOffRoster);
		connect(blockAction,SIGNAL(triggered(bool)),SLOT(onChangeStreamsOffRosterBlocked(bool)));
		AMenu->addAction(blockAction,AG_DEFAULT+100);
	}
}

void PrivacyLists::createAutoPrivacyContactActions(const QStringList &AStreams, const QStringList &AContacts, Menu *AMenu) const
{
	if (!AStreams.isEmpty() && AStreams.count()==AContacts.count())
	{
		int allListedMask = 0x01|0x02|0x04;
		for(int i=0; i<AStreams.count(); i++)
		{
			if (!isAutoPrivacy(AStreams.at(i)))
				allListedMask &= ~0x07;
			if (!isContactAutoListed(AStreams.at(i),AContacts.at(i),PRIVACY_LIST_VISIBLE))
				allListedMask &= ~0x01;
			if (!isContactAutoListed(AStreams.at(i),AContacts.at(i),PRIVACY_LIST_INVISIBLE))
				allListedMask &= ~0x02;
			if (!isContactAutoListed(AStreams.at(i),AContacts.at(i),PRIVACY_LIST_IGNORE))
				allListedMask &= ~0x04;
		}

		Action *defaultAction = new Action(AMenu);
		defaultAction->setText(tr("Default Rule"));
		defaultAction->setData(ADR_STREAM_JID,AStreams);
		defaultAction->setData(ADR_CONTACT_JID,AContacts);
		defaultAction->setCheckable(true);
		defaultAction->setChecked((allListedMask & 0x07)==0);
		connect(defaultAction,SIGNAL(triggered(bool)),SLOT(onChangeContactsAutoListed(bool)));
		AMenu->addAction(defaultAction,AG_DEFAULT);

		Action *visibleAction = new Action(AMenu);
		visibleAction->setText(tr("Visible to Contact"));
		visibleAction->setData(ADR_STREAM_JID,AStreams);
		visibleAction->setData(ADR_CONTACT_JID,AContacts);
		visibleAction->setData(ADR_LISTNAME,PRIVACY_LIST_VISIBLE);
		visibleAction->setCheckable(true);
		visibleAction->setChecked((allListedMask & 0x01)>0);
		connect(visibleAction,SIGNAL(triggered(bool)),SLOT(onChangeContactsAutoListed(bool)));
		AMenu->addAction(visibleAction,AG_DEFAULT);

		Action *invisibleAction = new Action(AMenu);
		invisibleAction->setText(tr("Invisible to Contact"));
		invisibleAction->setData(ADR_STREAM_JID,AStreams);
		invisibleAction->setData(ADR_CONTACT_JID,AContacts);
		invisibleAction->setData(ADR_LISTNAME,PRIVACY_LIST_INVISIBLE);
		invisibleAction->setCheckable(true);
		invisibleAction->setChecked((allListedMask & 0x02)>0);
		connect(invisibleAction,SIGNAL(triggered(bool)),SLOT(onChangeContactsAutoListed(bool)));
		AMenu->addAction(invisibleAction,AG_DEFAULT);

		Action *ignoreAction = new Action(AMenu);
		ignoreAction->setText(tr("Ignore Contact"));
		ignoreAction->setData(ADR_STREAM_JID,AStreams);
		ignoreAction->setData(ADR_CONTACT_JID,AContacts);
		ignoreAction->setData(ADR_LISTNAME,PRIVACY_LIST_IGNORE);
		ignoreAction->setCheckable(true);
		ignoreAction->setChecked((allListedMask & 0x04)>0);
		connect(ignoreAction,SIGNAL(triggered(bool)),SLOT(onChangeContactsAutoListed(bool)));
		AMenu->addAction(ignoreAction,AG_DEFAULT);

		QActionGroup *ruleGroup = new QActionGroup(AMenu);
		ruleGroup->addAction(defaultAction);
		ruleGroup->addAction(visibleAction);
		ruleGroup->addAction(invisibleAction);
		ruleGroup->addAction(ignoreAction);
	}
}

void PrivacyLists::createAutoPrivacyGroupActions(const QStringList &AStreams, const QStringList &AGroups, Menu *AMenu) const
{
	if (!AStreams.isEmpty() && AStreams.count()==AGroups.count())
	{
		int allListedMask = 0x01|0x02|0x04;
		for(int i=0; i<AStreams.count(); i++)
		{
			if (!isAutoPrivacy(AStreams.at(i)))
				allListedMask &= ~0x07;
			if (!isGroupAutoListed(AStreams.at(i),AGroups.at(i),PRIVACY_LIST_VISIBLE))
				allListedMask &= ~0x01;
			if (!isGroupAutoListed(AStreams.at(i),AGroups.at(i),PRIVACY_LIST_INVISIBLE))
				allListedMask &= ~0x02;
			if (!isGroupAutoListed(AStreams.at(i),AGroups.at(i),PRIVACY_LIST_IGNORE))
				allListedMask &= ~0x04;
		}

		Action *defaultAction = new Action(AMenu);
		defaultAction->setText(tr("Default Rule"));
		defaultAction->setData(ADR_STREAM_JID,AStreams);
		defaultAction->setData(ADR_GROUP_NAME,AGroups);
		defaultAction->setCheckable(true);
		defaultAction->setChecked((allListedMask & 0x07)==0);
		connect(defaultAction,SIGNAL(triggered(bool)),SLOT(onChangeGroupsAutoListed(bool)));
		AMenu->addAction(defaultAction,AG_DEFAULT);

		Action *visibleAction = new Action(AMenu);
		visibleAction->setText(tr("Visible to Group"));
		visibleAction->setData(ADR_STREAM_JID,AStreams);
		visibleAction->setData(ADR_GROUP_NAME,AGroups);
		visibleAction->setData(ADR_LISTNAME,PRIVACY_LIST_VISIBLE);
		visibleAction->setCheckable(true);
		visibleAction->setChecked((allListedMask & 0x01)>0);
		connect(visibleAction,SIGNAL(triggered(bool)),SLOT(onChangeGroupsAutoListed(bool)));
		AMenu->addAction(visibleAction,AG_DEFAULT);

		Action *invisibleAction = new Action(AMenu);
		invisibleAction->setText(tr("Invisible to Group"));
		invisibleAction->setData(ADR_STREAM_JID,AStreams);
		invisibleAction->setData(ADR_GROUP_NAME,AGroups);
		invisibleAction->setData(ADR_LISTNAME,PRIVACY_LIST_INVISIBLE);
		invisibleAction->setCheckable(true);
		invisibleAction->setChecked((allListedMask & 0x02)>0);
		connect(invisibleAction,SIGNAL(triggered(bool)),SLOT(onChangeGroupsAutoListed(bool)));
		AMenu->addAction(invisibleAction,AG_DEFAULT);

		Action *ignoreAction = new Action(AMenu);
		ignoreAction->setText(tr("Ignore Group"));
		ignoreAction->setData(ADR_STREAM_JID,AStreams);
		ignoreAction->setData(ADR_GROUP_NAME,AGroups);
		ignoreAction->setData(ADR_LISTNAME,PRIVACY_LIST_IGNORE);
		ignoreAction->setCheckable(true);
		ignoreAction->setChecked((allListedMask & 0x04)>0);
		connect(ignoreAction,SIGNAL(triggered(bool)),SLOT(onChangeGroupsAutoListed(bool)));
		AMenu->addAction(ignoreAction,AG_DEFAULT,false);

		QActionGroup *ruleGroup = new QActionGroup(AMenu);
		ruleGroup->addAction(defaultAction);
		ruleGroup->addAction(visibleAction);
		ruleGroup->addAction(invisibleAction);
		ruleGroup->addAction(ignoreAction);
	}
}

Menu *PrivacyLists::createSetActiveMenu(const Jid &AStreamJid, const QList<IPrivacyList> &ALists, Menu *AMenu) const
{
	QString listName = activeList(AStreamJid);

	Menu *activeMenu = new Menu(AMenu);
	activeMenu->setTitle(tr("Set Active List"));

	QActionGroup *listGroup = new QActionGroup(AMenu);

	Action *action = new Action(activeMenu);
	action->setData(ADR_STREAM_JID,AStreamJid.full());
	action->setData(ADR_LISTNAME,QString());
	action->setCheckable(true);
	action->setChecked(listName.isEmpty());
	action->setText(tr("<None>"));
	connect(action,SIGNAL(triggered(bool)),SLOT(onSetActiveListByAction(bool)));
	listGroup->addAction(action);
	activeMenu->addAction(action,AG_DEFAULT-100,false);

	foreach(IPrivacyList list, ALists)
	{
		action = new Action(activeMenu);
		action->setData(ADR_STREAM_JID,AStreamJid.full());
		action->setData(ADR_LISTNAME,list.name);
		action->setCheckable(true);
		action->setChecked(list.name == listName);
		action->setText(list.name);
		connect(action,SIGNAL(triggered(bool)),SLOT(onSetActiveListByAction(bool)));
		listGroup->addAction(action);
		activeMenu->addAction(action,AG_DEFAULT,true);
	}
	AMenu->addAction(activeMenu->menuAction(),AG_DEFAULT+200,false);

	return activeMenu;
}

Menu *PrivacyLists::createSetDefaultMenu(const Jid &AStreamJid, const QList<IPrivacyList> &ALists, Menu *AMenu) const
{
	QString listName = defaultList(AStreamJid);

	Menu *defaultMenu = new Menu(AMenu);
	defaultMenu->setTitle(tr("Set Default List"));

	QActionGroup *listGroup = new QActionGroup(AMenu);

	Action *action = new Action(defaultMenu);
	action->setData(ADR_STREAM_JID,AStreamJid.full());
	action->setData(ADR_LISTNAME,QString());
	action->setCheckable(true);
	action->setChecked(listName.isEmpty());
	action->setText(tr("<None>"));
	listGroup->addAction(action);
	connect(action,SIGNAL(triggered(bool)),SLOT(onSetDefaultListByAction(bool)));
	defaultMenu->addAction(action,AG_DEFAULT-100,false);

	foreach(IPrivacyList list, ALists)
	{
		action = new Action(defaultMenu);
		action->setData(ADR_STREAM_JID,AStreamJid.full());
		action->setData(ADR_LISTNAME,list.name);
		action->setCheckable(true);
		action->setChecked(list.name == listName);
		action->setText(list.name);
		connect(action,SIGNAL(triggered(bool)),SLOT(onSetDefaultListByAction(bool)));
		listGroup->addAction(action);
		defaultMenu->addAction(action,AG_DEFAULT,true);
	}
	AMenu->addAction(defaultMenu->menuAction(),AG_DEFAULT+200,false);

	return defaultMenu;
}

bool PrivacyLists::isMatchedJid(const Jid &AMask, const Jid &AJid) const
{
	return  ( (AMask.pDomain() == AJid.pDomain()) &&
	          (AMask.node().isEmpty() || AMask.pNode()==AJid.pNode()) &&
	          (AMask.resource().isEmpty() || AMask.resource()==AJid.resource()) );
}

void PrivacyLists::sendOnlinePresences(const Jid &AStreamJid, const IPrivacyList &AAutoList)
{
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AStreamJid) : NULL;
	IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->findPresence(AStreamJid) : NULL;
	if (presence)
	{
		QSet<Jid> denied = denyedContacts(AStreamJid,AAutoList,IPrivacyRule::PresencesOut).keys().toSet();
		QSet<Jid> online = FOfflinePresences.value(AStreamJid) - denied;
		if (presence->isOpen())
		{
			foreach(Jid contactJid, online)
			{
				IRosterItem ritem = roster!=NULL ? roster->rosterItem(contactJid) : IRosterItem();
				if (ritem.subscription==SUBSCRIPTION_BOTH || ritem.subscription==SUBSCRIPTION_FROM)
					presence->sendPresence(contactJid,presence->show(),presence->status(),presence->priority());
			}
			presence->setPresence(presence->show(),presence->status(),presence->priority());
		}
		FOfflinePresences[AStreamJid] -= online;
	}
}

void PrivacyLists::sendOfflinePresences(const Jid &AStreamJid, const IPrivacyList &AAutoList)
{
	IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->findPresence(AStreamJid) : NULL;
	if (presence)
	{
		QSet<Jid> denied = denyedContacts(AStreamJid,AAutoList,IPrivacyRule::PresencesOut).keys().toSet();
		QSet<Jid> offline = denied - FOfflinePresences.value(AStreamJid);
		if (presence->isOpen())
		{
			foreach(Jid contactJid, offline)
				presence->sendPresence(contactJid,IPresence::Offline,QString::null,0);
		}
		FOfflinePresences[AStreamJid] += offline;
	}
}

void PrivacyLists::setPrivacyLabel(const Jid &AStreamJid, const Jid &AContactJid, bool AVisible)
{
	if (FRostersModel)
	{
		QList<IRosterIndex *> indexList = FRostersModel->findContactIndexes(AStreamJid,AContactJid);
		foreach(IRosterIndex *index, indexList)
		{
			if (AVisible)
			{
				FLabeledContacts[AStreamJid]+=AContactJid;
				FRostersView->insertLabel(FPrivacyLabelId,index);
			}
			else
			{
				FLabeledContacts[AStreamJid]-=AContactJid;
				FRostersView->removeLabel(FPrivacyLabelId,index);
			}
		}
	}
}

void PrivacyLists::updatePrivacyLabels(const Jid &AStreamJid)
{
	if (FRostersModel)
	{
		QSet<Jid> denied = denyedContacts(AStreamJid,privacyList(AStreamJid,activeList(AStreamJid))).keys().toSet();
		QSet<Jid> deny = denied - FLabeledContacts.value(AStreamJid);
		QSet<Jid> allow = FLabeledContacts.value(AStreamJid) - denied;

		foreach(Jid contactJid, deny)
			setPrivacyLabel(AStreamJid,contactJid,true);

		foreach(Jid contactJid, allow)
			setPrivacyLabel(AStreamJid,contactJid,false);

		IRosterIndex *sroot = FRostersModel->streamRoot(AStreamJid);
		IRosterIndex *groupIndex = FRostersModel->findGroupIndex(RIK_GROUP_NOT_IN_ROSTER,QString::null,sroot);
		if (groupIndex)
		{
			for (int i=0;i<groupIndex->childCount();i++)
			{
				IRosterIndex *index = groupIndex->childIndex(i);
				if (index->kind() == RIK_CONTACT || index->kind()==RIK_AGENT)
				{
					IRosterItem ritem;
					ritem.itemJid = index->data(RDR_PREP_BARE_JID).toString();
					if ((denyedStanzas(ritem,privacyList(AStreamJid,activeList(AStreamJid))) & IPrivacyRule::AnyStanza)>0)
						FRostersView->insertLabel(FPrivacyLabelId,index);
					else
						FRostersView->removeLabel(FPrivacyLabelId,index);
				}
			}
		}
	}
}

bool PrivacyLists::isAllStreamsReady(const QStringList &AStreams) const
{
	foreach(const Jid &streamJid, AStreams)
		if (!isReady(streamJid))
			return false;
	return !AStreams.isEmpty();
}

bool PrivacyLists::isSelectionAccepted(const QList<IRosterIndex *> &ASelected) const
{
	static const QList<int> acceptKinds = QList<int>() << RIK_STREAM_ROOT << RIK_CONTACT << RIK_AGENT << RIK_GROUP;

	int singleKind = -1;
	foreach(IRosterIndex *index, ASelected)
	{
		int indexKind = index->kind();
		if (!acceptKinds.contains(indexKind))
			return false;
		else if (singleKind!=-1 && singleKind!=indexKind)
			return false;
		else if (indexKind==RIK_GROUP && !isAllStreamsReady(index->data(RDR_STREAMS).toStringList()))
			return false;
		else if (indexKind!=RIK_GROUP && !isAllStreamsReady(index->data(RDR_STREAM_JID).toStringList()))
			return false;
		singleKind = indexKind;
	}
	return !ASelected.isEmpty();
}

void PrivacyLists::onListAboutToBeChanged(const Jid &AStreamJid, const IPrivacyList &AList)
{
	if (AList.name == activeList(AStreamJid))
	{
		sendOfflinePresences(AStreamJid,AList);
	}
}

void PrivacyLists::onListChanged(const Jid &AStreamJid, const QString &AList)
{
	if (isAutoPrivacy(AStreamJid) && AutoLists.contains(AList))
	{
		FApplyAutoLists.insert(AStreamJid,activeList(AStreamJid));
		FApplyAutoListsTimer.start();
	}
	else if (AList == activeList(AStreamJid))
	{
		sendOnlinePresences(AStreamJid,privacyList(AStreamJid,AList));
		updatePrivacyLabels(AStreamJid);
	}
}

void PrivacyLists::onActiveListAboutToBeChanged(const Jid &AStreamJid, const QString &AList)
{
	sendOfflinePresences(AStreamJid,privacyList(AStreamJid,AList));
}

void PrivacyLists::onActiveListChanged(const Jid &AStreamJid, const QString &AList)
{
	sendOnlinePresences(AStreamJid,privacyList(AStreamJid,AList));
	updatePrivacyLabels(AStreamJid);
}

void PrivacyLists::onApplyAutoLists()
{
	QList<Jid> streamJids = FApplyAutoLists.keys();
	foreach(Jid streamJid, streamJids)
	{
		IPrivacyList list;
		list.name = FApplyAutoLists.value(streamJid);

		IPrivacyRule selfAllow;
		selfAllow.type = PRIVACY_TYPE_JID;
		selfAllow.value = streamJid.pBare();
		selfAllow.action = PRIVACY_ACTION_ALLOW;
		selfAllow.stanzas = IPrivacyRule::AnyStanza;
		list.rules.append(selfAllow);

		foreach(QString listName, AutoLists)
		{
			IPrivacyList autoList = privacyList(streamJid,listName,true);
			list.rules+=autoList.rules;
		}

		if (list.name == PRIVACY_LIST_AUTO_VISIBLE)
		{
			IPrivacyRule visible;
			visible.type = PRIVACY_TYPE_ALWAYS;
			visible.action = PRIVACY_ACTION_ALLOW;
			visible.stanzas = IPrivacyRule::AnyStanza;
			list.rules.append(visible);
		}
		else if (list.name == PRIVACY_LIST_AUTO_INVISIBLE)
		{
			IPrivacyRule invisible;
			invisible.type = PRIVACY_TYPE_ALWAYS;
			invisible.action = PRIVACY_ACTION_DENY;
			invisible.stanzas = IPrivacyRule::PresencesOut;
			list.rules.append(invisible);
		}

		for (int i=0;i<list.rules.count();i++)
			list.rules[i].order = i;

		savePrivacyList(streamJid,list);
	}
	FApplyAutoLists.clear();
}

void PrivacyLists::onStreamOpened(IXmppStream *AXmppStream)
{
	if (FStanzaProcessor)
	{
		IStanzaHandle shandle;
		shandle.handler = this;
		shandle.order = SHO_DEFAULT;
		shandle.direction = IStanzaHandle::DirectionIn;
		shandle.streamJid = AXmppStream->streamJid();

		shandle.conditions.append(SHC_PRIVACY);
		FSHIPrivacy.insert(shandle.streamJid,FStanzaProcessor->insertStanzaHandle(shandle));

		shandle.order = SHO_DEFAULT-1;
		shandle.conditions.clear();
		shandle.conditions.append(SHC_ROSTER);
		FSHIRosterIn.insert(shandle.streamJid,FStanzaProcessor->insertStanzaHandle(shandle));

		shandle.direction = IStanzaHandle::DirectionOut;
		FSHIRosterOut.insert(shandle.streamJid,FStanzaProcessor->insertStanzaHandle(shandle));

		loadPrivacyLists(AXmppStream->streamJid());
	}
}

void PrivacyLists::onStreamClosed(IXmppStream *AXmppStream)
{
	if (FEditListsDialogs.contains(AXmppStream->streamJid()))
		delete FEditListsDialogs.take(AXmppStream->streamJid());

	FApplyAutoLists.remove(AXmppStream->streamJid());
	FOfflinePresences.remove(AXmppStream->streamJid());
	FActiveLists.remove(AXmppStream->streamJid());
	FDefaultLists.remove(AXmppStream->streamJid());
	FPrivacyLists.remove(AXmppStream->streamJid());
	FStreamRequests.remove(AXmppStream->streamJid());

	updatePrivacyLabels(AXmppStream->streamJid());

	if (FStanzaProcessor)
	{
		FStanzaProcessor->removeStanzaHandle(FSHIPrivacy.take(AXmppStream->streamJid()));
		FStanzaProcessor->removeStanzaHandle(FSHIRosterIn.take(AXmppStream->streamJid()));
		FStanzaProcessor->removeStanzaHandle(FSHIRosterOut.take(AXmppStream->streamJid()));
	}
}

void PrivacyLists::onRosterIndexCreated(IRosterIndex *AIndex)
{
	if (FRostersView && (AIndex->kind()==RIK_CONTACT || AIndex->kind()==RIK_AGENT))
	{
		if (FNewRosterIndexes.isEmpty())
			QTimer::singleShot(0,this,SLOT(onUpdateNewRosterIndexes()));
		FNewRosterIndexes.append(AIndex);
	}
}

void PrivacyLists::onRostersViewIndexMultiSelection(const QList<IRosterIndex *> &ASelected, bool &AAccepted)
{
	AAccepted = AAccepted || isSelectionAccepted(ASelected);
}

void PrivacyLists::onRostersViewIndexContextMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu)
{
	bool isMultiSelection = AIndexes.count()>1;
	if (ALabelId==AdvancedDelegateItem::DisplayId && isSelectionAccepted(AIndexes))
	{
		int indexKind = AIndexes.first()->kind();
		if (indexKind == RIK_STREAM_ROOT)
		{
			QMap<int,QStringList> rolesMap = FRostersView->indexesRolesMap(AIndexes,QList<int>()<<RDR_STREAM_JID,RDR_STREAM_JID);
			
			Menu *pmenu = createPrivacyMenu(AMenu);
			createAutoPrivacyStreamActions(rolesMap.value(RDR_STREAM_JID),pmenu);

			if (!isMultiSelection)
			{
				Jid streamJid = AIndexes.first()->data(RDR_STREAM_JID).toString();
				if (!isAutoPrivacy(streamJid))
				{
					QList<IPrivacyList> lists = privacyLists(streamJid);
					for (int i=0; i<lists.count(); i++)
						if (AutoLists.contains(lists.at(i).name))
							lists.removeAt(i--);

					if (!lists.isEmpty())
					{
						createSetActiveMenu(streamJid,lists,pmenu);
						createSetDefaultMenu(streamJid,lists,pmenu);
					}
				}

				Action *action = new Action(AMenu);
				action->setText(tr("Advanced..."));
				action->setIcon(RSR_STORAGE_MENUICONS,MNI_PRIVACYLISTS_ADVANCED);
				action->setData(ADR_STREAM_JID,streamJid.full());
				connect(action,SIGNAL(triggered(bool)),SLOT(onShowEditListsDialog(bool)));
				pmenu->addAction(action,AG_DEFAULT+400,false);
			}
		}
		else
		{
			QStringList streams;
			QStringList contacts;
			QStringList groups;
			foreach(IRosterIndex *index, AIndexes)
			{
				if (indexKind != RIK_GROUP)
				{
					QString streamJid = index->data(RDR_STREAM_JID).toString();
					streams.append(streamJid);
					contacts.append(index->data(RDR_PREP_BARE_JID).toString());
				}
				else foreach(const QString &streamJid, index->data(RDR_STREAMS).toStringList())
				{
					streams.append(streamJid);
					groups.append(index->data(RDR_GROUP).toString());
				}
			}

			Menu *pmenu = createPrivacyMenu(AMenu);
			if (indexKind == RIK_GROUP)
				createAutoPrivacyGroupActions(streams,groups,pmenu);
			else
				createAutoPrivacyContactActions(streams,contacts,pmenu);
		}
	}
}

void PrivacyLists::onRostersViewIndexToolTips(IRosterIndex *AIndex, quint32 ALabelId, QMap<int,QString> &AToolTips)
{
	if (ALabelId == FPrivacyLabelId)
	{
		Jid streamJid = AIndex->data(RDR_STREAM_JID).toString();
		Jid contactJid = AIndex->data(RDR_PREP_BARE_JID).toString();
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(streamJid) : NULL;
		IRosterItem ritem = roster!=NULL ? roster->rosterItem(contactJid) : IRosterItem();
		ritem.itemJid = contactJid;
		int stanzas = denyedStanzas(ritem,privacyList(streamJid,activeList(streamJid)));
		QString toolTip = tr("<b>Privacy settings:</b>") +"<br>";
		toolTip += tr("- queries: %1").arg((stanzas & IPrivacyRule::Queries) >0             ? tr("<b>denied</b>") : tr("allowed")) + "<br>";
		toolTip += tr("- messages: %1").arg((stanzas & IPrivacyRule::Messages) >0           ? tr("<b>denied</b>") : tr("allowed")) + "<br>";
		toolTip += tr("- presences in: %1").arg((stanzas & IPrivacyRule::PresencesIn) >0    ? tr("<b>denied</b>") : tr("allowed")) + "<br>";
		toolTip += tr("- presences out: %1").arg((stanzas & IPrivacyRule::PresencesOut) >0  ? tr("<b>denied</b>") : tr("allowed"));
		AToolTips.insert(RTTO_PRIVACYLISTS,toolTip);
	}
}

void PrivacyLists::onUpdateNewRosterIndexes()
{
	while (!FNewRosterIndexes.isEmpty())
	{
		IRosterIndex *index = FNewRosterIndexes.takeFirst();
		Jid streamJid = index->data(RDR_STREAM_JID).toString();
		if (!activeList(streamJid).isEmpty())
		{
			Jid contactJid = index->data(RDR_FULL_JID).toString();
			IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(streamJid) : NULL;
			IRosterItem ritem = roster!=NULL ? roster->rosterItem(contactJid) : IRosterItem();
			ritem.itemJid = contactJid;
			if ((denyedStanzas(ritem,privacyList(streamJid,activeList(streamJid))) & IPrivacyRule::AnyStanza)>0)
			{
				if (ritem.isValid)
					FLabeledContacts[streamJid]+=ritem.itemJid;
				FRostersView->insertLabel(FPrivacyLabelId,index);
			}
		}
	}
	FNewRosterIndexes.clear();
}

void PrivacyLists::onShowEditListsDialog(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		Jid streamJid = action->data(ADR_STREAM_JID).toString();
		showEditListsDialog(streamJid);
	}
}

void PrivacyLists::onSetActiveListByAction(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		Jid streamJid = action->data(ADR_STREAM_JID).toString();
		QString alist = action->data(ADR_LISTNAME).toString();
		if (alist != activeList(streamJid))
			setActiveList(streamJid,alist);
	}
}

void PrivacyLists::onSetDefaultListByAction(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		Jid streamJid = action->data(ADR_STREAM_JID).toString();
		QString dlist = action->data(ADR_LISTNAME).toString();
		if (dlist != defaultList(streamJid))
			setDefaultList(streamJid,dlist);
	}
}

void PrivacyLists::onChangeStreamsAutoPrivacy(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		foreach(Jid streamJid, action->data(ADR_STREAM_JID).toStringList())
			setAutoPrivacy(streamJid,action->data(ADR_LISTNAME).toString());
	}
}

void PrivacyLists::onChangeContactsAutoListed(bool APresent)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QString listName = action->data(ADR_LISTNAME).toString();
		QStringList streams = action->data(ADR_STREAM_JID).toStringList();
		QStringList contacts = action->data(ADR_CONTACT_JID).toStringList();
		for (int i=0; i<streams.count(); i++)
		{
			if (!listName.isEmpty())
			{
				if (!isAutoPrivacy(streams.at(i)))
					setAutoPrivacy(streams.at(i),PRIVACY_LIST_AUTO_VISIBLE);
				setContactAutoListed(streams.at(i),contacts.at(i),listName,APresent);
			}
			else
			{
				static const QStringList ContactAutoLists = QStringList() << PRIVACY_LIST_VISIBLE << PRIVACY_LIST_INVISIBLE << PRIVACY_LIST_IGNORE << PRIVACY_LIST_CONFERENCES;
				foreach(QString listName, ContactAutoLists)
					setContactAutoListed(streams.at(i),contacts.at(i),listName,false);
			}
		}
	}
}

void PrivacyLists::onChangeGroupsAutoListed(bool APresent)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QString listName = action->data(ADR_LISTNAME).toString();
		QStringList streams = action->data(ADR_STREAM_JID).toStringList();
		QStringList groups = action->data(ADR_GROUP_NAME).toStringList();
		for (int i=0; i<streams.count(); i++)
		{
			if (!listName.isEmpty())
			{
				if (!isAutoPrivacy(streams.at(i)))
					setAutoPrivacy(streams.at(i),PRIVACY_LIST_AUTO_VISIBLE);
				setGroupAutoListed(streams.at(i),groups.at(i),listName,APresent);
			}
			else
			{
				static const QStringList GroupAutoLists = QStringList() << PRIVACY_LIST_VISIBLE << PRIVACY_LIST_INVISIBLE << PRIVACY_LIST_IGNORE;
				foreach(QString listName, GroupAutoLists)
					setGroupAutoListed(streams.at(i),groups.at(i),listName,false);
			}
		}
	}
}

void PrivacyLists::onChangeStreamsOffRosterBlocked(bool ABlocked)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		foreach(Jid streamJid, action->data(ADR_STREAM_JID).toStringList())
		{
			if (!isAutoPrivacy(streamJid))
				setAutoPrivacy(streamJid,PRIVACY_LIST_AUTO_VISIBLE);
			setOffRosterBlocked(streamJid,ABlocked);
		}
	}
}

void PrivacyLists::onEditListsDialogDestroyed(const Jid &AStreamJid)
{
	FEditListsDialogs.remove(AStreamJid);
}

void PrivacyLists::onMultiUserChatCreated(IMultiUserChat *AMultiChat)
{
	setContactAutoListed(AMultiChat->streamJid(),AMultiChat->roomJid(),PRIVACY_LIST_CONFERENCES,true);
}

Q_EXPORT_PLUGIN2(plg_privacylists, PrivacyLists)
