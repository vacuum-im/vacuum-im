#ifndef IBITSOFBINARY_H
#define IBITSOFBINARY_H

#include <QString>
#include <QByteArray>
#include <utils/jid.h>
#include <utils/stanza.h>
#include <utils/xmpperror.h>

#define BITSOFBINARY_UUID "{44d5c538-2254-4ae3-a78d-0c20a76ef87b}"

class IBitsOfBinary
{
public:
	virtual QObject *instance() =0;
	virtual QString contentIdentifier(const QByteArray &AData) const =0;
	virtual bool isSupported(const Jid &AStreamJid, const Jid &AContactJid) const =0;
	virtual bool hasBinary(const QString &AContentId) const =0;
	virtual bool loadBinary(const QString &AContentId, const Jid &AStreamJid, const Jid &AContactJid) =0;
	virtual bool loadBinary(const QString &AContentId, QString &AType, QByteArray &AData, quint64 &AMaxAge) =0;
	virtual bool saveBinary(const QString &AContentId, const QString &AType, const QByteArray &AData, quint64 AMaxAge) =0;
	virtual bool saveBinary(const QString &AContentId, const QString &AType, const QByteArray &AData, quint64 AMaxAge, Stanza &AStanza) =0;
	virtual bool removeBinary(const QString &AContentId) =0;
protected:
	virtual void binaryCached(const QString &AContentId, const QString &AType, const QByteArray &AData, quint64 AMaxAge) =0;
	virtual void binaryError(const QString &AContentId, const XmppError &AError) =0;
	virtual void binaryRemoved(const QString &AContentId) =0;
};

Q_DECLARE_INTERFACE(IBitsOfBinary,"Vacuum.Plugin.IBitsOfBinary/1.1")

#endif
