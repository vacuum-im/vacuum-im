#include "filemessagearchive.h"

#include <QDir>
#include <QUuid>
#include <QStringRef>
#include <QMutexLocker>
#include <QDirIterator>
#include <QMapIterator>
#include <QXmlStreamReader>
#include <definitions/optionvalues.h>
#include <definitions/internalerrors.h>
#include <definitions/archivecapabilityorders.h>
#include <definitions/filearchivedatabaseproperties.h>
#include <utils/options.h>
#include <utils/logger.h>

#define ARCHIVE_DIR_NAME      "archive"
#define COLLECTION_EXT        ".xml"
#define DATABASE_FILE_NAME    "filearchive.db"
#define GATEWAY_FILE_NAME     "gateways.dat"

#define CATEGORY_GATEWAY      "gateway"

FileMessageArchive::FileMessageArchive() : FMutex(QMutex::Recursive)
{
	FPluginManager = NULL;
	FArchiver = NULL;
	FDiscovery = NULL;
	FAccountManager = NULL;

	FFileWorker = new FileWorker(this);
	connect(FFileWorker,SIGNAL(taskFinished(FileTask *)),SLOT(onFileTaskFinished(FileTask *)));

	FDatabaseWorker = new DatabaseWorker(this);
	connect(FDatabaseWorker,SIGNAL(taskFinished(DatabaseTask *)),SLOT(onDatabaseTaskFinished(DatabaseTask *)));

	FDatabaseSyncWorker = new DatabaseSynchronizer(this,FDatabaseWorker,this);
	connect(FDatabaseSyncWorker,SIGNAL(syncFinished(const Jid &, bool)),SLOT(onDatabaseSyncFinished(const Jid &, bool)));

	qRegisterMetaType<FileTask *>("FileTask *");
}

FileMessageArchive::~FileMessageArchive()
{
	delete FDatabaseSyncWorker;
	delete FDatabaseWorker;
	delete FFileWorker;

	foreach(const QString &newDir, FNewDirs)
	{
		QDir dir(newDir);
		if (dir.entryList(QDir::NoDotAndDotDot).isEmpty())
		{
			QString name = dir.dirName();
			dir.cdUp();
			dir.rmdir(name);
		}
	}
}

void FileMessageArchive::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("File Message Archive");
	APluginInfo->description = tr("Allows to save the history of conversations in to local files");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(MESSAGEARCHIVER_UUID);
}

bool FileMessageArchive::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
	FPluginManager = APluginManager;

	IPlugin *plugin = APluginManager->pluginInterface("IMessageArchiver").value(0,NULL);
	if (plugin)
	{
		FArchiver = qobject_cast<IMessageArchiver *>(plugin->instance());
		if (FArchiver)
		{
			connect(FArchiver->instance(),SIGNAL(archivePrefsOpened(const Jid &)),SLOT(onArchivePrefsOpened(const Jid &)));
			connect(FArchiver->instance(),SIGNAL(archivePrefsClosed(const Jid &)),SLOT(onArchivePrefsClosed(const Jid &)));
		}
	}

	plugin = APluginManager->pluginInterface("IServiceDiscovery").value(0,NULL);
	if (plugin)
	{
		FDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());
		if (FDiscovery)
		{
			connect(FDiscovery->instance(),SIGNAL(discoInfoReceived(const IDiscoInfo &)),SLOT(onDiscoInfoReceived(const IDiscoInfo &)));
		}
	}

	plugin = APluginManager->pluginInterface("IAccountManager").value(0,NULL);
	if (plugin)
	{
		FAccountManager = qobject_cast<IAccountManager *>(plugin->instance());
		if (FAccountManager)
		{
			connect(FAccountManager->instance(),SIGNAL(shown(IAccount *)),SLOT(onAccountShown(IAccount *)));
			connect(FAccountManager->instance(),SIGNAL(hidden(IAccount *)),SLOT(onAccountHidden(IAccount *)));
		}
	}

	connect(Options::instance(),SIGNAL(optionsOpened()),SLOT(onOptionsOpened()));

	return FArchiver!=NULL;
}

bool FileMessageArchive::initObjects()
{
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_FILEARCHIVE_DATABASE_NOT_CREATED,tr("Failed to create database"));
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_FILEARCHIVE_DATABASE_NOT_OPENED,tr("Failed to open database"));
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_FILEARCHIVE_DATABASE_NOT_COMPATIBLE,tr("Database format is not compatible"));
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_FILEARCHIVE_DATABASE_EXEC_FAILED,tr("Failed to to execute SQL query"));

	FArchiveHomePath = FPluginManager->homePath();

	if (FArchiver)
		FArchiver->registerArchiveEngine(this);

	return true;
}

bool FileMessageArchive::initSettings()
{
	Options::setDefaultValue(OPV_FILEARCHIVE_HOMEPATH,QString());
	Options::setDefaultValue(OPV_FILEARCHIVE_FORCEDATABASESYNC,false);
	Options::setDefaultValue(OPV_FILEARCHIVE_COLLECTION_MINSIZE,1*1024);
	Options::setDefaultValue(OPV_FILEARCHIVE_COLLECTION_MAXSIZE,20*1024);
	Options::setDefaultValue(OPV_FILEARCHIVE_COLLECTION_CRITICALSIZE,25*1024);
	return true;
}

bool FileMessageArchive::startPlugin()
{
	FDatabaseWorker->start();
	return true;
}

QUuid FileMessageArchive::engineId() const
{
	return FILEMESSAGEARCHIVE_UUID;
}

QString FileMessageArchive::engineName() const
{
	return tr("Local File Archive");
}

QString FileMessageArchive::engineDescription() const
{
	return tr("History of conversations is stored in local files");
}

IOptionsWidget *FileMessageArchive::engineSettingsWidget(QWidget *AParent)
{
	return new FileArchiveOptions(FPluginManager,AParent);
}

quint32 FileMessageArchive::capabilities(const Jid &AStreamJid) const
{
	int caps = 0;
	if (AStreamJid.isValid())
	{
		caps = ArchiveManagement|FullTextSearch;
		if (FArchiver->isReady(AStreamJid))
			caps |= DirectArchiving|ManualArchiving;
		if (isDatabaseReady(AStreamJid))
			caps |= ArchiveReplication;
	}
	return caps;
}

bool FileMessageArchive::isCapable(const Jid &AStreamJid, quint32 ACapability) const
{
	return (capabilities(AStreamJid) & ACapability) == ACapability;
}

