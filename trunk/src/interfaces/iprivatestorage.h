#ifndef IPRIVATESTORAGE_H
#define IPRIVATESTORAGE_H

#define PRIVATESTORAGE_UUID "{E601766D-8867-47c5-B639-92DDEC224B33}"

#include <QDomElement>
#include <utils/jid.h>

class IPrivateStorage
{
public:
	virtual QObject *instance() =0;
	virtual bool hasData(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace) const =0;
	virtual QDomElement getData(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace) const =0;
	virtual QString saveData(const Jid &AStreamJid, const QDomElement &AElement) =0;
	virtual QString loadData(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace) =0;
	virtual QString removeData(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace) =0;
protected:
	virtual void storageOpened(const Jid &AStreamJid) =0;
	virtual void dataSaved(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement) =0;
	virtual void dataLoaded(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement) =0;
	virtual void dataRemoved(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement) =0;
	virtual void dataError(const QString &AId, const QString &AError) =0;
	virtual void storageClosed(const Jid &AStreamJid) =0;
};

Q_DECLARE_INTERFACE(IPrivateStorage,"Vacuum.Plugin.IPrivateStorage/1.0")

#endif
