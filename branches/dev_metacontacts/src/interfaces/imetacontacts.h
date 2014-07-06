#ifndef IMETACONTACTS_H
#define IMETACONTACTS_H

#include <QSet>
#include <QUuid>
#include <interfaces/ipresence.h>
#include <interfaces/irostersmodel.h>
#include <utils/jid.h>

#define METACONTACTS_UUID "{5E7FEFAD-FFD0-4782-8F3C-5B76907F8735}"

struct IMetaContact {
	QUuid id;
	QString name;
	QList<Jid> items;
	QSet<QString> groups;
	QList<IPresenceItem> presences;
	inline bool isNull() const {
		return id.isNull();
	}
	inline bool isEmpty() const {
		return items.isEmpty();
	}
	inline bool operator==(const IMetaContact &AOther) const {
		return id==AOther.id && name==AOther.name && items==AOther.items && groups==AOther.groups && presences==AOther.presences;
	}
	inline bool operator!=(const IMetaContact &AOther) const {
		return !operator==(AOther);
	}
};

class IMetaContacts
{
public:
	virtual QObject *instance() = 0;
	virtual bool isReady(const Jid &AStreamJid) const =0;
	virtual IMetaContact findMetaContact(const Jid &AStreamJid, const Jid &AItem) const =0;
	virtual IMetaContact findMetaContact(const Jid &AStreamJid, const QUuid &AMetaId) const =0;
	virtual QList<IRosterIndex *> findMetaIndexes(const Jid &AStreamJid, const QUuid &AMetaId) const =0;
	virtual QUuid createMetaContact(const Jid &AStreamJid, const QList<Jid> &AItems, const QString &AName) =0;
	virtual bool mergeMetaContacts(const Jid &AStreamJid, const QUuid &AMetaId1, const QUuid &AMetaId2) =0;
	virtual bool detachMetaContactItems(const Jid &AStreamJid, const QUuid &AMetaId, const QList<Jid> &AItems) =0;
	virtual bool setMetaContactName(const Jid &AStreamJid, const QUuid &AMetaId, const QString &AName) =0;
	virtual bool setMetaContactItems(const Jid &AStreamJid, const QUuid &AMetaId, const QList<Jid> &AItems) =0;
	virtual bool setMetaContactGroups(const Jid &AStreamJid, const QUuid &AMetaId, const QSet<QString> &AGroups) =0;
protected:
	virtual void metaContactChanged(const Jid &AStreamJid, const IMetaContact &AMetaContact, const IMetaContact &ABefore) =0;
};

Q_DECLARE_INTERFACE(IMetaContacts,"Vacuum.Plugin.IMetaContacts/1.0")

#endif // IMETACONTACTS_H
