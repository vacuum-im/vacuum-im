#ifndef IFILETRANSFER_H
#define IFILETRANSFER_H

#include <QString>
#include <interfaces/ifilestreamsmanager.h>
#include <utils/jid.h>

#define FILETRANSFER_UUID "{6e1cc70e-5604-4857-b742-ba185323bb4b}"

class IFileTransfer
{
public:
	virtual QObject *instance() =0;
	virtual bool isSupported(const Jid &AStreamJid, const Jid &AContactJid) const =0;
	virtual IFileStream *sendFile(const Jid &AStreamJid, const Jid &AContactJid, const QString &AFileName = QString::null, const QString &AFileDesc = QString::null) =0;
};

Q_DECLARE_INTERFACE(IFileTransfer,"Vacuum.Plugin.IFileTransfer/1.0")

#endif // IFILETRANSFER_H
