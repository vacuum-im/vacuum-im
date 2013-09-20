#ifndef IFILEMESSAGEARCHIVE_H
#define IFILEMESSAGEARCHIVE_H

#include <interfaces/imessagearchiver.h>

#define FILEMESSAGEARCHIVE_UUID "{2F1E540F-60D3-490f-8BE9-0EEA693B8B83}"

class IFileMessageArchive : 
	public IArchiveEngine
{
public:
	virtual QObject *instance() =0;
	// Files
	virtual QString fileArchiveRootPath() const =0;
	virtual QString fileArchivePath(const Jid &AStreamJid) const =0;
	virtual QString contactGateType(const Jid &AContactJid) const =0;
	virtual QString collectionDirName(const Jid &AWith) const =0;
	virtual QString collectionFileName(const QDateTime &AStart) const =0;
	virtual QString collectionDirPath(const Jid &AStreamJid, const Jid &AWith) const =0;
	virtual QString collectionFilePath(const Jid &AStreamJid, const Jid &AWith, const QDateTime &AStart) const =0;
	virtual IArchiveHeader loadFileHeader(const QString &AFilePath) const =0;
	virtual IArchiveCollection loadFileCollection(const QString &AFilePath) const =0;
	virtual QList<IArchiveHeader> loadFileHeaders(const Jid &AStreamJid, const IArchiveRequest &ARequest) const =0;
	virtual bool saveFileCollection(const Jid &AStreamJid, const IArchiveCollection &ACollection, const QString &ASaveMode, bool AAppend = true) =0;
	virtual bool removeFileCollection(const Jid &AStreamJid, const Jid &AWith, const QDateTime &AStart) =0;
	// Database
	virtual bool isDatabaseReady(const Jid &AStreamJid) const =0;
	virtual QString databaseArchiveFile(const Jid &AStreamJid) const =0;
	virtual QString databaseProperty(const Jid &AStreamJid, const QString &AParam) const =0;
	virtual bool setDatabaseProperty(const Jid &AStreamJid, const QString &AParam, const QString &AValue) =0;
	virtual QList<IArchiveHeader> loadDatabaseHeaders(const Jid &AStreamJid, const IArchiveRequest &ARequest) const =0;
	virtual IArchiveModifications loadDatabaseModifications(const Jid &AStreamJid, const QDateTime &AStart, int ACount) const =0;
protected:
	virtual void databaseOpened(const Jid &AStreamJid) =0;
	virtual void databaseAboutToClose(const Jid &AStreamJid) =0;
	virtual void databaseClosed(const Jid &AStreamJid) =0;
	virtual void databasePropertyChanged(const Jid &AStreamJid, const QString &AProperty) =0;
};

Q_DECLARE_INTERFACE(IFileMessageArchive,"Vacuum.Plugin.IFileMessageArchive/1.1")

#endif // IFILEMESSAGEARCHIVE_H
