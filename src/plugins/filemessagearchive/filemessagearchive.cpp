#include "filemessagearchive.h"

#include <QDir>
#include <QStringRef>
#include <QMutexLocker>
#include <QDirIterator>
#include <QMapIterator>
#include <QXmlStreamReader>
#include <definitions/internalerrors.h>
#include <definitions/filearchivedatabaseproperties.h>
#include "filearchiveoptions.h"

#define ARCHIVE_DIR_NAME      "archive"
#define COLLECTION_EXT        ".xml"
#define DB_FILE_NAME          "archive.db"
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

	foreach(QString newDir, FNewDirs)
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
	{
		FArchiver->registerArchiveEngine(this);
	}
	return true;
}

bool FileMessageArchive::initSettings()
{
	Options::setDefaultValue(OPV_FILEARCHIVE_HOMEPATH,QString());
	Options::setDefaultValue(OPV_FILEARCHIVE_FORCEDATABASESYNC,false);
	Options::setDefaultValue(OPV_FILEARCHIVE_COLLECTION_MINMESSAGES,5);
	Options::setDefaultValue(OPV_FILEARCHIVE_COLLECTION_SIZE,20*1024);
	Options::setDefaultValue(OPV_FILEARCHIVE_COLLECTION_MAXSIZE,30*1024);
	Options::setDefaultValue(OPV_FILEARCHIVE_COLLECTION_TIMEOUT,20*60*1000);
	Options::setDefaultValue(OPV_FILEARCHIVE_COLLECTION_MINTIMEOUT,5*60*1000);
	Options::setDefaultValue(OPV_FILEARCHIVE_COLLECTION_MAXTIMEOUT,120*60*1000);
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
		caps = ArchiveManagement|TextSearch;
		if (FArchiver->isReady(AStreamJid))
			caps |= DirectArchiving|ManualArchiving;
		if (isDatabaseReady(AStreamJid))
			caps |= Replication;
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
		case Replication:
			return ACO_REPLICATION_FILEARCHIVE;
		case TextSearch:
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
	return written;
}

QString FileMessageArchive::saveCollection(const Jid &AStreamJid, const IArchiveCollection &ACollection)
{
	if (isCapable(AStreamJid,ManualArchiving) && ACollection.header.with.isValid() && ACollection.header.start.isValid())
	{
		QString saveMode = FArchiver->archiveItemPrefs(AStreamJid,ACollection.header.with,ACollection.header.threadId).save;
		FileTaskSaveCollection *task = new FileTaskSaveCollection(this,AStreamJid,ACollection,saveMode);
		FFileWorker->startTask(task);
		return task->taskId();
	}
	return QString::null;
}

QString FileMessageArchive::loadHeaders(const Jid &AStreamJid, const IArchiveRequest &ARequest)
{
	if (isCapable(AStreamJid,ArchiveManagement))
	{
		FileTaskLoadHeaders *task = new FileTaskLoadHeaders(this,AStreamJid,ARequest);
		FFileWorker->startTask(task);
		return task->taskId();
	}
	return QString::null;
}

QString FileMessageArchive::loadCollection(const Jid &AStreamJid, const IArchiveHeader &AHeader)
{
	if (isCapable(AStreamJid,ArchiveManagement))
	{
		FileTaskLoadCollection *task = new FileTaskLoadCollection(this,AStreamJid,AHeader);
		FFileWorker->startTask(task);
		return task->taskId();
	}
	return QString::null;
}

QString FileMessageArchive::removeCollections(const Jid &AStreamJid, const IArchiveRequest &ARequest)
{
	if (isCapable(AStreamJid,ArchiveManagement))
	{
		FileTaskRemoveCollection *task = new FileTaskRemoveCollection(this,AStreamJid,ARequest);
		FFileWorker->startTask(task);
		return task->taskId();
	}
	return QString::null;
}

QString FileMessageArchive::loadModifications(const Jid &AStreamJid, const QDateTime &AStart, int ACount)
{
	if (isCapable(AStreamJid,Replication))
	{
		FileTaskLoadModifications *task = new FileTaskLoadModifications(this,AStreamJid,AStart,ACount);
		FFileWorker->startTask(task);
		return task->taskId();
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
		{
			dir.setPath(FPluginManager->homePath());
			dir.mkdir(ARCHIVE_DIR_NAME);
			FArchiveRootPath = dir.absoluteFilePath(ARCHIVE_DIR_NAME);
		}
		else
		{
			FArchiveRootPath = dir.absolutePath();
		}
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
		if (dir.cd(streamDir))
			return dir.absolutePath();
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
			foreach(QString subDir, withDir.split("/"))
			{
				path += '/'+subDir;
				FNewDirs.prepend(path);
			}
		}
		if (dir.cd(withDir))
			return dir.absolutePath();
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
	QMutexLocker locker(&FMutex);

	IArchiveHeader header;
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
			file.close();
		}
	}
	else
	{
		header = writer->header();
	}
	return header;
}

