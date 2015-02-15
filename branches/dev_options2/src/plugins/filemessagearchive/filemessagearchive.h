#ifndef FILEMESSAGEARCHIVE_H
#define FILEMESSAGEARCHIVE_H

#include <QMutex>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ifilemessagearchive.h>
#include <interfaces/imessagearchiver.h>
#include <interfaces/iservicediscovery.h>
#include <interfaces/iaccountmanager.h>
#include "filewriter.h"
#include "fileworker.h"
#include "databaseworker.h"
#include "databasesynchronizer.h"
#include "filearchiveoptionswidget.h"

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
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin();
	//IArchiveEngine
	virtual QUuid engineId() const;
	virtual QString engineName() const;
	virtual QString engineDescription() const;
	virtual IOptionsDialogWidget *engineSettingsWidget(QWidget *AParent);
	virtual quint32 capabilities(const Jid &AStreamJid = Jid::null) const;
	virtual bool isCapable(const Jid &AStreamJid, quint32 ACapability) const;
	virtual int capabilityOrder(quint32 ACapability, const Jid &AStreamJid = Jid::null) const;
	virtual bool saveMessage(const Jid &AStreamJid, const Message &AMessage, bool ADirectionIn);
	virtual bool saveNote(const Jid &AStreamJid, const Message &AMessage, bool ADirectionIn);
	virtual QString saveCollection(const Jid &AStreamJid, const IArchiveCollection &ACollection);
	virtual QString loadHeaders(const Jid &AStreamJid, const IArchiveRequest &ARequest);
	virtual QString loadCollection(const Jid &AStreamJid, const IArchiveHeader &AHeader);
	virtual QString removeCollections(const Jid &AStreamJid, const IArchiveRequest &ARequest);
	virtual QString loadModifications(const Jid &AStreamJid, const QDateTime &AStart, int ACount, const QString &ANextRef);
	//IFileMessageArchive
	virtual QString fileArchiveRootPath() const;
	virtual QString fileArchivePath(const Jid &AStreamJid) const;
	virtual QString contactGateType(const Jid &AContactJid) const;
	virtual QString collectionDirName(const Jid &AWith) const;
	virtual QString collectionFileName(const QDateTime &AStart) const;
	virtual QString collectionDirPath(const Jid &AStreamJid, const Jid &AWith) const;
	virtual QString collectionFilePath(const Jid &AStreamJid, const Jid &AWith, const QDateTime &AStart) const;
	virtual IArchiveHeader loadFileHeader(const QString &AFilePath) const;
	virtual IArchiveCollection loadFileCollection(const Jid &AStreamJid, const IArchiveHeader &AHeader) const;
	virtual QList<IArchiveHeader> loadFileHeaders(const Jid &AStreamJid, const IArchiveRequest &ARequest) const;
	virtual IArchiveHeader saveFileCollection(const Jid &AStreamJid, const IArchiveCollection &ACollection);
	virtual bool removeFileCollection(const Jid &AStreamJid, const IArchiveHeader &AHeader);
	// Database
	virtual bool isDatabaseReady(const Jid &AStreamJid) const;
	virtual QString databaseArchiveFile(const Jid &AStreamJid) const;
	virtual QString databaseProperty(const Jid &AStreamJid, const QString &AProperty) const;
	virtual bool setDatabaseProperty(const Jid &AStreamJid, const QString &AProperty, const QString &AValue);
	virtual QList<IArchiveHeader> loadDatabaseHeaders(const Jid &AStreamJid, const IArchiveRequest &ARequest) const;
	virtual IArchiveModifications loadDatabaseModifications(const Jid &AStreamJid, const QDateTime &AStart, int ACount, const QString &ANextRef) const;
signals:
	//IArchiveEngine
	void capabilitiesChanged(const Jid &AStreamJid);
	void requestFailed(const QString &AId, const XmppError &AError);
	void headersLoaded(const QString &AId, const QList<IArchiveHeader> &AHeaders);
	void collectionSaved(const QString &AId, const IArchiveCollection &ACollection);
	void collectionLoaded(const QString &AId, const IArchiveCollection &ACollection);
	void collectionsRemoved(const QString &AId, const IArchiveRequest &ARequest);
	void modificationsLoaded(const QString &AId, const IArchiveModifications &AModifications);
	//IFileMessageArchive
	void databaseOpened(const Jid &AStreamJid);
	void databaseAboutToClose(const Jid &AStreamJid);
	void databaseClosed(const Jid &AStreamJid);
	void databasePropertyChanged(const Jid &AStreamJid, const QString &AProperty);
	void fileCollectionChanged(const Jid &AStreamJid, const IArchiveHeader &AHeader);
	void fileCollectionRemoved(const Jid &AStreamJid, const IArchiveHeader &AHeader);
protected:
	void loadGatewayTypes();
	Jid gatewayJid(const Jid &AJid) const;
	void saveGatewayType(const QString &ADomain, const QString &AType);
	bool startDatabaseSync(const Jid &AStreamJid, bool AForce = false);
	IArchiveHeader makeHeader(const Jid &AItemJid, const Message &AMessage) const;
	bool checkRequestHeader(const IArchiveHeader &AHeader, const IArchiveRequest &ARequest) const;
	bool checkRequestFile(const QString &AFileName, const IArchiveRequest &ARequest, IArchiveHeader *AHeader=NULL) const;
	bool saveModification(const Jid &AStreamJid, const IArchiveHeader &AHeader, IArchiveModification::ModifyAction AAction);
protected:
	FileWriter *findFileWriter(const Jid &AStreamJid, const IArchiveHeader &AHeader) const;
	FileWriter *findFileWriter(const Jid &AStreamJid, const Jid &AWith, const QString &AThreadId) const;
	FileWriter *newFileWriter(const Jid &AStreamJid, const IArchiveHeader &AHeader, const QString &AFileName);
	void removeFileWriter(FileWriter *AWriter);
protected slots:
	void onFileTaskFinished(FileTask *ATask);
	void onDatabaseTaskFinished(DatabaseTask *ATask);
	void onArchivePrefsOpened(const Jid &AStreamJid);
	void onArchivePrefsClosed(const Jid &AStreamJid);
	void onFileWriterDestroyed(FileWriter *AWriter);
	void onDatabaseSyncFinished(const Jid &AStreamJid, bool AFailed);
protected slots:
	void onOptionsOpened();
	void onOptionsClosed();
	void onAccountShown(IAccount *AAccount);
	void onAccountHidden(IAccount *AAccount);
	void onDiscoInfoReceived(const IDiscoInfo &AInfo);
private:
	IPluginManager *FPluginManager;
	IMessageArchiver *FArchiver;
	IServiceDiscovery *FDiscovery;
	IAccountManager *FAccountManager;
private:
	mutable QMutex FMutex;
	FileWorker *FFileWorker;
	DatabaseWorker *FDatabaseWorker;
	DatabaseSynchronizer *FDatabaseSyncWorker;
private:
	QString FArchiveHomePath;
	mutable QString FArchiveRootPath;
	mutable QList<QString> FNewDirs;
	QMap<QString,QString> FGatewayTypes;
	QMap<QString,FileWriter *> FWritingFiles;
	QMap<Jid, QMultiMap<Jid,FileWriter *> > FFileWriters;
	QMap<Jid, QMap<QString,QString> > FDatabaseProperties;
};

#endif // FILEMESSAGEARCHIVE_H
