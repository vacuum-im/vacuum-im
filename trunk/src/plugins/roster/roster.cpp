#include "roster.h"

#include <QSet>
#include <QFile>

#define REQUEST_TIMEOUT       30000

#define SHC_ROSTER            "/iq[@type='set']/query[@xmlns='" NS_JABBER_ROSTER "']"
#define SHC_PRESENCE          "/presence[@type]"

#define NS_GROUP_DELIMITER    "roster:delimiter"

Roster::Roster(IXmppStream *AXmppStream, IStanzaProcessor *AStanzaProcessor) : QObject(AXmppStream->instance())
{
	FXmppStream = AXmppStream;
	FStanzaProcessor = AStanzaProcessor;

	FOpened = false;
	FVerSupported = false;
	setStanzaHandlers();

	connect(FXmppStream->instance(),SIGNAL(opened()),SLOT(onStreamOpened()));
	connect(FXmppStream->instance(),SIGNAL(closed()),SLOT(onStreamClosed()));
	connect(FXmppStream->instance(),SIGNAL(jidAboutToBeChanged(const Jid &)),SLOT(onStreamJidAboutToBeChanged(const Jid &)));
	connect(FXmppStream->instance(),SIGNAL(jidChanged(const Jid &)),SLOT(onStreamJidChanged(const Jid &)));
}

Roster::~Roster()
{
	clearItems();
	removeStanzaHandlers();
}

bool Roster::stanzaReadWrite(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept)
{
	if (AHandlerId == FSHIRosterPush)
	{
		if (isOpen() && (AStanza.from().isEmpty() || (AStreamJid && AStanza.from())))
		{
			AAccept = true;
			processItemsElement(AStanza.firstElement("query",NS_JABBER_ROSTER),false);

			Stanza result("iq");
			result.setType("result").setId(AStanza.id());
			FStanzaProcessor->sendStanzaOut(AStreamJid,result);
		}
	}
	else if (AHandlerId == FSHISubscription)
	{
		QString status = AStanza.firstElement("status").text();
		if (AStanza.type() == SUBSCRIPTION_SUBSCRIBE)
		{
			emit subscription(AStanza.from(),IRoster::Subscribe,status);
			AAccept = true;
		}
		else if (AStanza.type() == SUBSCRIPTION_SUBSCRIBED)
		{
			emit subscription(AStanza.from(),IRoster::Subscribed,status);
			AAccept = true;
		}
		else if (AStanza.type() == SUBSCRIPTION_UNSUBSCRIBE)
		{
			emit subscription(AStanza.from(),IRoster::Unsubscribe,status);
			AAccept = true;
		}
		else if (AStanza.type() == SUBSCRIPTION_UNSUBSCRIBED)
		{
			emit subscription(AStanza.from(),IRoster::Unsubscribed,status);
			AAccept = true;
		}
	}
	return false;
}

void Roster::stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza)
{
	if (AStanza.id() == FDelimRequestId)
	{
		QString groupDelim = "::";
		if (AStanza.type() == "result")
		{
			groupDelim = AStanza.firstElement("query",NS_JABBER_PRIVATE).firstChildElement("roster").text();
			if (groupDelim.isEmpty())
			{
				groupDelim = "::";
				Stanza delim("iq");
				delim.setType("set").setId(FStanzaProcessor->newId());
				QDomElement elem = delim.addElement("query",NS_JABBER_PRIVATE);
				elem.appendChild(delim.createElement("roster",NS_GROUP_DELIMITER)).appendChild(delim.createTextNode(groupDelim));
				FStanzaProcessor->sendStanzaOut(AStreamJid,delim);
			}
		}
		setGroupDelimiter(groupDelim);
		requestRosterItems();
	}
	else if (AStanza.id() == FOpenRequestId)
	{
		if (AStanza.type() == "result")
		{
			processItemsElement(AStanza.firstElement("query",NS_JABBER_ROSTER),true);
			FOpened = true;
			emit opened();
		}
		else
			FXmppStream->abort(tr("Roster request failed"));
	}
}

void Roster::stanzaRequestTimeout(const Jid &AStreamJid, const QString &AStanzaId)
{
	Q_UNUSED(AStreamJid);
	if (AStanzaId==FDelimRequestId || AStanzaId == FOpenRequestId)
		FXmppStream->abort(tr("Roster request failed"));
}

