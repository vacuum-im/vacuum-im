#ifndef IFILEMESSAGEARCHIVE_H
#define IFILEMESSAGEARCHIVE_H

#include <interfaces/imessagearchiver.h>

#define FILEMESSAGEARCHIVE_UUID "{2F1E540F-60D3-490f-8BE9-0EEA693B8B83}"

class IFileMessageArchive : 
	public IArchiveEngine
{
public:
	virtual QObject *instance() =0;
	virtual QString collectionDirName(const Jid &AWith) const =0;
	virtual QString collectionFileName(const QDateTime &AStart) const =0;
	virtual QString collectionDirPath(const Jid &AStreamJid, const Jid &AWith) const =0;
	virtual QString collectionFilePath(const Jid &AStreamJid, const Jid &AWith, const QDateTime &AStart) const =0;
	virtual QStringList findCollectionFiles(const Jid &AStreamJid, const IArchiveRequest &ARequest) const =0;
	virtual IArchiveHeader loadHeaderFromFile(const QString &AFileName) const =0;
	virtual IArchiveCollection loadCollectionFromFile(const QString &AFileName) const =0;
	virtual bool saveCollectionToFile(const Jid &AStreamJid, const IArchiveCollection &ACollection, const QString &ASaveMode, bool AAppend = true) const =0;
	virtual bool removeCollectionFile(const Jid &AStreamJid, const Jid &AWith, const QDateTime &AStart) const =0;
	virtual IArchiveModifications loadFileModifications(const Jid &AStreamJid, const QDateTime &AStart, int ACount) const =0;
};

Q_DECLARE_INTERFACE(IFileMessageArchive,"Vacuum.Plugin.IFileMessageArchive/1.0")

#endif // IFILEMESSAGEARCHIVE_H
