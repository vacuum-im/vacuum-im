#ifndef FILEMESSAGEARCHIVE_H
#define FILEMESSAGEARCHIVE_H

#include <QReadWriteLock>
#include <definitions/optionvalues.h>
#include <definitions/archivecapabilityorders.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ifilemessagearchive.h>
#include <interfaces/imessagearchiver.h>
#include <interfaces/iservicediscovery.h>
#include <utils/options.h>
#include "filetask.h"
#include "filewriter.h"
#include "filearchiveoptions.h"

class FileMessageArchive : 
	public QObject,
	public IPlugin,
	public IFileMessageArchive
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IArchiveEngine IFileMessageArchive);
	Q_PLUGIN_METADATA(IID "org.jrudevels.vacuum.IFileMessageArchive");
public:
	FileMessageArchive();
	~FileMessageArchive();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return FILEMESSAGEARCHIVE_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin()  { return true; }
	//IArchiveEngine
	virtual QUuid engineId() const;
	virtual QString engineName() const;
	virtual QString engineDescription() const;
	virtual IOptionsWidget *engineSettingsWidget(QWidget *AParent);
	virtual quint32 capabilities(const Jid &AStreamJid = Jid::null) const;
	virtual bool isCapable(const Jid &AStreamJid, quint32 ACapability) const;
	virtual int capabilityOrder(quint32 ACapability, const Jid &AStreamJid = Jid::null) const;
	virtual bool saveMessage(const Jid &AStreamJid, const Message &AMessage, bool ADirectionIn);
	virtual bool saveNote(const Jid &AStreamJid, const Message &AMessage, bool ADirectionIn);
	virtual QString saveCollection(const Jid &AStreamJid, const IArchiveCollection &ACollection);
	virtual QString loadHeaders(const Jid &AStreamJid, const IArchiveRequest &ARequest);
	virtual QString loadCollection(const Jid &AStreamJid, const IArchiveHeader &AHeader);
	virtual QString removeCollections(const Jid &AStreamJid, const IArchiveRequest &ARequest);
	virtual QString loadModifications(const Jid &AStreamJid, const QDateTime &AStart, int ACount);
	//IFileMessageArchive
	virtual QString archiveHomePath() const;
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
	void requestFailed(const QString &AId, const XmppError &AError);
	void collectionSaved(const QString &AId, const IArchiveHeader &AHeader);
	void headersLoaded(const QString &AId, const QList<IArchiveHeader> &AHeaders);
	void collectionLoaded(const QString &AId, const IArchiveCollection &ACollection);
	void collectionsRemoved(const QString &AId, const IArchiveRequest &ARequest);
	void modificationsLoaded(const QString &AId, const IArchiveModifications &AModifs);
	//IFileMessageArchive
	void fileCollectionOpened(const Jid &AStreamJid, const IArchiveHeader &AHeader);
	void fileCollectionSaved(const Jid &AStreamJid, const IArchiveHeader &AHeader);
	void fileCollectionRemoved(const Jid &AStreamJid, const IArchiveHeader &AHeader);
protected:
	void loadGatewayTypes();
	Jid gatewayJid(const Jid &AJid) const;
	IArchiveHeader makeHeader(const Jid &AItemJid, const Message &AMessage) const;
	bool checkCollectionFile(const QString &AFileName, const IArchiveRequest &ARequest) const;
	bool saveFileModification(const Jid &AStreamJid, const IArchiveHeader &AHeader, const QString &AAction) const;
protected:
	FileWriter *findFileWriter(const Jid &AStreamJid, const IArchiveHeader &AHeader) const;
	FileWriter *findFileWriter(const Jid &AStreamJid, const Jid &AWith, const QString &AThreadId) const;
	FileWriter *newFileWriter(const Jid &AStreamJid, const IArchiveHeader &AHeader, const QString &AFileName);
	void removeFileWriter(FileWriter *AWriter);
protected slots:
	void onFileTaskFinished(FileTask *ATask);
	void onArchivePrefsOpened(const Jid &AStreamJid);
	void onArchivePrefsClosed(const Jid &AStreamJid);
	void onFileWriterDestroyed(FileWriter *AWriter);
protected slots:
	void onOptionsOpened();
	void onOptionsClosed();
	void onDiscoInfoReceived(const IDiscoInfo &AInfo);
private:
	IPluginManager *FPluginManager;
	IMessageArchiver *FArchiver;
	IServiceDiscovery *FDiscovery;
private:
	mutable QReadWriteLock FThreadLock;
private:
	QString FArchiveHomePath;
	mutable QList<QString> FNewDirs;
	QMap<Jid, QString> FGatewayTypes;
	QMap<QString, FileWriter *> FWritingFiles;
	QMap<Jid, QMultiMap<Jid,FileWriter *> > FFileWriters;
};

#endif // FILEMESSAGEARCHIVE_H
