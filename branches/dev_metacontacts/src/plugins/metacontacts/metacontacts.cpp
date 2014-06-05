#include "metacontacts.h"

#include <QDir>
#include <definitions/namespaces.h>
#include <definitions/actiongroups.h>
#include <definitions/rosterindexkinds.h>
#include <definitions/rosterindexroles.h>
#include <definitions/rosterindexkindorders.h>
#include <utils/widgetmanager.h>
#include <utils/logger.h>

#define STORAGE_SAVE_TIMEOUT    100

#define DIR_METACONTACTS        "metacontacts"

#define ADR_STREAM_JID          Action::DR_StreamJid
#define ADR_CONTACT_JID         Action::DR_Parametr1
#define ADR_METACONTACT_ID      Action::DR_Parametr2

static const IMetaContact NullMetaContact = IMetaContact();

MetaContacts::MetaContacts()
{
	FPluginManager = NULL;
	FPrivateStorage = NULL;
	FRosterPlugin = NULL;
	FPresencePlugin = NULL;
	FRostersModel = NULL;
	FRostersView = NULL;
	FRostersViewPlugin = NULL;

	FSaveTimer.setSingleShot(true);
	connect(&FSaveTimer,SIGNAL(timeout()),SLOT(onSaveContactsToStorageTimerTimeout()));
}

MetaContacts::~MetaContacts()
{

}

void MetaContacts::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Meta-Contacts");
	APluginInfo->description = tr("Allows to combine single contacts to meta-contacts");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(ROSTER_UUID);
	APluginInfo->dependences.append(PRIVATESTORAGE_UUID);
}