IArchiveCollection FileMessageArchive::loadFileCollection(const QString &AFilePath) const
{
	QMutexLocker locker(&FMutex);

	IArchiveCollection collection;
	FileWriter *writer = FWritingFiles.value(AFilePath,NULL);
	if (writer==NULL || writer->recordsCount()>0)
	{
		QFile file(AFilePath);
		if (file.open(QFile::ReadOnly))
		{
			QDomDocument doc;
			doc.setContent(file.readAll(),true);
			FArchiver->elementToCollection(doc.documentElement(),collection);
			collection.header.engineId = engineId();
			file.close();
		}
	}
	else
	{
		collection.header = writer->header();
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

		QMultiMap<QString,QString> filesMap;
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
				if (fname.endsWith(CollectionExt) && (startName.isEmpty() || startName<=fname) && (endName.isEmpty() || endName>=fname))
				{
					filesMap.insertMulti(fname,fpath);
					if (ARequest.maxItems>0 && filesMap.count()>ARequest.maxItems)
						filesMap.erase(ARequest.order==Qt::AscendingOrder ? --filesMap.end() : filesMap.begin());
				}
			}
		}

		IArchiveHeader header;
		QMapIterator<QString,QString> fileIt(filesMap);
		if (ARequest.order == Qt::DescendingOrder)
			fileIt.toBack();
		while (ARequest.order==Qt::AscendingOrder ? fileIt.hasNext() : fileIt.hasPrevious())
		{
			QString fpath = ARequest.order==Qt::AscendingOrder ? fileIt.next().value() : fileIt.previous().value();
			if (checkRequesFile(fpath,ARequest,&header))
				headers.append(header);
		}
	}
	
	return headers;
}

bool FileMessageArchive::saveFileCollection(const Jid &AStreamJid, const IArchiveCollection &ACollection, const QString &ASaveMode, bool AAppend)
{
	if (AStreamJid.isValid() && ACollection.header.with.isValid() && ACollection.header.start.isValid())
	{
		QMutexLocker locker(&FMutex);

		QString filePath = collectionFilePath(AStreamJid,ACollection.header.with,ACollection.header.start);
		IArchiveCollection collection = loadFileCollection(filePath);
		
		IArchiveModification::ModifyAction logAction = ACollection.header==collection.header ? IArchiveModification::Modified : IArchiveModification::Created;
		collection.header = ACollection.header;

		if (AAppend)
		{
			if (!ACollection.body.messages.isEmpty())
			{
				QMultiMap<QDateTime, QString> curMessages;
				foreach(Message message, collection.body.messages)
					curMessages.insertMulti(message.dateTime(),message.body());
					
				foreach(Message message, ACollection.body.messages)
					if (!curMessages.contains(message.dateTime(),message.body()))
						collection.body.messages.append(message);

				qSort(collection.body.messages);
			}
			if (!ACollection.body.notes.isEmpty())
			{
				for (QMultiMap<QDateTime, QString>::const_iterator it= ACollection.body.notes.constBegin(); it!=ACollection.body.notes.constEnd(); ++it)
					if (!collection.body.notes.contains(it.key(),it.value()))
						collection.body.notes.insertMulti(it.key(),it.value());
			}
		}
		else
		{
			collection.body.messages = ACollection.body.messages;
			collection.body.notes = ACollection.body.notes;
		}

		QFile file(filePath);
		if (file.open(QFile::WriteOnly|QFile::Truncate))
		{
			QDomDocument doc;
			QDomElement chatElem = doc.appendChild(doc.createElement("chat")).toElement();
			FArchiver->collectionToElement(collection,chatElem,ASaveMode);
			file.write(doc.toByteArray(2));
			file.close();

			saveModification(AStreamJid,collection.header,logAction);
			return true;
		}
	}
	return false;
}