bool Roster::xmppStanzaIn(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder)
{
	if (!isOpen() && FXmppStream==AXmppStream && AOrder==XSHO_XMPP_FEATURE)
	{
		if (AStanza.element().nodeName()=="stream:features" && !AStanza.firstElement("ver",NS_FEATURE_ROSTER_VER).isNull())
			FVerSupported = true;
	}
	return false;
}

bool Roster::xmppStanzaOut(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder)
{
	Q_UNUSED(AXmppStream);
	Q_UNUSED(AStanza);
	Q_UNUSED(AOrder);
	return false;
}

IRosterItem Roster::rosterItem(const Jid &AItemJid) const
{
	foreach(IRosterItem ritem, FRosterItems)
		if (AItemJid && ritem.itemJid)
			return ritem;
	return IRosterItem();
}

QList<IRosterItem> Roster::rosterItems() const
{
	return FRosterItems.values();
}

QSet<QString> Roster::groups() const
{
	QSet<QString> allGroups;
	foreach(IRosterItem ritem, FRosterItems)
		if (!ritem.itemJid.node().isEmpty())
			allGroups += ritem.groups;
	return allGroups;
}

QList<IRosterItem> Roster::groupItems(const QString &AGroup) const
{
	QString groupWithDelim = AGroup+FGroupDelim;
	QList<IRosterItem> ritems;
	foreach(IRosterItem ritem, FRosterItems)
	{
		QSet<QString> allItemGroups = ritem.groups;
		foreach(QString itemGroup,allItemGroups)
		{
			if (itemGroup==AGroup || itemGroup.startsWith(groupWithDelim))
			{
				ritems.append(ritem);
				break;
			}
		}
	}
	return ritems;
}

QSet<QString> Roster::itemGroups(const Jid &AItemJid) const
{
	return rosterItem(AItemJid).groups;
}

void Roster::setItem(const Jid &AItemJid, const QString &AName, const QSet<QString> &AGroups)
{
	Stanza query("iq");
	query.setType("set").setId(FStanzaProcessor->newId());
	QDomElement itemElem = query.addElement("query",NS_JABBER_ROSTER).appendChild(query.createElement("item")).toElement();
	itemElem.setAttribute("jid", AItemJid.eBare());
	if (!AName.isEmpty())
		itemElem.setAttribute("name",AName);
	foreach (QString groupName,AGroups)
		if (!groupName.isEmpty())
			itemElem.appendChild(query.createElement("group")).appendChild(query.createTextNode(groupName));
	FStanzaProcessor->sendStanzaOut(FXmppStream->streamJid(),query);
}

void Roster::setItems(const QList<IRosterItem> &AItems)
{
	if (!AItems.isEmpty())
	{
		Stanza query("iq");
		query.setType("set").setId(FStanzaProcessor->newId());
		QDomElement elem = query.addElement("query",NS_JABBER_ROSTER);
		foreach(IRosterItem ritem, AItems)
		{
			QDomElement itemElem = elem.appendChild(query.createElement("item")).toElement();
			itemElem.setAttribute("jid", ritem.itemJid.eBare());
			if (!ritem.name.isEmpty())
				itemElem.setAttribute("name",ritem.name);
			foreach (QString groupName,ritem.groups)
				if (!groupName.isEmpty())
					itemElem.appendChild(query.createElement("group")).appendChild(query.createTextNode(groupName));
		}
		FStanzaProcessor->sendStanzaOut(FXmppStream->streamJid(),query);
	}
}

void Roster::sendSubscription(const Jid &AItemJid, int ASubsType, const QString &AText)
{
	QString type;
	if (ASubsType == IRoster::Subscribe)
		type = SUBSCRIPTION_SUBSCRIBE;
	else if (ASubsType == IRoster::Subscribed)
		type = SUBSCRIPTION_SUBSCRIBED;
	else if (ASubsType == IRoster::Unsubscribe)
		type = SUBSCRIPTION_UNSUBSCRIBE;
	else if (ASubsType == IRoster::Unsubscribed)
		type = SUBSCRIPTION_UNSUBSCRIBED;
	else return;

	Stanza subscr("presence");
	subscr.setTo(AItemJid.eBare()).setType(type);
	if (!AText.isEmpty())
		subscr.addElement("status").appendChild(subscr.createTextNode(AText));
	FStanzaProcessor->sendStanzaOut(FXmppStream->streamJid(),subscr);
}