bool MetaContacts::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
	FPluginManager = APluginManager;

	IPlugin *plugin = APluginManager->pluginInterface("IPrivateStorage").value(0,NULL);
	if (plugin)
	{
		FPrivateStorage = qobject_cast<IPrivateStorage *>(plugin->instance());
		if (FPrivateStorage)
		{
			connect(FPrivateStorage->instance(),SIGNAL(storageOpened(const Jid &)),SLOT(onPrivateStorageOpened(const Jid &)));
			connect(FPrivateStorage->instance(),SIGNAL(dataLoaded(const QString &, const Jid &, const QDomElement &)),
				SLOT(onPrivateStorageDataLoaded(const QString &, const Jid &, const QDomElement &)));
			connect(FPrivateStorage->instance(),SIGNAL(dataChanged(const Jid &, const QString &, const QString &)),
				SLOT(onPrivateStorageDataChanged(const Jid &, const QString &, const QString &)));
			connect(FPrivateStorage->instance(),SIGNAL(storageNotifyAboutToClose(const Jid &)),SLOT(onPrivateStorageNotifyAboutToClose(const Jid &)));
		}
	}

	plugin = APluginManager->pluginInterface("IRosterPlugin").value(0,NULL);
	if (plugin)
	{
		FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
		if (FRosterPlugin)
		{
			connect(FRosterPlugin->instance(),SIGNAL(rosterAdded(IRoster *)),SLOT(onRosterAdded(IRoster *)));
			connect(FRosterPlugin->instance(),SIGNAL(rosterRemoved(IRoster *)),SLOT(onRosterRemoved(IRoster *)));
			connect(FRosterPlugin->instance(),SIGNAL(rosterStreamJidChanged(IRoster *, const Jid &)),SLOT(onRosterStreamJidChanged(IRoster *, const Jid &)));
		}
	}

	plugin = APluginManager->pluginInterface("IPresencePlugin").value(0,NULL);
	if (plugin)
	{
		FPresencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IRostersModel").value(0,NULL);
	if (plugin)
	{
		FRostersModel = qobject_cast<IRostersModel *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
	if (plugin)
	{
		FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
		if (FRostersViewPlugin)
		{
			FRostersView = FRostersViewPlugin->rostersView();
			connect(FRostersView->instance(),SIGNAL(indexMultiSelection(const QList<IRosterIndex *> &, bool &)), 
				SLOT(onRostersViewIndexMultiSelection(const QList<IRosterIndex *> &, bool &)));
			connect(FRostersView->instance(), SIGNAL(indexContextMenu(const QList<IRosterIndex *> &, quint32 , Menu *)),
				SLOT(onRostersViewIndexContextMenu(const QList<IRosterIndex *> &, quint32 , Menu *)));
		}
	}

	return FRosterPlugin!=NULL && FPrivateStorage!=NULL;
}

bool MetaContacts::initObjects()
{
	return true;
}

bool MetaContacts::initSettings()
{
	return true;
}

bool MetaContacts::startPlugin()
{
	return true;
}

bool MetaContacts::isReady(const Jid &AStreamJid) const
{
	return FPrivateStorage==NULL || FPrivateStorage->isLoaded(AStreamJid,"storage",NS_STORAGE_METACONTACTS);
}

IMetaContact MetaContacts::findMetaContact(const Jid &AStreamJid, const Jid &AItem) const
{
	QUuid metaId = FItemMetaContact.value(AStreamJid).value(AItem.bare());
	return !metaId.isNull() ? FMetaContacts.value(AStreamJid).value(metaId) : NullMetaContact;
}

IMetaContact MetaContacts::findMetaContact(const Jid &AStreamJid, const QUuid &AMetaId) const
{
	return FMetaContacts.value(AStreamJid).value(AMetaId,NullMetaContact);
}

QList<IRosterIndex *> MetaContacts::findMetaIndexes(const Jid &AStreamJid, const QUuid &AMetaId) const
{
	return FMetaIndexes.value(AStreamJid).value(AMetaId);
}

QUuid MetaContacts::createMetaContact(const Jid &AStreamJid, const QList<Jid> &AItems, const QString &AName)
{
	if (isReady(AStreamJid))
	{
		IMetaContact meta;
		meta.id = QUuid::createUuid();
		meta.name = AName;
		meta.items = AItems;
		if (updateMetaContact(AStreamJid,meta))
		{
			startSaveContactsToStorage(AStreamJid);
			return meta.id;
		}
	}
	return QUuid();
}

QUuid MetaContacts::mergeMetaContacts(const Jid &AStreamJid, const QUuid &AMetaId1, const QUuid &AMetaId2)
{
	if (isReady(AStreamJid))
	{
		IMetaContact meta1 = findMetaContact(AStreamJid,AMetaId1);
		IMetaContact meta2 = findMetaContact(AStreamJid,AMetaId2);
		if (meta1.id==AMetaId1 && meta2.id==AMetaId2)
		{
			meta1.items += meta2.items;
			if (updateMetaContact(AStreamJid,meta1))
			{
				startSaveContactsToStorage(AStreamJid);
				return meta1.id;
			}
		}
	}
	return QUuid();
}

bool MetaContacts::detachMetaContactItems(const Jid &AStreamJid, const QUuid &AMetaId, const QList<Jid> &AItems)
{
	if (isReady(AStreamJid))
	{
		IMetaContact meta = findMetaContact(AStreamJid,AMetaId);
		if (meta.id == AMetaId)
		{
			foreach(const Jid &item, AItems)
				meta.items.removeAll(item);
			updateMetaContact(AStreamJid,meta);
			startSaveContactsToStorage(AStreamJid);
			return true;
		}
	}
	return false;
}

bool MetaContacts::setMetaContactName(const Jid &AStreamJid, const QUuid &AMetaId, const QString &AName)
{
	if (isReady(AStreamJid))
	{
		IMetaContact meta = findMetaContact(AStreamJid,AMetaId);
		if (meta.id == AMetaId)
		{
			meta.name = AName;
			if (updateMetaContact(AStreamJid,meta))
			{
				startSaveContactsToStorage(AStreamJid);
				return true;
			}
		}
	}
	return false;
}

bool MetaContacts::setMetaContactItems(const Jid &AStreamJid, const QUuid &AMetaId, const QList<Jid> &AItems)
{
	if (isReady(AStreamJid))
	{
		IMetaContact meta = findMetaContact(AStreamJid,AMetaId);
		if (meta.id == AMetaId)
		{
			meta.items = AItems;
			if (updateMetaContact(AStreamJid,meta))
			{
				startSaveContactsToStorage(AStreamJid);
				return true;
			}
		}
	}
	return false;
}

bool MetaContacts::setMetaContactGroups(const Jid &AStreamJid, const QUuid &AMetaId, const QSet<QString> &AGroups)
{
	if (isReady(AStreamJid))
	{
		IMetaContact meta = findMetaContact(AStreamJid,AMetaId);
		if (meta.id==AMetaId && meta.groups!=AGroups)
		{
			IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AStreamJid) : NULL;
			if (roster && roster->isOpen())
			{
				QSet<QString> newGroups = AGroups - meta.groups;
				QSet<QString> oldGroups = meta.groups - AGroups;
				foreach(const Jid &item, meta.items)
				{
					IRosterItem ritem = roster->rosterItem(item);
					roster->setItem(ritem.itemJid,ritem.name,ritem.groups + newGroups - oldGroups);
				}
				return true;
			}
		}
	}
	return false;
}