int FileMessageArchive::capabilityOrder(quint32 ACapability, const Jid &AStreamJid) const
{
	Q_UNUSED(AStreamJid);
	if (isCapable(AStreamJid,ACapability))
	{
		switch (ACapability)
		{
		case DirectArchiving:
			return ACO_DIRECT_FILEARCHIVE;
		case ManualArchiving:
			return ACO_MANUAL_FILEARCHIVE;
		case ArchiveManagement:
			return ACO_MANAGE_FILEARCHIVE;
		case ArchiveReplication:
			return ACO_REPLICATION_FILEARCHIVE;
		case FullTextSearch:
			return ACO_SEARCH_FILEARCHIVE;
		default:
			break;
		}
	}
	return -1;
}

bool FileMessageArchive::saveMessage(const Jid &AStreamJid, const Message &AMessage, bool ADirectionIn)
{
	bool written = false;
	if (isCapable(AStreamJid,DirectArchiving))
	{
		Jid itemJid = ADirectionIn ? AMessage.from() : AMessage.to();
		Jid with = AMessage.type()==Message::GroupChat ? itemJid.bare() : itemJid;

		QMutexLocker locker(&FMutex);
		FileWriter *writer = findFileWriter(AStreamJid,with,AMessage.threadId());
		if (!writer)
		{
			IArchiveHeader header = makeHeader(with,AMessage);
			QString filePath = collectionFilePath(AStreamJid,header.with,header.start);
			writer = newFileWriter(AStreamJid,header,filePath);
		}
		if (writer)
		{
			IArchiveItemPrefs prefs = FArchiver->archiveItemPrefs(AStreamJid,itemJid,AMessage.threadId());
			written = writer->writeMessage(AMessage,prefs.save,ADirectionIn);
		}
	}
	else
	{
		REPORT_ERROR("Failed to write message: Not capable");
	}
	return written;
}

bool FileMessageArchive::saveNote(const Jid &AStreamJid, const Message &AMessage, bool ADirectionIn)
{
	bool written = false;
	if (isCapable(AStreamJid,DirectArchiving))
	{
		Jid itemJid = ADirectionIn ? AMessage.from() : AMessage.to();
		Jid with = AMessage.type()==Message::GroupChat ? itemJid.bare() : itemJid;

		QMutexLocker locker(&FMutex);
		FileWriter *writer = findFileWriter(AStreamJid,with,AMessage.threadId());
		if (!writer)
		{
			IArchiveHeader header = makeHeader(with,AMessage);
			QString filePath = collectionFilePath(AStreamJid,header.with,header.start);
			writer = newFileWriter(AStreamJid,header,filePath);
		}
		if (writer)
		{
			written = writer->writeNote(AMessage.body());
		}
	}
	else
	{
		REPORT_ERROR("Failed to write note: Not capable");
	}
	return written;
}

QString FileMessageArchive::saveCollection(const Jid &AStreamJid, const IArchiveCollection &ACollection)
{
	if (isCapable(AStreamJid,ManualArchiving) && ACollection.header.with.isValid() && ACollection.header.start.isValid())
	{
		FileTaskSaveCollection *task = new FileTaskSaveCollection(this,AStreamJid,ACollection);
		if (FFileWorker->startTask(task))
		{
			LOG_STRM_DEBUG(AStreamJid,QString("Save collection task started, id=%1").arg(task->taskId()));
			return task->taskId();
		}
		else
		{
			LOG_STRM_WARNING(AStreamJid,QString("Failed to save collection with=%1: Task not started").arg(ACollection.header.with.full()));
		}
	}
	else if (!isCapable(AStreamJid,ManualArchiving))
	{
		LOG_STRM_ERROR(AStreamJid,QString("Failed to save collection with=%1: Not capable").arg(ACollection.header.with.full()));
	}
	else
	{
		REPORT_ERROR("Failed to save collection: Invalid params");
	}
	return QString::null;
}

QString FileMessageArchive::loadHeaders(const Jid &AStreamJid, const IArchiveRequest &ARequest)
{
	if (isCapable(AStreamJid,ArchiveManagement))
	{
		FileTaskLoadHeaders *task = new FileTaskLoadHeaders(this,AStreamJid,ARequest);
		if (FFileWorker->startTask(task))
		{
			LOG_STRM_DEBUG(AStreamJid,QString("Load headers task started, id=%1").arg(task->taskId()));
			return task->taskId();
		}
		else
		{
			LOG_STRM_WARNING(AStreamJid,"Failed to load headers: Task not started");
		}
	}
	else
	{
		LOG_STRM_ERROR(AStreamJid,"Failed to load headers: Not capable");
	}
	return QString::null;
}

QString FileMessageArchive::loadCollection(const Jid &AStreamJid, const IArchiveHeader &AHeader)
{
	if (isCapable(AStreamJid,ArchiveManagement) && AHeader.with.isValid() && AHeader.start.isValid())
	{
		FileTaskLoadCollection *task = new FileTaskLoadCollection(this,AStreamJid,AHeader);
		if (FFileWorker->startTask(task))
		{
			LOG_STRM_DEBUG(AStreamJid,QString("Load collection task started, id=%1").arg(task->taskId()));
			return task->taskId();
		}
		else
		{
			LOG_STRM_WARNING(AStreamJid,"Failed to load collection: Task not started");
		}
	}
	else if (!isCapable(AStreamJid,ArchiveManagement))
	{
		LOG_STRM_ERROR(AStreamJid,"Failed to load collection: Not capable");
	}
	else
	{
		REPORT_ERROR("Failed to load collection: Invalid params");
	}
	return QString::null;
}

QString FileMessageArchive::removeCollections(const Jid &AStreamJid, const IArchiveRequest &ARequest)
{
	if (isCapable(AStreamJid,ArchiveManagement))
	{
		FileTaskRemoveCollection *task = new FileTaskRemoveCollection(this,AStreamJid,ARequest);
		if (FFileWorker->startTask(task))
		{
			LOG_STRM_DEBUG(AStreamJid,QString("Remove collections task started, id=%1").arg(task->taskId()));
			return task->taskId();
		}
		else
		{
			LOG_STRM_WARNING(AStreamJid,"Failed to remove collections: Task not started");
		}
	}
	else
	{
		LOG_STRM_ERROR(AStreamJid,"Failed to remove collections: Not capable");
	}
	return QString::null;
}

