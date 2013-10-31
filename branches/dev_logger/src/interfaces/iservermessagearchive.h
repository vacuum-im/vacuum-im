#ifndef ISERVERMESSAGEARCHIVE_H
#define ISERVERMESSAGEARCHIVE_H

#include <interfaces/imessagearchiver.h>

#define SERVERMESSAGEARCHIVE_UUID "{5309B204-651E-4cc8-9993-6A50D66301AA}"

class IServerMesssageArchive :
	public IArchiveEngine
{
public:
	virtual QObject *instance() =0;
	virtual QString loadServerHeaders(const Jid &AStreamJid, const IArchiveRequest &ARequest, const QString &ANextRef = QString::null) =0;
	virtual QString saveServerCollection(const Jid &AStreamJid, const IArchiveCollection &ACollection, const QString &ANextRef = QString::null) =0;
	virtual QString loadServerCollection(const Jid &AStreamJid, const IArchiveHeader &AHeader, const QString &ANextRef = QString::null) =0;
	virtual QString loadServerModifications(const Jid &AStreamJid, const QDateTime &AStart, int ACount, const QString &ANextRef = QString::null) =0;
protected:
	virtual void serverHeadersLoaded(const QString &AId, const QList<IArchiveHeader> &AHeaders, const QString &ANextRef) =0;
	virtual void serverCollectionSaved(const QString &AId, const IArchiveCollection &ACollection, const QString &ANextRef) =0;
	virtual void serverCollectionLoaded(const QString &AId, const IArchiveCollection &ACollection, const QString &ANextRef) =0;
	virtual void serverModificationsLoaded(const QString &AId, const IArchiveModifications &AModifs, const QString &ANextRef) =0;
};

Q_DECLARE_INTERFACE(IServerMesssageArchive,"Vacuum.Plugin.IServerMesssageArchive/1.2")

#endif //ISERVERMESSAGEARCHIVE_H