bool MetaContacts::isValidItem(const Jid &AStreamJid, const Jid &AItem) const
{
	if (AItem.isValid() && !AItem.node().isEmpty())
	{
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AStreamJid) : NULL;
		if (roster != NULL)
		{
			IRosterItem ritem = roster->rosterItem(AItem);
			return ritem.isValid;
		}
	}
	return false;
}

QList<Jid> MetaContacts::filterValidItems(const Jid &AStreamJid, const QList<Jid> &AItems) const
{
	QList<Jid> items;
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AStreamJid) : NULL;
	if (roster != NULL)
	{
		foreach(const Jid &item, AItems)
		{
			if (item.isValid() && !item.node().isEmpty())
			{
				IRosterItem ritem = roster->rosterItem(item);
				if (ritem.isValid)
					items.append(ritem.itemJid);
			}
		}
	}
	return items;
}

bool MetaContacts::updateMetaContact(const Jid &AStreamJid, const IMetaContact &AMetaContact)
{
	if (!AMetaContact.id.isNull())
	{
		IMetaContact meta = AMetaContact;
		IMetaContact before = findMetaContact(AStreamJid,meta.id);

		meta.items = filterValidItems(AStreamJid,meta.items);
		if (meta.items != before.items)
		{
			if (!meta.items.isEmpty())
			{
				meta.groups.clear();
				IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AStreamJid) : NULL;

				QList<IPresenceItem> metaPresences;
				IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->findPresence(AStreamJid) : NULL;

				foreach(const Jid &item, meta.items)
				{
					if (!before.items.contains(item))
					{
						IMetaContact oldMeta = findMetaContact(AStreamJid,item);
						if (!oldMeta.id.isNull())
							detachMetaContactItems(AStreamJid,oldMeta.id,oldMeta.items);
					}

					if (roster)
					{
						IRosterItem ritem = roster->rosterItem(item);
						if (meta.name.isEmpty())
							meta.name = ritem.name;
						meta.groups += ritem.groups;
					}

					if (presence)
						metaPresences += presence->findItems(item);
				}

				if (FPresencePlugin)
					meta.presence = FPresencePlugin->sortPresenceItems(metaPresences).value(0);
			}
			else
			{
				meta = NullMetaContact;
				meta.id = before.id;
			}
		}

		if (meta != before)
		{
			if (!meta.items.isEmpty())
			{
				FMetaContacts[AStreamJid][meta.id] = meta;
				updateMetaIndex(AStreamJid,meta);
			}
			else
			{
				FMetaContacts[AStreamJid].remove(meta.id);
				removeMetaIndex(AStreamJid,meta.id);
			}
			emit metaContactReceived(AStreamJid,meta,before);
		}

		return !meta.items.isEmpty();
	}
	return false;
}

void MetaContacts::updateMetaContacts(const Jid &AStreamJid, const QList<IMetaContact> &AMetaContacts)
{
	QSet<QUuid> oldMetaId = FMetaContacts.value(AStreamJid).keys().toSet();

	foreach(const IMetaContact &meta, AMetaContacts)
	{
		updateMetaContact(AStreamJid,meta);
		oldMetaId -= meta.id;
	}

	foreach(const QUuid &metaId, oldMetaId)
	{
		IMetaContact before = FMetaContacts[AStreamJid].take(metaId);
		IMetaContact meta = NullMetaContact;
		meta.id = before.id;
		removeMetaIndex(AStreamJid,metaId);
		emit metaContactReceived(AStreamJid,meta,before);
	}
}

bool MetaContacts::isReadyStreams(const QStringList &AStreams) const
{
	foreach(const Jid &streamJid, AStreams)
		if (!isReady(streamJid))
			return false;
	return !AStreams.isEmpty();
}

bool MetaContacts::isSelectionAccepted(const QList<IRosterIndex *> &ASelected) const
{
	foreach(IRosterIndex *index, ASelected)
	{
		int kind = index->data(RDR_KIND).toInt();
		if (kind!=RIK_CONTACT && kind!=RIK_METACONTACT)
			return false;
		else if (kind==RIK_CONTACT && !isValidItem(index->data(RDR_STREAM_JID).toString(),index->data(RDR_PREP_BARE_JID).toString()))
			return false;
	}
	return !ASelected.isEmpty();
}