QString FileMessageArchive::loadModifications(const Jid &AStreamJid, const QDateTime &AStart, int ACount, const QString &ANextRef)
{
	if (isCapable(AStreamJid,ArchiveReplication) && AStart.isValid() && ACount>0)
	{
		FileTaskLoadModifications *task = new FileTaskLoadModifications(this,AStreamJid,AStart,ACount,ANextRef);
		if (FFileWorker->startTask(task))
		{
			LOG_STRM_DEBUG(AStreamJid,QString("Load modifications task started, id=%1").arg(task->taskId()));
			return task->taskId();
		}
		else
		{
			LOG_STRM_WARNING(AStreamJid,"Failed to load modifications: Task not started");
		}
	}
	else if (!isCapable(AStreamJid,ArchiveReplication))
	{
		LOG_STRM_ERROR(AStreamJid,"Failed to load modifications: Not capable");
	}
	else
	{
		REPORT_ERROR("Failed to load modifications: Invalid params");
	}
	return QString::null;
}

QString FileMessageArchive::fileArchiveRootPath() const
{
	QMutexLocker locker(&FMutex);
	if (FArchiveRootPath.isEmpty())
	{
		QDir dir(FArchiveHomePath);
		dir.mkdir(ARCHIVE_DIR_NAME);
		if (!dir.cd(ARCHIVE_DIR_NAME))
			FArchiveRootPath = FArchiver->archiveDirPath();
		else
			FArchiveRootPath = dir.absolutePath();
	}
	return FArchiveRootPath;
}

QString FileMessageArchive::fileArchivePath(const Jid &AStreamJid) const
{
	if (AStreamJid.isValid())
	{
		QDir dir(fileArchiveRootPath());
		QString streamDir = Jid::encode(AStreamJid.pBare());
		if (dir.mkdir(streamDir))
		{
			QMutexLocker locker(&FMutex);
			FNewDirs.prepend(dir.absoluteFilePath(streamDir));
		}
		return dir.cd(streamDir) ? dir.absolutePath() : QString::null;
	}
	return QString::null;
}

QString FileMessageArchive::contactGateType(const Jid &AContactJid) const
{
	QMutexLocker locker(&FMutex);
	return FGatewayTypes.value(AContactJid.pDomain());
}

QString FileMessageArchive::collectionDirName(const Jid &AWith) const
{
	if (AWith.isValid())
	{
		Jid gateWith = gatewayJid(AWith);
		QString dirName = Jid::encode(gateWith.pBare());
		if (!gateWith.resource().isEmpty())
			dirName += "/" + Jid::encode(gateWith.pResource());
		return dirName;
	}
	return QString::null;
}

QString FileMessageArchive::collectionFileName(const QDateTime &AStart) const
{
	if (AStart.isValid())
	{
		// Remove msecs for correct file name comparation
		DateTime start(AStart.addMSecs(-AStart.time().msec()));
		return start.toX85UTC().replace(":","=") + COLLECTION_EXT;
	}
	return QString::null;
}

QString FileMessageArchive::collectionDirPath(const Jid &AStreamJid, const Jid &AWith) const
{
	if (AStreamJid.isValid() && AWith.isValid())
	{
		QDir dir(fileArchivePath(AStreamJid));
		QString withDir = collectionDirName(AWith);
		if (!dir.exists(withDir) && dir.mkpath(withDir))
		{
			QMutexLocker locker(&FMutex);
			QString path = dir.absolutePath();
			foreach(const QString &subDir, withDir.split("/"))
			{
				path += '/'+subDir;
				FNewDirs.prepend(path);
			}
		}
		return dir.cd(withDir) ? dir.absolutePath() : QString::null;
	}
	return QString::null;
}

QString FileMessageArchive::collectionFilePath(const Jid &AStreamJid, const Jid &AWith, const QDateTime &AStart) const
{
	if (AStreamJid.isValid() && AWith.isValid() && AStart.isValid())
	{
		QString fileName = collectionFileName(AStart);
		QString dirPath = collectionDirPath(AStreamJid,AWith);
		if (!dirPath.isEmpty() && !fileName.isEmpty())
			return dirPath+"/"+fileName;
	}
	return QString::null;
}

IArchiveHeader FileMessageArchive::loadFileHeader(const QString &AFilePath) const
{
	IArchiveHeader header;
	if (!AFilePath.isEmpty())
	{
		QMutexLocker locker(&FMutex);
		FileWriter *writer = FWritingFiles.value(AFilePath,NULL);
		if (writer == NULL)
		{
			QFile file(AFilePath);
			if (file.open(QFile::ReadOnly))
			{
				QXmlStreamReader reader(&file);
				while (!reader.atEnd())
				{
					reader.readNext();
					if (reader.isStartElement() && reader.qualifiedName()=="chat")
					{
						header.engineId = engineId();
						header.with = reader.attributes().value("with").toString();
						header.start = DateTime(reader.attributes().value("start").toString()).toLocal();
						header.subject = reader.attributes().value("subject").toString();
						header.threadId = reader.attributes().value("thread").toString();
						header.version = reader.attributes().value("version").toString().toInt();
						break;
					}
					else if (!reader.isStartDocument())
					{
						break;
					}
				}
			}
			else if (file.exists())
			{
				LOG_ERROR(QString("Failed to load file header from file=%1: %2").arg(file.fileName(),file.errorString()));
			}
		}
		else
		{
			header = writer->header();
		}
	}
	else
	{
		REPORT_ERROR("Failed to load file header: Invalid params");
	}
	return header;
}

IArchiveCollection FileMessageArchive::loadFileCollection(const Jid &AStreamJid, const IArchiveHeader &AHeader) const
{
	IArchiveCollection collection;
	if (AStreamJid.isValid() && AHeader.with.isValid() && AHeader.start.isValid())
	{
		QMutexLocker locker(&FMutex);
		QString filePath = collectionFilePath(AStreamJid,AHeader.with,AHeader.start);
		FileWriter *writer = FWritingFiles.value(filePath,NULL);
		if (writer==NULL || writer->recordsCount()>0)
		{
			QFile file(filePath);
			if (file.open(QFile::ReadOnly))
			{
				QDomDocument doc;
				doc.setContent(&file,true);
				FArchiver->elementToCollection(doc.documentElement(),collection);
				collection.header.engineId = engineId();
			}
			else if (file.exists())
			{
				LOG_ERROR(QString("Failed to load file collection from file=%1: %2").arg(file.fileName(),file.errorString()));
			}
		}
		else
		{
			collection.header = writer->header();
		}
	}
	else
	{
		REPORT_ERROR("Failed to load file collection: Invalid params");
	}
	return collection;
}

