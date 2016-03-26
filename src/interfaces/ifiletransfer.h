#ifndef IFILETRANSFER_H
#define IFILETRANSFER_H

#include <QString>
#include <interfaces/ifilestreamsmanager.h>
#include <utils/xmpperror.h>
#include <utils/stanza.h>
#include <utils/jid.h>

#define FILETRANSFER_UUID "{6e1cc70e-5604-4857-b742-ba185323bb4b}"

struct IPublicFile {
	QString id;
	Jid ownerJid;
	QString mimeType;
	QString name;
	qint64 size;
	QString hash;
	QDateTime date;
	QString description;

	inline bool isNull() const {
		return id.isEmpty();
	}
	inline bool isValid() const {
		return !id.isEmpty() && ownerJid.isValid() && !name.isEmpty() && size>0;
	}
};

class IFileTransfer
{
public:
	virtual QObject *instance() =0;
	// Send Files
	virtual bool isSupported(const Jid &AStreamJid, const Jid &AContactJid) const =0;
	virtual IFileStream *sendFile(const Jid &AStreamJid, const Jid &AContactJid, const QString &AFileName=QString::null, const QString &AFileDesc=QString::null) =0;
	//Send Public Files
	virtual IPublicFile findPublicFile(const QString &AFileId) const =0;
	virtual QList<IPublicFile> findPublicFiles(const Jid &AOwnerJid=Jid::null, const QString &AFileName=QString::null) const =0;
	virtual QString registerPublicFile(const Jid &AOwnerJid, const QString &AFileName, const QString &AFileDesc=QString::null) =0;
	virtual void removePublicFile(const QString &AFileId) =0;
	//Receive Public Files
	virtual QList<IPublicFile> readPublicFiles(const QDomElement &AParent) const =0;
	virtual bool writePublicFile(const QString &AFileId, QDomElement &AParent) const =0;
	virtual QString receivePublicFile(const Jid &AStreamJid, const Jid &AContactJid, const QString &AFileId) =0;
protected:
	virtual void publicFileSendStarted(const QString &AFileId, IFileStream *AStream) =0;
	virtual void publicFileReceiveStarted(const QString &ARequestId, IFileStream *AStream) =0;
	virtual void publicFileReceiveRejected(const QString &ARequestId, const XmppError &AError) =0;
};

Q_DECLARE_INTERFACE(IFileTransfer,"Vacuum.Plugin.IFileTransfer/1.1")

#endif // IFILETRANSFER_H