void MetaContacts::updateMetaIndex(const Jid &AStreamJid, const IMetaContact &AMetaContact)
{
	IRosterIndex *sroot = FRostersModel!=NULL ? FRostersModel->streamRoot(AStreamJid) : NULL;
	if (sroot)
	{
		QList<IRosterIndex *> indexList = FMetaIndexes[AStreamJid][AMetaContact.id];
		
		QMap<QString, IRosterIndex *> indexGroup;
		foreach(IRosterIndex *index, indexList)
		{
			QString group = index->data(RDR_GROUP).toString();
			indexGroup.insert(group,index);
		}

		QSet<QString> curGroups = indexGroup.keys().toSet();
		QSet<QString> newGroups = AMetaContact.groups - curGroups;
		QSet<QString> oldGroups = curGroups - AMetaContact.groups;

		QSet<QString>::iterator oldGroupsIt = oldGroups.begin();
		foreach(const QString &group, newGroups)
		{
			IRosterIndex *groupIndex = FRostersModel->getGroupIndex(RIK_GROUP,group,sroot);

			IRosterIndex *index;
			if (oldGroupsIt == oldGroups.end())
			{
				index = FRostersModel->newRosterIndex(RIK_METACONTACT);
				index->setData(AStreamJid.pFull(),RDR_STREAM_JID);
				index->setData(AMetaContact.id.toString(),RDR_METACONTACT_ID);
				indexList.append(index);
			}
			else
			{
				QString oldGroup = *oldGroupsIt;
				oldGroupsIt = oldGroups.erase(oldGroupsIt);
				index = indexGroup.value(oldGroup);
			}

			index->setData(group,RDR_GROUP);
			FRostersModel->insertRosterIndex(index,groupIndex);
		}
		
		while(oldGroupsIt != oldGroups.end())
		{
			IRosterIndex *index = indexGroup.value(*oldGroupsIt);
			indexList.removeAll(index);
			FRostersModel->removeRosterIndex(index);
			oldGroupsIt = oldGroups.erase(oldGroupsIt);
		}

		foreach(IRosterIndex *index, indexList)
		{
			index->setData(AMetaContact.name,RDR_NAME);
			index->setData(AMetaContact.presence.show,RDR_SHOW);
			index->setData(AMetaContact.presence.status,RDR_STATUS);
			index->setData(AMetaContact.presence.priority,RDR_PRIORITY);
		}
	}
}

void MetaContacts::removeMetaIndex(const Jid &AStreamJid, const QUuid &AMetaId)
{
	if (FRostersModel)
	{
		foreach(IRosterIndex *index, FMetaIndexes[AStreamJid].take(AMetaId))
			FRostersModel->removeRosterIndex(index);
	}
}

void MetaContacts::showCombineContactsDialog(const Jid &AStreamJid, const QStringList &AContactJids, const QStringList &AMetaIds)
{
	if (isReady(AStreamJid))
	{
		CombineContactsDialog *dialog = new CombineContactsDialog(this,AStreamJid,AContactJids,AMetaIds);
		WidgetManager::showActivateRaiseWindow(dialog);
	}
}

void MetaContacts::startSaveContactsToStorage(const Jid &AStreamJid)
{
	if (FPrivateStorage)
	{
		FSaveStreams += AStreamJid;
		FSaveTimer.start(STORAGE_SAVE_TIMEOUT);
	}
}

bool MetaContacts::saveContactsToStorage(const Jid &AStreamJid) const
{
	if (FPrivateStorage && isReady(AStreamJid))
	{
		QDomDocument doc;
		QDomElement storageElem = doc.appendChild(doc.createElementNS(NS_STORAGE_METACONTACTS,"storage")).toElement();
		saveMetaContactsToXML(storageElem,FMetaContacts.value(AStreamJid).values());
		if (!FPrivateStorage->saveData(AStreamJid,storageElem).isEmpty())
			LOG_STRM_INFO(AStreamJid,"Save meta-contacts to storage request sent");
		else
			LOG_STRM_WARNING(AStreamJid,"Failed to send save meta-contacts to storage request");
		return true;
	}
	return false;
}

QString MetaContacts::metaContactsFileName(const Jid &AStreamJid) const
{
	QDir dir(FPluginManager->homePath());
	if (!dir.exists(DIR_METACONTACTS))
		dir.mkdir(DIR_METACONTACTS);
	dir.cd(DIR_METACONTACTS);
	return dir.absoluteFilePath(Jid::encode(AStreamJid.pBare())+".xml");
}