void Roster::removeItem(const Jid &AItemJid)
{
	Stanza query("iq");
	query.setType("set").setId(FStanzaProcessor->newId());
	QDomElement itemElem = query.addElement("query",NS_JABBER_ROSTER).appendChild(query.createElement("item")).toElement();
	itemElem.setAttribute("jid", AItemJid.eBare());
	itemElem.setAttribute("subscription",SUBSCRIPTION_REMOVE);
	FStanzaProcessor->sendStanzaOut(FXmppStream->streamJid(),query);
}

void Roster::removeItems(const QList<IRosterItem> &AItems)
{
	if (!AItems.isEmpty())
	{
		Stanza query("iq");
		query.setType("set").setId(FStanzaProcessor->newId());
		QDomElement elem = query.addElement("query",NS_JABBER_ROSTER);
		foreach(IRosterItem ritem, AItems)
		{
			QDomElement itemElem = elem.appendChild(query.createElement("item")).toElement();
			itemElem.setAttribute("jid", ritem.itemJid.eBare());
			itemElem.setAttribute("subscription",SUBSCRIPTION_REMOVE);
		}
		FStanzaProcessor->sendStanzaOut(FXmppStream->streamJid(),query);
	}
}

void Roster::saveRosterItems(const QString &AFileName) const
{
	QDomDocument xml;
	QDomElement elem = xml.appendChild(xml.createElement("roster")).toElement();
	elem.setAttribute("ver",FRosterVer);
	elem.setAttribute("streamJid",streamJid().pBare());
	elem.setAttribute("groupDelimiter",FGroupDelim);
	foreach(IRosterItem ritem, FRosterItems)
	{
		QDomElement itemElem = elem.appendChild(xml.createElement("item")).toElement();
		itemElem.setAttribute("jid",ritem.itemJid.eBare());
		itemElem.setAttribute("name",ritem.name);
		itemElem.setAttribute("subscription",ritem.subscription);
		itemElem.setAttribute("ask",ritem.ask);
		foreach(QString group, ritem.groups)
			itemElem.appendChild(xml.createElement("group")).appendChild(xml.createTextNode(group));
	}

	QFile rosterFile(AFileName);
	if (rosterFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
	{
		rosterFile.write(xml.toByteArray());
		rosterFile.close();
	}
}

void Roster::loadRosterItems(const QString &AFileName)
{
	if (!isOpen())
	{
		QFile rosterFile(AFileName);
		if (rosterFile.exists() && rosterFile.open(QIODevice::ReadOnly))
		{
			QDomDocument xml;
			if (xml.setContent(rosterFile.readAll()))
			{
				QDomElement itemsElem = xml.firstChildElement("roster");
				if (!itemsElem.isNull() && itemsElem.attribute("streamJid")==streamJid().pBare())
				{
					setGroupDelimiter(itemsElem.attribute("groupDelimiter"));
					processItemsElement(itemsElem,true);
				}
			}
			rosterFile.close();
		}
	}
}

void Roster::renameItem(const Jid &AItemJid, const QString &AName)
{
	IRosterItem ritem = rosterItem(AItemJid);
	if (ritem.isValid && ritem.name!=AName)
		setItem(AItemJid,AName,ritem.groups);
}

void Roster::copyItemToGroup(const Jid &AItemJid, const QString &AGroup)
{
	IRosterItem ritem = rosterItem(AItemJid);
	if (ritem.isValid && !AGroup.isEmpty() && !ritem.groups.contains(AGroup))
	{
		QSet<QString> allItemGroups = ritem.groups;
		setItem(AItemJid,ritem.name,allItemGroups += AGroup);
	}
}

void Roster::moveItemToGroup(const Jid &AItemJid, const QString &AGroupFrom, const QString &AGroupTo)
{
	IRosterItem ritem = rosterItem(AItemJid);
	if (ritem.isValid && !ritem.groups.contains(AGroupTo))
	{
		QSet<QString> allItemGroups = ritem.groups;
		if (!AGroupTo.isEmpty())
		{
			allItemGroups += AGroupTo;
			allItemGroups -= AGroupFrom;
		}
		else
		{
			allItemGroups.clear();
		}
		setItem(AItemJid,ritem.name,allItemGroups);
	}
}

void Roster::removeItemFromGroup(const Jid &AItemJid, const QString &AGroup)
{
	IRosterItem ritem = rosterItem(AItemJid);
	if (ritem.isValid && ritem.groups.contains(AGroup))
	{
		QSet<QString> allItemGroups = ritem.groups;
		setItem(AItemJid,ritem.name,allItemGroups -= AGroup);
	}
}

void Roster::renameGroup(const QString &AGroup, const QString &AGroupTo)
{
	QList<IRosterItem> allGroupItems = groupItems(AGroup);
	QList<IRosterItem>::iterator it = allGroupItems.begin();
	while (it != allGroupItems.end())
	{
		QSet<QString> newItemGroups;
		foreach(QString group,it->groups)
		{
			QString newGroup = group;
			if (newGroup.startsWith(AGroup))
			{
				newGroup.remove(0,AGroup.size());
				newGroup.prepend(AGroupTo);
			}
			newItemGroups += newGroup;
		}
		it->groups = newItemGroups;
		it++;
	}
	setItems(allGroupItems);
}

void Roster::copyGroupToGroup(const QString &AGroup, const QString &AGroupTo)
{
	if (AGroup!=AGroupTo && !AGroup.isEmpty())
	{
		QList<IRosterItem> allGroupItems = groupItems(AGroup);
		QString groupName = AGroup.split(FGroupDelim,QString::SkipEmptyParts).last();
		QList<IRosterItem>::iterator it = allGroupItems.begin();
		while (it != allGroupItems.end())
		{
			QSet<QString> newItemGroups;
			QSet<QString> oldItemGroups = it->groups;
			foreach(QString group,oldItemGroups)
			{
				if (group.startsWith(AGroup))
				{
					QString newGroup = group;
					newGroup.remove(0,AGroup.size());
					if (!AGroupTo.isEmpty())
						newGroup.prepend(AGroupTo + FGroupDelim + groupName);
					else
						newGroup.prepend(groupName);

					newItemGroups += newGroup;
				}
			}
			it->groups += newItemGroups;
			it++;
		}
		setItems(allGroupItems);
	}
}

void Roster::moveGroupToGroup(const QString &AGroup, const QString &AGroupTo)
{
	if (AGroup != AGroupTo)
	{
		QList<IRosterItem> allGroupItems = groupItems(AGroup);
		QString groupName = AGroup.split(FGroupDelim,QString::SkipEmptyParts).last();
		QList<IRosterItem>::iterator it = allGroupItems.begin();
		while (it != allGroupItems.end())
		{
			QSet<QString> newItemGroups;
			QSet<QString> oldItemGroups = it->groups;
			foreach(QString group,oldItemGroups)
			{
				if (group.startsWith(AGroup))
				{
					QString newGroup = group;
					newGroup.remove(0,AGroup.size());
					if (!AGroupTo.isEmpty())
						newGroup.prepend(AGroupTo + FGroupDelim + groupName);
					else
						newGroup.prepend(groupName);
					newItemGroups += newGroup;
					oldItemGroups -= group;
				}
			}
			it->groups = oldItemGroups+newItemGroups;
			it++;
		}
		setItems(allGroupItems);
	}
}

void Roster::removeGroup(const QString &AGroup)
{
	QList<IRosterItem> allGroupItems = groupItems(AGroup);
	QList<IRosterItem>::iterator it = allGroupItems.begin();
	while (it != allGroupItems.end())
	{
		QSet<QString> newItemGroups = it->groups;
		foreach(QString group, it->groups)
			if (group.startsWith(AGroup))
				newItemGroups -= group;
		it->groups = newItemGroups;
		it++;
	}
	setItems(allGroupItems);
}

void Roster::processItemsElement(const QDomElement &AItemsElem, bool ACompleteRoster)
{
	if (!AItemsElem.isNull())
	{
		FRosterVer = AItemsElem.attribute("ver");
		QSet<Jid> oldItems = ACompleteRoster ? FRosterItems.keys().toSet() : QSet<Jid>();
		QDomElement itemElem = AItemsElem.firstChildElement("item");
		while (!itemElem.isNull())
		{
			Jid itemJid = itemElem.attribute("jid");
			if (itemJid.isValid() && itemJid.resource().isEmpty())
			{
				QString subs = itemElem.attribute("subscription");
				if (subs==SUBSCRIPTION_BOTH || subs==SUBSCRIPTION_TO || subs==SUBSCRIPTION_FROM || subs==SUBSCRIPTION_NONE)
				{
					IRosterItem &ritem = FRosterItems[itemJid];
					ritem.isValid = true;
					ritem.itemJid = itemJid;
					ritem.name = itemElem.attribute("name");
					ritem.subscription = subs;
					ritem.ask = itemElem.attribute("ask");
					oldItems -= ritem.itemJid;

					QSet<QString> allItemGroups;
					QDomElement groupElem = itemElem.firstChildElement("group");
					while (!groupElem.isNull())
					{
						allItemGroups += groupElem.text();
						groupElem = groupElem.nextSiblingElement("group");
					}
					ritem.groups = allItemGroups;

					emit received(ritem);
				}
				else if (subs == SUBSCRIPTION_REMOVE)
				{
					removeRosterItem(itemJid);
				}
			}
			itemElem = itemElem.nextSiblingElement("item");
		}

		foreach(Jid itemJid,oldItems)
			removeRosterItem(itemJid);
	}
}

void Roster::removeRosterItem(const Jid &AItemJid)
{
	if (FRosterItems.contains(AItemJid))
	{
		IRosterItem ritem = FRosterItems.take(AItemJid);
		emit removed(ritem);
	}
}

void Roster::clearItems()
{
	foreach(Jid itemJid,FRosterItems.keys())
		removeRosterItem(itemJid);
	FRosterVer.clear();
}

void Roster::requestGroupDelimiter()
{
	Stanza query("iq");
	query.setType("get").setId(FStanzaProcessor->newId());
	query.addElement("query",NS_JABBER_PRIVATE).appendChild(query.createElement("roster",NS_GROUP_DELIMITER));
	if (FStanzaProcessor->sendStanzaRequest(this,FXmppStream->streamJid(),query,REQUEST_TIMEOUT))
		FDelimRequestId = query.id();
}

void Roster::setGroupDelimiter(const QString &ADelimiter)
{
	if (FGroupDelim != ADelimiter)
		clearItems();
	FGroupDelim = ADelimiter;
}

void Roster::requestRosterItems()
{
	Stanza query("iq");
	query.setType("get").setId(FStanzaProcessor->newId());

	if (!FVerSupported)
		query.addElement("query",NS_JABBER_ROSTER);
	else
		query.addElement("query",NS_JABBER_ROSTER).setAttribute("ver",FRosterVer);

	if (FStanzaProcessor->sendStanzaRequest(this,FXmppStream->streamJid(),query,REQUEST_TIMEOUT))
		FOpenRequestId = query.id();
}

void Roster::setStanzaHandlers()
{
	IStanzaHandle shandle;
	shandle.handler = this;
	shandle.order = SHO_DEFAULT;
	shandle.direction = IStanzaHandle::DirectionIn;
	shandle.streamJid = FXmppStream->streamJid();

	shandle.conditions.append(SHC_ROSTER);
	FSHIRosterPush = FStanzaProcessor->insertStanzaHandle(shandle);

	shandle.conditions.clear();
	shandle.conditions.append(SHC_PRESENCE);
	FSHISubscription = FStanzaProcessor->insertStanzaHandle(shandle);

	FXmppStream->insertXmppStanzaHandler(this,XSHO_XMPP_FEATURE);
}

void Roster::removeStanzaHandlers()
{
	FStanzaProcessor->removeStanzaHandle(FSHIRosterPush);
	FStanzaProcessor->removeStanzaHandle(FSHISubscription);
	FXmppStream->removeXmppStanzaHandler(this, XSHO_XMPP_FEATURE);
}

void Roster::onStreamOpened()
{
	FXmppStream->removeXmppStanzaHandler(this, XSHO_XMPP_FEATURE);
	requestGroupDelimiter();
}

void Roster::onStreamClosed()
{
	if (isOpen())
	{
		FOpened = false;
		emit closed();
	}
	FVerSupported = false;
	FXmppStream->insertXmppStanzaHandler(this, XSHO_XMPP_FEATURE);
}

void Roster::onStreamJidAboutToBeChanged(const Jid &AAfter)
{
	emit streamJidAboutToBeChanged(AAfter);
	if (!(FXmppStream->streamJid() && AAfter))
		clearItems();
}

void Roster::onStreamJidChanged(const Jid &ABefore)
{
	emit streamJidChanged(ABefore);
}
