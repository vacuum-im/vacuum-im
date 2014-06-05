#ifndef IMETACONTACTS_H
#define IMETACONTACTS_H

#include <QSet>
#include <QUuid>
#include <interfaces/ipresence.h>
#include <utils/jid.h>

#define METACONTACTS_UUID "{5E7FEFAD-FFD0-4782-8F3C-5B76907F8735}"

struct IMetaContact {
	QUuid id;
	QString name;
	QList<Jid> items;
	QSet<QString> groups;
	IPresenceItem presence;
	bool operator==(const IMetaContact &AOther) const {
		return id==AOther.id && name==AOther.name && items==AOther.items && groups==AOther.groups && presence==AOther.presence;
	}
	bool operator!=(const IMetaContact &AOther) const {
		return !operator==(AOther);
	}
};

class IMetaContacts
{
public:
	virtual QObject *instance() = 0;
};

Q_DECLARE_INTERFACE(IMetaContacts,"Vacuum.Plugin.IMetaContacts/1.0")

#endif // IMETACONTACTS_H