bool FileMessageArchive::removeFileCollection(const Jid &AStreamJid, const Jid &AWith, const QDateTime &AStart)
{
	QMutexLocker locker(&FMutex);
	QString filePath = collectionFilePath(AStreamJid,AWith,AStart);
	if (QFile::exists(filePath))
	{
		IArchiveHeader header = loadFileHeader(filePath);
		removeFileWriter(findFileWriter(AStreamJid,header));
		if (QFile::remove(filePath))
		{
			saveModification(AStreamJid,header,IArchiveModification::Removed);
			return true;
		}
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
	if (AStreamJid.isValid())
	{
		QDir dir(FPluginManager->homePath());
		dir.mkdir(ARCHIVE_DIR_NAME);
		if (dir.cd(ARCHIVE_DIR_NAME))
		{
			QString streamDir = collectionDirName(AStreamJid.bare());
			if (dir.mkdir(streamDir))
			{
				QMutexLocker locker(&FMutex);
				FNewDirs.prepend(dir.absoluteFilePath(streamDir));
			}
			if (dir.cd(streamDir))
			{
				return dir.absoluteFilePath(DB_FILE_NAME);
			}
		}
	}
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
				changed = true;
				properties[AProperty] = AValue;
				emit databasePropertyChanged(bareStreamJid,AProperty); // TODO: emit signal from main thread
			}
			delete task;
		}
		else
		{
			changed = true;
		}
	}
	return changed;
}

QList<IArchiveHeader> FileMessageArchive::loadDatabaseHeaders(const Jid &AStreamJid, const IArchiveRequest &ARequest) const
{
	QList<IArchiveHeader> headers;
	if (isDatabaseReady(AStreamJid))
	{
		DatabaseTaskLoadHeaders *task = new DatabaseTaskLoadHeaders(AStreamJid,ARequest,contactGateType(ARequest.with));
		if (FDatabaseWorker->execTask(task) && !task->isFailed())
		{
			foreach(const IArchiveHeader &header, task->headers())
			{
				if (ARequest.text.isEmpty())
					headers.append(header);
				else if (checkRequesFile(collectionFilePath(AStreamJid,header.with,header.start),ARequest))
					headers.append(header);
			}
			
			QMutexLocker locker(&FMutex);
			foreach(FileWriter *writer, FFileWriters.value(AStreamJid).values())
			{
				if (writer->messagesCount()>0 && checkRequestHeader(writer->header(),ARequest))
				{
					if (ARequest.text.isEmpty())
						headers.append(writer->header());
					else if (checkRequesFile(writer->fileName(),ARequest))
						headers.append(writer->header());
				}
			}
		}
		delete task;
	}
	return headers;
}

IArchiveModifications FileMessageArchive::loadDatabaseModifications(const Jid &AStreamJid, const QDateTime &AStart, int ACount) const
{
	IArchiveModifications modifs;
	if (isDatabaseReady(AStreamJid))
	{
		DatabaseTaskLoadModifications *task = new DatabaseTaskLoadModifications(AStreamJid,AStart,ACount);
		if (FDatabaseWorker->execTask(task) && !task->isFailed())
			modifs = task->modifications();
		delete task;
	}
	return modifs;
}

void FileMessageArchive::loadGatewayTypes()
{
	QMutexLocker locker(&FMutex);

	QDir dir(fileArchiveRootPath());
	QFile gateways(dir.absoluteFilePath(GATEWAY_FILE_NAME));
	if (gateways.open(QFile::ReadOnly|QFile::Text))
	{
		FGatewayTypes.clear();
		while (!gateways.atEnd())
		{
			QStringList gateway = QString::fromUtf8(gateways.readLine()).split(" ");
			if (!gateway.value(0).isEmpty() && !gateway.value(1).isEmpty())
				FGatewayTypes.insert(gateway.value(0),gateway.value(1));
		}
		gateways.close();
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
	QFile gateways(dir.absoluteFilePath(GATEWAY_FILE_NAME));
	if (gateways.open(QFile::WriteOnly|QFile::Append|QFile::Text))
	{
		QStringList gateway;
		gateway << ADomain;
		gateway << AType;
		gateway << "\n";
		gateways.write(gateway.join(" ").toUtf8());
		gateways.close();
		FGatewayTypes.insert(ADomain,AType);
	}
}

bool FileMessageArchive::startDatabaseSync(const Jid &AStreamJid, bool AForce)
{
	if (FDatabaseProperties.contains(AStreamJid.bare()))
	{
		if (AForce)
		{
			FDatabaseSyncWorker->startSync(AStreamJid);
			return true;
		}
		if (!isDatabaseReady(AStreamJid))
		{
			FDatabaseSyncWorker->startSync(AStreamJid);
			return true;
		}
		if (Options::node(OPV_FILEARCHIVE_FORCEDATABASESYNC).value().toBool())
		{
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
		else
		{
			return false;
		}
	}

	return true;
}

bool FileMessageArchive::checkRequesFile(const QString &AFileName, const IArchiveRequest &ARequest, IArchiveHeader *AHeader) const
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
		file.close();
		return validState==Qt::Checked && textState==Qt::Checked && threadState==Qt::Checked;
	}
	return false;
}