QList<IMetaContact> MetaContacts::loadMetaContactsFromXML(const QDomElement &AElement) const
{
	QList<IMetaContact> contacts;
	QDomElement metaElem = AElement.firstChildElement("meta");
	while (!metaElem.isNull())
	{
		IMetaContact meta;
		meta.id = metaElem.attribute("id");
		meta.name = metaElem.attribute("name");

		QDomElement itemElem = metaElem.firstChildElement("item");
		while (!itemElem.isNull())
		{
			meta.items.append(itemElem.text());
			itemElem = itemElem.nextSiblingElement("item");
		}

		contacts.append(meta);
		metaElem = metaElem.nextSiblingElement("meta");
	}
	return contacts;
}

void MetaContacts::saveMetaContactsToXML(QDomElement &AElement, const QList<IMetaContact> &AContacts) const
{
	for (QList<IMetaContact>::const_iterator metaIt=AContacts.constBegin(); metaIt!=AContacts.constEnd(); ++metaIt)
	{
		QDomElement metaElem = AElement.ownerDocument().createElement("meta");
		metaElem.setAttribute("id",metaIt->id.toString());
		metaElem.setAttribute("name",metaIt->name);

		for (QList<Jid>::const_iterator itemIt=metaIt->items.constBegin(); itemIt!=metaIt->items.constEnd(); ++itemIt)
		{
			QDomElement itemElem = AElement.ownerDocument().createElement("item");
			itemElem.appendChild(AElement.ownerDocument().createTextNode(itemIt->pBare()));
			metaElem.appendChild(itemElem);
		}

		AElement.appendChild(metaElem);
	}
}

QList<IMetaContact> MetaContacts::loadMetaContactsFromFile(const QString &AFileName) const
{
	QList<IMetaContact> contacts;

	QFile file(AFileName);
	if (file.open(QIODevice::ReadOnly))
	{
		QDomDocument doc;
		if (doc.setContent(file.readAll()))
		{
			QDomElement storageElem = doc.firstChildElement("storage");
			contacts = loadMetaContactsFromXML(storageElem);
		}
		file.close();
	}
	else if (file.exists())
	{
		REPORT_ERROR(QString("Failed to load meta-contacts from file: %1").arg(file.errorString()));
	}

	return contacts;
}

void MetaContacts::saveMetaContactsToFile(const QString &AFileName, const QList<IMetaContact> &AContacts) const
{
	QFile file(AFileName);
	if (file.open(QIODevice::WriteOnly | QIODevice::Truncate))
	{
		QDomDocument doc;
		QDomElement storageElem = doc.appendChild(doc.createElementNS(NS_STORAGE_METACONTACTS,"storage")).toElement();
		saveMetaContactsToXML(storageElem,AContacts);
		file.write(doc.toByteArray());
		file.close();
	}
	else
	{
		REPORT_ERROR(QString("Failed to save meta-contact to file: %1").arg(file.errorString()));
	}
}

void MetaContacts::onRosterAdded(IRoster *ARoster)
{
	FMetaContacts[ARoster->streamJid()].clear();
	updateMetaContacts(ARoster->streamJid(),loadMetaContactsFromFile(metaContactsFileName(ARoster->streamJid())));
}

void MetaContacts::onRosterRemoved(IRoster *ARoster)
{
	QHash<QUuid, IMetaContact> metas = FMetaContacts.take(ARoster->streamJid());
	saveMetaContactsToFile(metaContactsFileName(ARoster->streamJid()),metas.values());

	FSaveStreams -= ARoster->streamJid();
	FMetaIndexes.remove(ARoster->streamJid());
	FItemMetaContact.remove(ARoster->streamJid());
}

void MetaContacts::onRosterStreamJidChanged(IRoster *ARoster, const Jid &ABefore)
{
	if (FSaveStreams.contains(ABefore))
	{
		FSaveStreams -= ABefore;
		FSaveStreams += ARoster->streamJid();
	}

	FMetaContacts[ARoster->streamJid()] = FMetaContacts.take(ABefore);
	FItemMetaContact[ARoster->streamJid()] = FItemMetaContact.take(ABefore);
	
	QHash<QUuid, QList<IRosterIndex *> > indexes = FMetaIndexes.take(ABefore);
	for (QHash<QUuid, QList<IRosterIndex *> >::const_iterator metaIt = indexes.constBegin(); metaIt!=indexes.constEnd(); ++metaIt)
		for (QList<IRosterIndex *>::const_iterator indexIt=metaIt->constBegin(); indexIt!=metaIt->constEnd(); ++indexIt)
			(*indexIt)->setData(ARoster->streamJid().pFull(),RDR_STREAM_JID);
	FMetaIndexes.insert(ARoster->streamJid(),indexes);
}

