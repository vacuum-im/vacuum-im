#ifndef FILEMESSAGEARCHIVE_H
#define FILEMESSAGEARCHIVE_H

#include <interfaces/ipluginmanager.h>
#include <interfaces/ifilemessagearchive.h>
#include <interfaces/imessagearchiver.h>
#include "collectionwriter.h"

class FileMessageArchive : 
	public QObject,
	public IPlugin,
	public IFileMessageArchive
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IArchiveEngine IFileMessageArchive);
public:
	FileMessageArchive();
	~FileMessageArchive();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return FILEMESSAGEARCHIVE_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects() { return true; }
	virtual bool initSettings() { return true; }
	virtual bool startPlugin()  { return true; }
	//IArchiveEngine
	virtual QUuid engineId() const;
	virtual QString engineName() const;
	virtual QString engineDescription() const;
	virtual uint capabilities(const Jid &AStreamJid = Jid::null) const;
	virtual bool isCapable(const Jid &AStreamJid, uint ACapability) const;
	virtual bool saveNote(const Jid &AStreamJid, const Message &AMessage, bool ADirectionIn);
	virtual bool saveMessage(const Jid &AStreamJid, const Message &AMessage, bool ADirectionIn);
	virtual QString saveCollection(const Jid &AStreamJid, const IArchiveCollection &ACollection);
	virtual QString removeCollections(const Jid &AStreamJid, const IArchiveRequest &ARequest, bool AOpened = false);
	virtual QString loadHeaders(const Jid AStreamJid, const IArchiveRequest &ARequest, const QString &AAfter = QString::null);
	virtual QString loadCollection(const Jid AStreamJid, const IArchiveHeader &AHeader, const QString &AAfter = QString::null);
	virtual QString loadModifications(const Jid &AStreamJid, const QDateTime &AStart, int ACount, const QString &AAfter = QString::null);
	//IFileMessageArchive
	virtual QString collectionDirName(const Jid &AWith) const;
	virtual QString collectionFileName(const QDateTime &AStart) const;
	virtual QString collectionDirPath(const Jid &AStreamJid, const Jid &AWith) const;
	virtual QString collectionFilePath(const Jid &AStreamJid, const Jid &AWith, const QDateTime &AStart) const;
	virtual QStringList findCollectionFiles(const Jid &AStreamJid, const IArchiveRequest &ARequest) const;
	virtual IArchiveHeader loadHeaderFromFile(const QString &AFileName) const;
	virtual IArchiveCollection loadCollectionFromFile(const QString &AFileName) const;
	virtual IArchiveModifications loadFileModifications(const Jid &AStreamJid, const QDateTime &AStart, int ACount) const;
	virtual bool saveCollectionToFile(const Jid &AStreamJid, const IArchiveCollection &ACollection, const QString &ASaveMode, bool AAppend = true);
	virtual bool removeCollectionFile(const Jid &AStreamJid, const Jid &AWith, const QDateTime &AStart);
signals:
	//IArchiveEngine
	void capabilitiesChanged(const Jid &AStreamJid);
	void requestFailed(const QString &AId, const QString &AError);
	void collectionSaved(const QString &AId, const IArchiveHeader &AHeader);
	void collectionsRemoved(const QString &AId, const IArchiveRequest &ARequest);
	void headersLoaded(const QString &AId, const QList<IArchiveHeader> &AHeaders, const IArchiveResultSet &AResult);
	void collectionLoaded(const QString &AId, const IArchiveCollection &ACollection, const IArchiveResultSet &AResult);
	void modificationsLoaded(const QString &AId, const IArchiveModifications &AModifs, const IArchiveResultSet &AResult);
	//IFileMessageArchive
	void fileCollectionOpened(const Jid &AStreamJid, const IArchiveHeader &AHeader);
	void fileCollectionSaved(const Jid &AStreamJid, const IArchiveHeader &AHeader);
	void fileCollectionRemoved(const Jid &AStreamJid, const IArchiveHeader &AHeader);
protected:
	bool saveFileModification(const Jid &AStreamJid, const IArchiveHeader &AHeader, const QString &AAction) const;
	CollectionWriter *findCollectionWriter(const Jid &AStreamJid, const IArchiveHeader &AHeader) const;
	CollectionWriter *findCollectionWriter(const Jid &AStreamJid, const Jid &AWith, const QString &AThreadId) const;
	CollectionWriter *getCollectionWriter(const Jid &AStreamJid, const IArchiveHeader &AHeader);
protected slots:
	void onWorkingThreadFinished();
	void onCollectionWriterDestroyed(CollectionWriter *AWriter);
private:
	IPluginManager *FPluginManager;
	IMessageArchiver *FMessageArchiver;
private:
	QMap<Jid, QMultiMap<Jid,CollectionWriter *> > FCollectionWriters;
};

#endif // FILEMESSAGEARCHIVE_H