QList<IArchiveHeader> FileMessageArchive::loadFileHeaders(const Jid &AStreamJid, const IArchiveRequest &ARequest) const
{
	static const QString CollectionExt = COLLECTION_EXT;

	QList<IArchiveHeader> headers;
	if (AStreamJid.isValid())
	{
		QList<QString> dirPaths;
		QString streamPath = fileArchivePath(AStreamJid);
		if (!ARequest.with.isValid())
		{
			dirPaths.append(streamPath);
		}
		else if (ARequest.with.node().isEmpty() && !ARequest.exactmatch)
		{
			QString gateDomain = gatewayJid(ARequest.with).pDomain();
			QString encResource = Jid::encode(ARequest.with.pResource());
			
			// Check if gateway was saved as not gateway
			if (ARequest.with.pDomain() != gateDomain)
				dirPaths.append(collectionDirPath(AStreamJid,ARequest.with));

			QDirIterator dirIt(streamPath,QDir::Dirs|QDir::NoDotAndDotDot);
			while (dirIt.hasNext())
			{
				QString dirPath = dirIt.next();
				Jid bareJid = Jid::decode(dirIt.fileName());
				if (bareJid.pDomain() == gateDomain)
				{
					if (!encResource.isEmpty())
						dirPath += "/" + encResource;
					dirPaths.append(dirPath);
				}
			}
		}
		else
		{
			dirPaths.append(collectionDirPath(AStreamJid,ARequest.with));
		}

		QMultiMap<QString,IArchiveHeader> filesMap;
		QString startName = collectionFileName(ARequest.start);
		QString endName = collectionFileName(ARequest.end);
		QDirIterator::IteratorFlags flags = ARequest.with.isValid() && ARequest.exactmatch ? QDirIterator::NoIteratorFlags : QDirIterator::Subdirectories;
		for (int i=0; i<dirPaths.count(); i++)
		{
			QDirIterator dirIt(dirPaths.at(i),QDir::Files,flags);
			while (dirIt.hasNext())
			{
				QString fpath = dirIt.next();
				QString fname = dirIt.fileName();
				if (!ARequest.openOnly || FWritingFiles.contains(fpath))
				{
					if (fname.endsWith(CollectionExt) && (startName.isEmpty() || startName<=fname) && (endName.isEmpty() || endName>=fname))
					{
						IArchiveHeader header;
						if (checkRequestFile(fpath,ARequest,&header))
						{
							filesMap.insertMulti(fname,header);
							if ((quint32)filesMap.count() > ARequest.maxItems)
								filesMap.erase(ARequest.order==Qt::AscendingOrder ? --filesMap.end() : filesMap.begin());
						}
					}
				}
			}
		}

		QMapIterator<QString,IArchiveHeader> fileIt(filesMap);
		if (ARequest.order == Qt::DescendingOrder)
			fileIt.toBack();
		while (ARequest.order==Qt::AscendingOrder ? fileIt.hasNext() : fileIt.hasPrevious())
		{
			const IArchiveHeader &header = ARequest.order==Qt::AscendingOrder ? fileIt.next().value() : fileIt.previous().value();
			headers.append(header);
		}
	}
	else
	{
		REPORT_ERROR("Failed to load file headers: Invalid params");
	}
	
	return headers;
}

IArchiveHeader FileMessageArchive::saveFileCollection(const Jid &AStreamJid, const IArchiveCollection &ACollection)
{
	if (AStreamJid.isValid() && ACollection.header.with.isValid() && ACollection.header.start.isValid())
	{
		QMutexLocker locker(&FMutex);
		IArchiveCollection collection = loadFileCollection(AStreamJid,ACollection.header);

		// if collection already exist append new messages and notes
		if (collection.header == ACollection.header)
		{
			quint32 newVersion = collection.header.version+1;

			IArchiveCollectionBody newBody = collection.body;
			if (!ACollection.body.messages.isEmpty())
			{
				QMultiMap<int, QString> curMessages;
				foreach(const Message &message, collection.body.messages)
					curMessages.insertMulti(collection.header.start.secsTo(message.dateTime()),message.body());
					
				foreach(const Message &message, ACollection.body.messages)
					if (!curMessages.contains(collection.header.start.secsTo(message.dateTime()),message.body()))
						newBody.messages.append(message);

				qSort(newBody.messages);
			}
			if (!ACollection.body.notes.isEmpty())
			{
				for (QMultiMap<QDateTime, QString>::const_iterator it=ACollection.body.notes.constBegin(); it!=ACollection.body.notes.constEnd(); ++it)
					if (!collection.body.notes.contains(it.key(),it.value()))
						newBody.notes.insertMulti(it.key(),it.value());
			}
			
			collection = ACollection;
			collection.body = newBody;
			collection.header.version = newVersion;
		}
		else
		{
			collection = ACollection;
			collection.header.version = 0;
		}

		QFile file(collectionFilePath(AStreamJid,ACollection.header.with,ACollection.header.start));
		if (file.open(QFile::WriteOnly|QFile::Truncate))
		{
			QDomDocument doc;
			QDomElement chatElem = doc.appendChild(doc.createElement("chat")).toElement();
			FArchiver->collectionToElement(collection,chatElem,ARCHIVE_SAVE_MESSAGE);
			file.write(doc.toByteArray(2));
			file.flush();

			saveModification(AStreamJid,collection.header,IArchiveModification::Changed);
			return collection.header;
		}
		else
		{
			LOG_ERROR(QString("Failed to save file collection to file=%1: %2").arg(file.fileName(),file.errorString()));
		}
	}
	else
	{
		REPORT_ERROR("Failed to save file collection: Invalid params");
	}
	return IArchiveHeader();
}

bool FileMessageArchive::removeFileCollection(const Jid &AStreamJid, const IArchiveHeader &AHeader)
{
	if (AStreamJid.isValid() && AHeader.with.isValid() && AHeader.start.isValid())
	{
		QMutexLocker locker(&FMutex);
		QString filePath = collectionFilePath(AStreamJid,AHeader.with,AHeader.start);
		if (QFile::exists(filePath))
		{
			removeFileWriter(findFileWriter(AStreamJid,AHeader));
			if (QFile::remove(filePath))
			{
				saveModification(AStreamJid,AHeader,IArchiveModification::Removed);
				return true;
			}
			else
			{
				LOG_STRM_ERROR(AStreamJid,QString("Failed to remove file collection with=%1: File not removed").arg(AHeader.with.full()));
			}
		}
	}
	else
	{
		REPORT_ERROR("Failed to remove file collection: Invalid params");
	}
	return false;
}

bool FileMessageArchive::isDatabaseReady(const Jid &AStreamJid) const
{
	QDateTime lastSync = DateTime(databaseProperty(AStreamJid,FADP_LAST_SYNC_TIME)).toLocal();
	return lastSync.isValid();
}