void MetaContacts::onPrivateStorageOpened(const Jid &AStreamJid)
{
	if (!FPrivateStorage->loadData(AStreamJid,"storage",NS_STORAGE_METACONTACTS).isEmpty())
		LOG_STRM_INFO(AStreamJid,"Load meta-contacts from storage request sent");
	else
		LOG_STRM_WARNING(AStreamJid,"Failed to send load meta-contacts from storage request");
}

void MetaContacts::onPrivateStorageDataLoaded(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement)
{
	Q_UNUSED(AId);
	if (AElement.namespaceURI() == NS_STORAGE_METACONTACTS)
	{
		LOG_STRM_INFO(AStreamJid,"Meta-contacts loaded from storage");
		updateMetaContacts(AStreamJid,loadMetaContactsFromXML(AElement));
	}
}

void MetaContacts::onPrivateStorageDataChanged(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace)
{
	Q_UNUSED(ATagName);
	if (ANamespace == NS_STORAGE_METACONTACTS)
	{
		if (!FPrivateStorage->loadData(AStreamJid,ATagName,NS_STORAGE_METACONTACTS).isEmpty())
			LOG_STRM_INFO(AStreamJid,"Reload meta-contacts from storage request sent");
		else
			LOG_STRM_WARNING(AStreamJid,"Failed to send reload meta-contacts from storage request");
	}
}

void MetaContacts::onPrivateStorageNotifyAboutToClose(const Jid &AStreamJid)
{
	if (FSaveStreams.contains(AStreamJid))
	{
		FSaveStreams -= AStreamJid;
		saveContactsToStorage(AStreamJid);
	}
}

void MetaContacts::onRostersViewIndexMultiSelection(const QList<IRosterIndex *> &ASelected, bool &AAccepted)
{
	AAccepted = AAccepted || isSelectionAccepted(ASelected);
}

void MetaContacts::onRostersViewIndexContextMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu)
{
	if (ALabelId==AdvancedDelegateItem::DisplayId && isSelectionAccepted(AIndexes))
	{
		QMap<int, QStringList> rolesMap = FRostersView->indexesRolesMap(AIndexes,QList<int>()<<RDR_STREAM_JID<<RDR_PREP_BARE_JID<<RDR_METACONTACT_ID);
		if (isReadyStreams(rolesMap.value(RDR_STREAM_JID)))
		{
			Jid streamJid;
			bool isSameStream = true;
			foreach(const QString stream, rolesMap.value(RDR_STREAM_JID))
			{
				if (streamJid.isEmpty())
				{
					streamJid = stream;
				}
				else if (streamJid != stream)
				{
					isSameStream = false;
					break;
				}
			}
			
			if (isSameStream && FRostersView->hasMultiSelection())
			{
				Action *combineAction = new Action(AMenu);
				combineAction->setText(tr("Combine Contacts..."));
				combineAction->setData(ADR_STREAM_JID,streamJid.pFull());
				combineAction->setData(ADR_CONTACT_JID,rolesMap.value(RDR_PREP_BARE_JID));
				combineAction->setData(ADR_METACONTACT_ID,rolesMap.value(RDR_METACONTACT_ID));
				connect(combineAction,SIGNAL(triggered()),SLOT(onCombineContactsByAction()));
				AMenu->addAction(combineAction,AG_RVCM_METACONTACTS);
			}
		}
	}
}

void MetaContacts::onCombineContactsByAction()
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
		showCombineContactsDialog(action->data(ADR_STREAM_JID).toString(),action->data(ADR_CONTACT_JID).toStringList(),action->data(ADR_METACONTACT_ID).toStringList());
}

void MetaContacts::onSaveContactsToStorageTimerTimeout()
{
	for (QSet<Jid>::iterator it=FSaveStreams.begin(); it!=FSaveStreams.end(); )
	{
		if (saveContactsToStorage(*it))
			it = FSaveStreams.erase(it);
		else
			++it;
	}
}

Q_EXPORT_PLUGIN2(plg_metacontacts, MetaContacts)
