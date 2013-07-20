#include "metacontacts.h"

#include <QDir>
#include <definitions/namespaces.h>

#define DIR_METACONTACTS        "metacontacts"

MetaContacts::MetaContacts()
{
	FPluginManager = NULL;
	FPrivateStorage = NULL;
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

	return FPrivateStorage!=NULL;
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

bool MetaContacts::processMetaContact(const Jid &AStreamJid, const IMetaContact &AContact)
{
	if (!AContact.id.isNull())
	{
		FMetaContacts[AStreamJid].insert(AContact.id,AContact);
		return true;
	}
	return false;
}

void MetaContacts::processMetaContacts(const Jid &AStreamJid, const QList<IMetaContact> &AContacts)
{
	QSet<QUuid> oldContacts = FMetaContacts.value(AStreamJid).keys().toSet();

	foreach(const IMetaContact &contact, AContacts)
	{
		if (processMetaContact(AStreamJid,contact))
			oldContacts -= contact.id;
	}

	foreach(const QUuid &id, oldContacts)
		FMetaContacts[AStreamJid].remove(id);
}

bool MetaContacts::saveContactsToStorage( const Jid &AStreamJid ) const
{
	if (FPrivateStorage && isReady(AStreamJid))
	{
		QDomDocument doc;
		QDomElement storageElem = doc.appendChild(doc.createElementNS(NS_STORAGE_METACONTACTS,"storage")).toElement();
		saveMetaContactsToXML(storageElem,FMetaContacts.value(AStreamJid).values());
		FPrivateStorage->saveData(AStreamJid,storageElem);
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
		IMetaContact contact;
		contact.id = metaElem.attribute("id");
		contact.name = metaElem.attribute("name");

		QDomElement itemElem = metaElem.firstChildElement("item");
		while (!itemElem.isNull())
		{
			contact.items.append(itemElem.text());
			itemElem = itemElem.nextSiblingElement("item");
		}

		contacts.append(contact);
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
	if (file.exists() && file.open(QIODevice::ReadOnly))
	{
		QDomDocument doc;
		if (doc.setContent(file.readAll()))
		{
			QDomElement storageElem = doc.firstChildElement("storage");
			contacts = loadMetaContactsFromXML(storageElem);
		}
		file.close();
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
}

void MetaContacts::onPrivateStorageOpened(const Jid &AStreamJid)
{
	FPrivateStorage->loadData(AStreamJid,"storage",NS_STORAGE_METACONTACTS);
}

void MetaContacts::onPrivateStorageDataLoaded(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement)
{
	Q_UNUSED(AId);
	if (AElement.namespaceURI() == NS_STORAGE_METACONTACTS)
	{
		foreach(const IMetaContact &contact, loadMetaContactsFromXML(AElement))
			processMetaContact(AStreamJid,contact);
	}
}

void MetaContacts::onPrivateStorageDataChanged(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace)
{
	Q_UNUSED(ATagName);
	if (ANamespace == NS_STORAGE_METACONTACTS)
		FPrivateStorage->loadData(AStreamJid,ATagName,NS_STORAGE_METACONTACTS);
}

void MetaContacts::onPrivateStorageNotifyAboutToClose(const Jid &AStreamJid)
{
	saveContactsToStorage(AStreamJid);
}