QString FileMessageArchive::databaseArchiveFile(const Jid &AStreamJid) const
{
	QString archiveDir = AStreamJid.isValid() ? FArchiver->archiveDirPath(AStreamJid) : QString::null;
	if (!archiveDir.isEmpty())
		return archiveDir +"/"DATABASE_FILE_NAME;
	return QString::null;
}

QString FileMessageArchive::databaseProperty(const Jid &AStreamJid, const QString &AProperty) const
{
	QMutexLocker locker(&FMutex);
	return FDatabaseProperties.value(AStreamJid.bare()).value(AProperty);
}

bool FileMessageArchive::setDatabaseProperty(const Jid &AStreamJid, const QString &AProperty, const QString &AValue)
{
	QMutexLocker locker(&FMutex);

	bool changed = false;
	Jid bareStreamJid = AStreamJid.bare();
	if (FDatabaseProperties.contains(bareStreamJid))
	{
		QMap<QString,QString> &properties = FDatabaseProperties[bareStreamJid];
		if (properties.value(AProperty) != AValue)
		{
			DatabaseTaskSetProperty *task = new DatabaseTaskSetProperty(bareStreamJid,AProperty,AValue);
			if (FDatabaseWorker->execTask(task) && !task->isFailed())
			{
				LOG_STRM_DEBUG(AStreamJid,QString("Database property changed, property=%1, value=%2").arg(AProperty,AValue));
				changed = true;
				properties[AProperty] = AValue;
				emit databasePropertyChanged(bareStreamJid,AProperty); // TODO: emit signal from main thread
			}
			else if (task->isFailed())
			{
				LOG_STRM_ERROR(AStreamJid,QString("Failed to change database property=%1: %2").arg(AProperty,task->error().condition()));
			}
			else
			{
				LOG_STRM_WARNING(AStreamJid,QString("Failed to change database property=%1: Task not started").arg(AProperty));
			}
			delete task;
		}
		else
		{
			changed = true;
		}
	}
	else
	{
		REPORT_ERROR("Failed to set database property: Database not ready");
	}
	return changed;
}

QList<IArchiveHeader> FileMessageArchive::loadDatabaseHeaders(const Jid &AStreamJid, const IArchiveRequest &ARequest) const
{
	QList<IArchiveHeader> headers;
	if (isDatabaseReady(AStreamJid))
	{
		IArchiveRequest request = ARequest;
		request.maxItems = request.text.isEmpty() ? request.maxItems : 0xFFFFFFFF;

		DatabaseTaskLoadHeaders *task = new DatabaseTaskLoadHeaders(AStreamJid,request,contactGateType(ARequest.with));
		if (FDatabaseWorker->execTask(task) && !task->isFailed())
		{
			foreach(const IArchiveHeader &header, task->headers())
			{
				if ((quint32)headers.count() >= ARequest.maxItems)
					break;
				else if (ARequest.text.isEmpty())
					headers.append(header);
				else if (checkRequestFile(collectionFilePath(AStreamJid,header.with,header.start),ARequest))
					headers.append(header);
			}

			QMutexLocker locker(&FMutex);
			int dbHeadersCount = headers.count();
			foreach(FileWriter *writer, FFileWriters.value(AStreamJid).values())
			{
				if (writer->messagesCount()>0 && checkRequestHeader(writer->header(),ARequest))
				{
					if (ARequest.text.isEmpty())
						headers.append(writer->header());
					else if (checkRequestFile(writer->fileName(),ARequest))
						headers.append(writer->header());
				}
			}

			if (headers.count() > dbHeadersCount)
			{
				if (ARequest.order == Qt::AscendingOrder)
					qSort(headers.begin(),headers.end(),qLess<IArchiveHeader>());
				else
					qSort(headers.begin(),headers.end(),qGreater<IArchiveHeader>());

				if ((quint32)headers.count() > ARequest.maxItems)
					headers = headers.mid(0,ARequest.maxItems);
			}
		}
		else if (task->isFailed())
		{
			LOG_STRM_ERROR(AStreamJid,QString("Failed to load database headers: %1").arg(task->error().condition()));
		}
		else
		{
			LOG_STRM_WARNING(AStreamJid,QString("Failed to load database headers: Task not started"));
		}
		delete task;
	}
	else
	{
		REPORT_ERROR("Failed to load database headers: Database not ready");
	}
	return headers;
}

IArchiveModifications FileMessageArchive::loadDatabaseModifications(const Jid &AStreamJid, const QDateTime &AStart, int ACount, const QString &ANextRef) const
{
	IArchiveModifications modifs;
	if (isDatabaseReady(AStreamJid) && AStart.isValid() && ACount>0)
	{
		DatabaseTaskLoadModifications *task = new DatabaseTaskLoadModifications(AStreamJid,AStart,ACount,ANextRef);
		if (FDatabaseWorker->execTask(task) && !task->isFailed())
			modifs = task->modifications();
		else if (task->isFailed())
			LOG_STRM_ERROR(AStreamJid,QString("Failed to load database modifications: %1").arg(task->error().condition()));
		else
			LOG_STRM_WARNING(AStreamJid,QString("Failed to load database modifications: Task not started"));
		delete task;
	}
	else if (!isDatabaseReady(AStreamJid))
	{
		REPORT_ERROR("Failed to load database modification: Database not ready");
	}
	else
	{
		REPORT_ERROR("Failed to load database modification: Invalid params");
	}
	return modifs;
}

void FileMessageArchive::loadGatewayTypes()
{
	QMutexLocker locker(&FMutex);

	QDir dir(fileArchiveRootPath());
	QFile file(dir.absoluteFilePath(GATEWAY_FILE_NAME));
	if (file.open(QFile::ReadOnly|QFile::Text))
	{
		FGatewayTypes.clear();
		while (!file.atEnd())
		{
			QStringList gateway = QString::fromUtf8(file.readLine()).split(" ");
			if (!gateway.value(0).isEmpty() && !gateway.value(1).isEmpty())
				FGatewayTypes.insert(gateway.value(0),gateway.value(1));
		}
	}
	else if (file.exists())
	{
		REPORT_ERROR(QString("Failed to load gateway types from file: %1").arg(file.errorString()));
	}
}

Jid FileMessageArchive::gatewayJid(const Jid &AJid) const
{
	if (!AJid.node().isEmpty())
	{
		QString gateType = contactGateType(AJid);
		if (!gateType.isEmpty())
		{
			Jid jid = AJid;
			jid.setDomain(QString("%1.gateway").arg(gateType));
			return jid;
		}
	}
	return AJid;
}