bool FileMessageArchive::saveModification(const Jid &AStreamJid, const IArchiveHeader &AHeader, IArchiveModification::ModifyAction AAction)
{
	bool saved = false;
	if (FDatabaseProperties.contains(AStreamJid.bare()))
	{
		if (AAction == IArchiveModification::Removed)
		{
			DatabaseTaskRemoveHeaders *task = new DatabaseTaskRemoveHeaders(AStreamJid,QList<IArchiveHeader>() << AHeader);
			if (FDatabaseWorker->execTask(task) && !task->isFailed())
				saved = true;
			delete task;
		}
		else
		{
			DatabaseTaskUpdateHeaders *task = new DatabaseTaskUpdateHeaders(AStreamJid,QList<IArchiveHeader>() << AHeader,true,contactGateType(AHeader.with));
			if (FDatabaseWorker->execTask(task) && !task->isFailed())
				saved = true;
			delete task;
		}
	}
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
	if (AHeader.with.isValid() && AHeader.start.isValid() && !AFileName.isEmpty() && !FWritingFiles.contains(AFileName))
	{
		FileWriter *writer = new FileWriter(AStreamJid,AFileName,AHeader,this);
		if (writer->isOpened())
		{
			FWritingFiles.insert(writer->fileName(),writer);
			FFileWriters[AStreamJid].insert(AHeader.with,writer);
			connect(writer,SIGNAL(writerDestroyed(FileWriter *)),SLOT(onFileWriterDestroyed(FileWriter *)));
		}
		else
		{
			delete writer;
			writer = NULL;
		}
		return writer;
	}
	return NULL;
}

void FileMessageArchive::removeFileWriter(FileWriter *AWriter)
{
	QMutexLocker locker(&FMutex);
	if (AWriter && FWritingFiles.contains(AWriter->fileName()))
	{
		AWriter->closeAndDeleteLater();
		FWritingFiles.remove(AWriter->fileName());
		FFileWriters[AWriter->streamJid()].remove(AWriter->header().with,AWriter);
		if (AWriter->messagesCount() > 0)
			saveModification(AWriter->streamJid(),AWriter->header(),IArchiveModification::Created);
		else
			QFile::remove(AWriter->fileName());
	}
}

void FileMessageArchive::onFileTaskFinished(FileTask *ATask)
{
	if (!ATask->isFailed())
	{
		switch (ATask->type())
		{
		case FileTask::SaveCollection:
			emit collectionSaved(ATask->taskId(),static_cast<FileTaskSaveCollection *>(ATask)->archiveHeader());
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
		emit requestFailed(ATask->taskId(),ATask->error());
	}
	delete ATask;
}

void FileMessageArchive::onDatabaseTaskFinished(DatabaseTask *ATask)
{
	if (!ATask->isFailed())
	{
		switch(ATask->type())
		{
		case DatabaseTask::OpenDatabase:
			{
				QMutexLocker locker(&FMutex);
				DatabaseTaskOpenDatabase *task = static_cast<DatabaseTaskOpenDatabase *>(ATask);
				FPluginManager->delayShutdown();
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
		}
	}
	else
	{
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
		int caps = capabilities(AStreamJid);
		setDatabaseProperty(AStreamJid,FADP_LAST_SYNC_TIME,DateTime(QDateTime::currentDateTime()).toX85UTC());
		if (caps != capabilities(AStreamJid))
			emit capabilitiesChanged(AStreamJid);
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
		FDatabaseWorker->startTask(task);
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
		FDatabaseWorker->startTask(task);
	}
}

void FileMessageArchive::onDiscoInfoReceived(const IDiscoInfo &AInfo)
{
	if (AInfo.node.isEmpty() && AInfo.contactJid.node().isEmpty() && AInfo.contactJid.resource().isEmpty() && !FGatewayTypes.contains(AInfo.contactJid.pDomain()))
	{
		foreach(IDiscoIdentity identity, AInfo.identity)
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