void FileMessageArchive::saveGatewayType(const QString &ADomain, const QString &AType)
{
	QMutexLocker locker(&FMutex);

	QDir dir(fileArchiveRootPath());
	QFile file(dir.absoluteFilePath(GATEWAY_FILE_NAME));
	if (file.open(QFile::WriteOnly|QFile::Append|QFile::Text))
	{
		QStringList gateway;
		gateway << ADomain;
		gateway << AType;
		gateway << "\n";

		file.write(gateway.join(" ").toUtf8());
		file.flush();

		FGatewayTypes.insert(ADomain,AType);
	}
	else
	{
		REPORT_ERROR(QString("Failed to save gateway type to file: %1").arg(file.errorString()));
	}
}

bool FileMessageArchive::startDatabaseSync(const Jid &AStreamJid, bool AForce)
{
	if (FDatabaseProperties.contains(AStreamJid.bare()))
	{
		if (AForce)
		{
			LOG_STRM_INFO(AStreamJid,"Database synchronization started");
			FDatabaseSyncWorker->startSync(AStreamJid);
			return true;
		}
		if (!isDatabaseReady(AStreamJid))
		{
			LOG_STRM_INFO(AStreamJid,"Database synchronization started");
			FDatabaseSyncWorker->startSync(AStreamJid);
			return true;
		}
		if (Options::node(OPV_FILEARCHIVE_FORCEDATABASESYNC).value().toBool())
		{
			LOG_STRM_INFO(AStreamJid,"Database synchronization started");
			FDatabaseSyncWorker->startSync(AStreamJid);
			return true;
		}
	}
	return false;
}

IArchiveHeader FileMessageArchive::makeHeader(const Jid &AItemJid, const Message &AMessage) const
{
	IArchiveHeader header;
	header.engineId = engineId();
	header.with = AItemJid;
	if (!AMessage.dateTime().isValid() || AMessage.dateTime().secsTo(QDateTime::currentDateTime())>5)
		header.start = QDateTime::currentDateTime();
	else
		header.start = AMessage.dateTime();
	header.subject = AMessage.subject();
	header.threadId = AMessage.threadId();
	header.version = 0;
	return header;
}

bool FileMessageArchive::checkRequestHeader(const IArchiveHeader &AHeader, const IArchiveRequest &ARequest) const
{
	if (ARequest.start.isValid() && ARequest.start>AHeader.start)
		return false;

	if (ARequest.end.isValid() && ARequest.end<AHeader.start)
		return false;

	if (!ARequest.threadId.isEmpty() && ARequest.threadId!=AHeader.threadId)
		return false;

	if (ARequest.with.isValid() && ARequest.with!=AHeader.with)
	{
		if (!ARequest.exactmatch)
		{
			if (!ARequest.with.pNode().isEmpty() && ARequest.with.pNode()!=AHeader.with.pNode())
				return false;
			if (!ARequest.with.pResource().isEmpty() && ARequest.with.pResource()!=AHeader.with.pResource())
				return false;
			
			QString headerGate = contactGateType(AHeader.with);
			QString requestGate = contactGateType(ARequest.with);
			if (requestGate != headerGate)
				return false;
			if (requestGate.isEmpty() && ARequest.with.pDomain()!=AHeader.with.pDomain())
				return false;
		}
		else if (gatewayJid(ARequest.with) != gatewayJid(AHeader.with))
		{
			return false;
		}
	}

	return true;
}

bool FileMessageArchive::checkRequestFile(const QString &AFileName, const IArchiveRequest &ARequest, IArchiveHeader *AHeader) const
{
	QFile file(AFileName);
	if (file.open(QFile::ReadOnly))
	{
		QXmlStreamReader reader(&file);
		reader.setNamespaceProcessing(false);

		Qt::CheckState validState = Qt::PartiallyChecked;
		Qt::CheckState textState = ARequest.text.isEmpty() ? Qt::Checked : Qt::PartiallyChecked;
		Qt::CheckState threadState = ARequest.threadId.isEmpty() ? Qt::Checked : Qt::PartiallyChecked;

		QStringList elemStack;
		bool checkElemText = false;
		while (!reader.atEnd() && validState!=Qt::Unchecked && textState!=Qt::Unchecked && threadState!=Qt::Unchecked && 
			(validState==Qt::PartiallyChecked || textState==Qt::PartiallyChecked || threadState==Qt::PartiallyChecked))
		{
			reader.readNext();
			if (reader.isStartElement())
			{
				elemStack.append(reader.qualifiedName().toString().toLower());
				QString elemPath = elemStack.join("/");
				if (elemPath == "chat")
				{
					if (AHeader)
					{
						AHeader->engineId = engineId();
						AHeader->with = reader.attributes().value("with").toString();
						AHeader->start = DateTime(reader.attributes().value("start").toString()).toLocal();
						AHeader->subject = reader.attributes().value("subject").toString();
						AHeader->threadId = reader.attributes().value("thread").toString();
						AHeader->version = reader.attributes().value("version").toString().toInt();

						validState = AHeader->with.isValid() && AHeader->start.isValid() ? Qt::Checked : Qt::Unchecked;

						if (threadState == Qt::PartiallyChecked)
							threadState = AHeader->threadId==ARequest.threadId ? Qt::Checked : Qt::Unchecked;

						if (textState==Qt::PartiallyChecked && AHeader->subject.contains(ARequest.text,Qt::CaseInsensitive))
							textState = Qt::Checked;
					}
					else if (reader.attributes().value("with").isEmpty())
					{
						validState = Qt::Unchecked;
					}
					else if (reader.attributes().value("start").isEmpty())
					{
						validState = Qt::Unchecked;
					}
					else
					{
						validState = Qt::Checked;

						if (threadState == Qt::PartiallyChecked)
							threadState = reader.attributes().value("thread").compare(ARequest.threadId)==0 ? Qt::Checked : Qt::Unchecked;

						if (textState == Qt::PartiallyChecked)
						{
#if QT_VERSION >= QT_VERSION_CHECK(4,8,0)
							if (reader.attributes().value("subject").contains(ARequest.text,Qt::CaseInsensitive))
								textState = Qt::Checked;
#else
							if (reader.attributes().value("subject").toString().contains(ARequest.text,Qt::CaseInsensitive))
								textState = Qt::Checked;
#endif
						}
					}
				}
				else if (textState == Qt::PartiallyChecked)
				{
					checkElemText = elemPath=="chat/to/body" || elemPath=="chat/from/body" || elemPath=="chat/note";
				}
			}
			else if (reader.isCharacters())
			{
				if (checkElemText)
				{
#if QT_VERSION >= QT_VERSION_CHECK(4,8,0)
					if (reader.text().contains(ARequest.text,Qt::CaseInsensitive))
						textState = Qt::Checked;
#else
					if (reader.text().toString().contains(ARequest.text,Qt::CaseInsensitive))
						textState = Qt::Checked;
#endif
				}
			}
			else if (reader.isEndElement())
			{
				checkElemText = false;
				elemStack.removeLast();
			}
		}
		return validState==Qt::Checked && textState==Qt::Checked && threadState==Qt::Checked;
	}
	else if (file.exists())
	{
		REPORT_ERROR(QString("Failed to check file for history request: %1").arg(file.errorString()));
	}
	return false;
}

bool FileMessageArchive::saveModification(const Jid &AStreamJid, const IArchiveHeader &AHeader, IArchiveModification::ModifyAction AAction)
{
	bool saved = false;
	if (FDatabaseProperties.contains(AStreamJid.bare()) && AHeader.with.isValid() && AHeader.start.isValid())
	{
		if (AAction == IArchiveModification::Removed)
		{
			DatabaseTaskRemoveHeaders *task = new DatabaseTaskRemoveHeaders(AStreamJid,QList<IArchiveHeader>() << AHeader);
			if (FDatabaseWorker->execTask(task) && !task->isFailed())
				saved = true;
			else if (task->isFailed())
				LOG_STRM_ERROR(AStreamJid,QString("Failed to save modification: %1").arg(task->error().condition()));
			else
				LOG_STRM_WARNING(AStreamJid,QString("Failed to save modification: Task not started"));
			delete task;
		}
		else
		{
			DatabaseTaskUpdateHeaders *task = new DatabaseTaskUpdateHeaders(AStreamJid,QList<IArchiveHeader>() << AHeader,true,contactGateType(AHeader.with));
			if (FDatabaseWorker->execTask(task) && !task->isFailed())
				saved = true;
			else if (task->isFailed())
				LOG_STRM_ERROR(AStreamJid,QString("Failed to save modification: %1").arg(task->error().condition()));
			else
				LOG_STRM_WARNING(AStreamJid,QString("Failed to save modification: Task not started"));
			delete task;
		}
	}
	else if (!FDatabaseProperties.contains(AStreamJid.bare()))
	{
		REPORT_ERROR("Failed to save modification: Database not ready");
	}
	else
	{
		REPORT_ERROR("Failed to save modification: Invalid params");
	}

	if (AAction == IArchiveModification::Changed)
		emit fileCollectionChanged(AStreamJid,AHeader);
	else if (AAction == IArchiveModification::Removed)
		emit fileCollectionRemoved(AStreamJid,AHeader);

	return saved;
}

FileWriter *FileMessageArchive::findFileWriter(const Jid &AStreamJid, const IArchiveHeader &AHeader) const
{
	QMutexLocker locker(&FMutex);
	QList<FileWriter *> writers = FFileWriters.value(AStreamJid).values(AHeader.with);
	foreach(FileWriter *writer, writers)
		if (writer->header() == AHeader)
			return writer;
	return NULL;
}

FileWriter *FileMessageArchive::findFileWriter(const Jid &AStreamJid, const Jid &AWith, const QString &AThreadId) const
{
	QMutexLocker locker(&FMutex);
	QList<FileWriter *> writers = FFileWriters.value(AStreamJid).values(AWith);
	foreach(FileWriter *writer, writers)
		if (writer->header().threadId == AThreadId)
			return writer;
	return NULL;
}

FileWriter *FileMessageArchive::newFileWriter(const Jid &AStreamJid, const IArchiveHeader &AHeader, const QString &AFileName)
{
	QMutexLocker locker(&FMutex);
	if (AStreamJid.isValid() && AHeader.with.isValid() && AHeader.start.isValid() && !AFileName.isEmpty() && !FWritingFiles.contains(AFileName))
	{
		FileWriter *writer = new FileWriter(AStreamJid,AFileName,AHeader,this);
		if (writer->isOpened())
		{
			LOG_STRM_DEBUG(AStreamJid,QString("Creating file writer with=%1").arg(AHeader.with.full()));
			FWritingFiles.insert(writer->fileName(),writer);
			FFileWriters[AStreamJid].insert(AHeader.with,writer);
			connect(writer,SIGNAL(writerDestroyed(FileWriter *)),SLOT(onFileWriterDestroyed(FileWriter *)));
		}
		else
		{
			delete writer;
			writer = NULL;
			REPORT_ERROR("Failed to create file writer: Writer not opened");
		}
		return writer;
	}
	else if (FWritingFiles.contains(AFileName))
	{
		REPORT_ERROR("Failed to create file writer: File already exists");
	}
	else
	{
		REPORT_ERROR("Failed to create file writer: Invalid params");
	}
	return NULL;
}

void FileMessageArchive::removeFileWriter(FileWriter *AWriter)
{
	QMutexLocker locker(&FMutex);
	if (AWriter && FWritingFiles.contains(AWriter->fileName()))
	{
		LOG_STRM_DEBUG(AWriter->streamJid(),QString("Destroying file writer with=%1").arg(AWriter->header().with.full()));
		AWriter->closeAndDeleteLater();
		FWritingFiles.remove(AWriter->fileName());
		FFileWriters[AWriter->streamJid()].remove(AWriter->header().with,AWriter);
		if (AWriter->messagesCount() > 0)
			saveModification(AWriter->streamJid(),AWriter->header(),IArchiveModification::Changed);
		else
			QFile::remove(AWriter->fileName());
	}
}

void FileMessageArchive::onFileTaskFinished(FileTask *ATask)
{
	if (!ATask->isFailed())
	{
		LOG_STRM_DEBUG(ATask->streamJid(),QString("File task finished, type=%1, id=%2").arg(ATask->type()).arg(ATask->taskId()));
		switch (ATask->type())
		{
		case FileTask::SaveCollection:
			emit collectionSaved(ATask->taskId(),static_cast<FileTaskSaveCollection *>(ATask)->archiveCollection());
			break;
		case FileTask::LoadHeaders:
			emit headersLoaded(ATask->taskId(),static_cast<FileTaskLoadHeaders *>(ATask)->archiveHeaders());
			break;
		case FileTask::LoadCollection:
			emit collectionLoaded(ATask->taskId(),static_cast<FileTaskLoadCollection *>(ATask)->archiveCollection());
			break;
		case FileTask::RemoveCollections:
			emit collectionsRemoved(ATask->taskId(),static_cast<FileTaskRemoveCollection *>(ATask)->archiveRequest());
			break;
		case FileTask::LoadModifications:
			emit modificationsLoaded(ATask->taskId(),static_cast<FileTaskLoadModifications *>(ATask)->archiveModifications());
			break;
		}
	}
	else
	{
		LOG_STRM_ERROR(ATask->streamJid(),QString("Failed to execute file task, type=%1, id=%2: %3").arg(ATask->type()).arg(ATask->taskId(),ATask->error().condition()));
		emit requestFailed(ATask->taskId(),ATask->error());
	}
	delete ATask;
}

void FileMessageArchive::onDatabaseTaskFinished(DatabaseTask *ATask)
{
	if (!ATask->isFailed())
	{
		LOG_STRM_DEBUG(ATask->streamJid(),QString("Database task finished, type=%1 id=%2").arg(ATask->type()).arg(ATask->taskId()));
		switch(ATask->type())
		{
		case DatabaseTask::OpenDatabase:
			{
				QMutexLocker locker(&FMutex);
				DatabaseTaskOpenDatabase *task = static_cast<DatabaseTaskOpenDatabase *>(ATask);
				FPluginManager->continueShutdown();
				FDatabaseProperties.insert(task->streamJid(),task->databaseProperties());
				emit databaseOpened(task->streamJid());
				
				startDatabaseSync(task->streamJid(),databaseProperty(task->streamJid(),FADP_DATABASE_NOT_CLOSED)!="false");
				setDatabaseProperty(task->streamJid(),FADP_DATABASE_NOT_CLOSED,"true");
			}
			break;
		case DatabaseTask::CloseDatabase:
			{
				QMutexLocker locker(&FMutex);
				DatabaseTaskCloseDatabase *task = static_cast<DatabaseTaskCloseDatabase *>(ATask);
				FPluginManager->continueShutdown();
				FDatabaseProperties.remove(task->streamJid());
				FDatabaseSyncWorker->removeSync(task->streamJid());
				emit databaseClosed(task->streamJid());
			}
			break;
		default:
			break;
		}
	}
	else
	{
		LOG_STRM_ERROR(ATask->streamJid(),QString("Failed to execute database task, type=%1, id=%2: %3").arg(ATask->type()).arg(ATask->taskId(),ATask->error().condition()));
		emit requestFailed(ATask->taskId(),ATask->error());
	}
	delete ATask;
}

void FileMessageArchive::onArchivePrefsOpened(const Jid &AStreamJid)
{
	emit capabilitiesChanged(AStreamJid);
}

void FileMessageArchive::onArchivePrefsClosed(const Jid &AStreamJid)
{
	QMutexLocker locker(&FMutex);
	foreach(FileWriter *writer, FFileWriters.value(AStreamJid).values())
		removeFileWriter(writer);
	emit capabilitiesChanged(AStreamJid);
}

void FileMessageArchive::onFileWriterDestroyed(FileWriter *AWriter)
{
	removeFileWriter(AWriter);
}

void FileMessageArchive::onDatabaseSyncFinished(const Jid &AStreamJid, bool AFailed)
{
	if (!AFailed)
	{
		LOG_STRM_INFO(AStreamJid,"Database synchronization finished");
		quint32 caps = capabilities(AStreamJid);
		setDatabaseProperty(AStreamJid,FADP_LAST_SYNC_TIME,DateTime(QDateTime::currentDateTime()).toX85UTC());
		if (caps != capabilities(AStreamJid))
			emit capabilitiesChanged(AStreamJid);
	}
	else
	{
		LOG_STRM_ERROR(AStreamJid,"Failed to synchronize database");
	}
}

void FileMessageArchive::onOptionsOpened()
{
	FArchiveRootPath = QString::null;
	FArchiveHomePath = Options::node(OPV_FILEARCHIVE_HOMEPATH).value().toString();
	if (!FArchiveHomePath.isEmpty())
	{
		QDir dir(FArchiveHomePath);
		if (!dir.exists() && !dir.mkpath(FArchiveHomePath))
			FArchiveHomePath = FPluginManager->homePath();
	}
	else
	{
		FArchiveHomePath = FPluginManager->homePath();
	}
	loadGatewayTypes();
}

void FileMessageArchive::onOptionsClosed()
{
	FArchiveRootPath = QString::null;
	FArchiveHomePath = FPluginManager->homePath();
}

void FileMessageArchive::onAccountShown(IAccount *AAccount)
{
	Jid bareStreamJid = AAccount->streamJid().bare();
	if (!FDatabaseProperties.contains(bareStreamJid))
	{
		DatabaseTaskOpenDatabase *task = new DatabaseTaskOpenDatabase(bareStreamJid,databaseArchiveFile(bareStreamJid));
		if (FDatabaseWorker->startTask(task))
		{
			LOG_STRM_DEBUG(AAccount->streamJid(),QString("Database open task started, id=%1").arg(task->taskId()));
			FPluginManager->delayShutdown();
		}
		else
		{
			LOG_STRM_WARNING(AAccount->streamJid(),"Failed to open database: Task not started");
		}
	}
}

void FileMessageArchive::onAccountHidden(IAccount *AAccount)
{
	Jid bareStreamJid = AAccount->streamJid().bare();
	if (FDatabaseProperties.contains(bareStreamJid))
	{
		emit databaseAboutToClose(bareStreamJid);
		setDatabaseProperty(bareStreamJid,FADP_DATABASE_NOT_CLOSED,"false");
		DatabaseTaskCloseDatabase *task = new DatabaseTaskCloseDatabase(bareStreamJid);
		if (FDatabaseWorker->startTask(task))
		{
			LOG_STRM_DEBUG(AAccount->streamJid(),QString("Database close task started, id=%1").arg(task->taskId()));
			FPluginManager->delayShutdown();
		}
		else
		{
			LOG_STRM_WARNING(AAccount->streamJid(),"Failed to close database: Task not started");
		}
	}
}

void FileMessageArchive::onDiscoInfoReceived(const IDiscoInfo &AInfo)
{
	if (AInfo.node.isEmpty() && AInfo.contactJid.node().isEmpty() && AInfo.contactJid.resource().isEmpty() && !FGatewayTypes.contains(AInfo.contactJid.pDomain()))
	{
		foreach(const IDiscoIdentity &identity, AInfo.identity)
		{
			if (identity.category==CATEGORY_GATEWAY && !identity.type.isEmpty())
			{
				saveGatewayType(AInfo.contactJid.pDomain(),identity.type);
				break;
			}
		}
	}
}

Q_EXPORT_PLUGIN2(plg_filemessagearchive, FileMessageArchive)
