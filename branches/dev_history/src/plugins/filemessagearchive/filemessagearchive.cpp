#include "filemessagearchive.h"

#define ARCHIVE_DIR_NAME      "archive"
#define COLLECTION_EXT        ".xml"
#define LOG_FILE_NAME         "archive.dat"

#define LOG_ACTION_CREATE     "C"
#define LOG_ACTION_MODIFY     "M"
#define LOG_ACTION_REMOVE     "R"

#include <QDir>
#include <QDirIterator>
#include <QXmlStreamReader>

#include "workingthread.h"

FileMessageArchive::FileMessageArchive()
{
	FPluginManager = NULL;
	FArchiver = NULL;
}

FileMessageArchive::~FileMessageArchive()
{

}

void FileMessageArchive::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("File Message Archive");
	APluginInfo->description = tr("Allows to save the history of communications in local files");
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

	return FArchiver!=NULL;
}

bool FileMessageArchive::initObjects()
{
	if (FArchiver)
	{
		FArchiver->registerArchiveEngine(this);
	}
	return true;
}

bool FileMessageArchive::initSettings()
{
	Options::setDefaultValue(OPV_FILEARCHIVE_COLLECTION_MINMESSAGES,5);
	Options::setDefaultValue(OPV_FILEARCHIVE_COLLECTION_SIZE,20*1024);
	Options::setDefaultValue(OPV_FILEARCHIVE_COLLECTION_MAXSIZE,30*1024);
	Options::setDefaultValue(OPV_FILEARCHIVE_COLLECTION_TIMEOUT,20*60*1000);
	Options::setDefaultValue(OPV_FILEARCHIVE_COLLECTION_MINTIMEOUT,5*60*1000);
	Options::setDefaultValue(OPV_FILEARCHIVE_COLLECTION_MAXTIMEOUT,120*60*1000);
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
	return tr("History of communications is stored in local files");
}

quint32 FileMessageArchive::capabilities(const Jid &AStreamJid) const
{
	if (AStreamJid.isValid() && !FArchiver->isReady(AStreamJid))
		return ArchiveManagement|Replication|TextSearch;
	return DirectArchiving|ManualArchiving|ArchiveManagement|Replication|TextSearch;
}

bool FileMessageArchive::isCapable(const Jid &AStreamJid, uint ACapability) const
{
	return (capabilities(AStreamJid) & ACapability) > 0;
}

int FileMessageArchive::capabilityOrder(quint32 ACapability, const Jid &AStreamJid) const
{
	Q_UNUSED(AStreamJid);
	switch (ACapability)
	{
	case DirectArchiving:
		return ACO_DIRECT_FILEARCHIVE;
	case ManualArchiving:
		return ACO_MANUAL_FILEARCHIVE;
	case AutomaticArchiving:
		return ACO_AUTOMATIC_FILEARCHIVE;
	case ArchiveManagement:
		return ACO_MANAGE_FILEARCHIVE;
	case Replication:
		return ACO_REPLICATION_FILEARCHIVE;
	case TextSearch:
		return ACO_SEARCH_FILEARCHIVE;
	default:
		return 0;
	}
}

bool FileMessageArchive::saveMessage(const Jid &AStreamJid, const Message &AMessage, bool ADirectionIn)
{
	if (isCapable(AStreamJid,DirectArchiving) && FArchiver->isReady(AStreamJid))
	{
		Jid itemJid = ADirectionIn ? AMessage.from() : AMessage.to();
		Jid with = AMessage.type()==Message::GroupChat ? itemJid.bare() : itemJid;

		CollectionWriter *writer = findCollectionWriter(AStreamJid,with,AMessage.threadId());
		if (!writer)
			writer = getCollectionWriter(AStreamJid,makeHeader(with,AMessage));
		if (writer)
		{
			IArchiveItemPrefs prefs = FArchiver->archiveItemPrefs(AStreamJid,itemJid,AMessage.threadId());
			return writer->writeMessage(AMessage,prefs.save,ADirectionIn);
		}
	}
	return false;
}

bool FileMessageArchive::saveNote(const Jid &AStreamJid, const Message &AMessage, bool ADirectionIn)
{
	if (isCapable(AStreamJid,DirectArchiving) && FArchiver->isReady(AStreamJid))
	{
		Jid itemJid = ADirectionIn ? AMessage.from() : AMessage.to();
		Jid with = AMessage.type()==Message::GroupChat ? itemJid.bare() : itemJid;

		CollectionWriter *writer = findCollectionWriter(AStreamJid,with,AMessage.threadId());
		if (!writer)
			writer = getCollectionWriter(AStreamJid,makeHeader(with,AMessage));
		if (writer)
			return writer->writeNote(AMessage.body());
	}
	return false;
}

QString FileMessageArchive::saveCollection(const Jid &AStreamJid, const IArchiveCollection &ACollection)
{
	if (isCapable(AStreamJid,ManualArchiving) && AStreamJid.isValid() && ACollection.header.with.isValid() && ACollection.header.start.isValid())
	{
		WorkingThread *wthread = new WorkingThread(this,FArchiver,this);
		wthread->setStreamJid(AStreamJid);
		wthread->setArchiveCollection(ACollection);
		return wthread->executeAction(WorkingThread::SaveCollection);
	}
	return QString::null;
}

QString FileMessageArchive::removeCollections(const Jid &AStreamJid, const IArchiveRequest &ARequest, bool AOpened)
{
	Q_UNUSED(AOpened);
	if (AStreamJid.isValid() && isCapable(AStreamJid,ManualArchiving))
	{
		WorkingThread *wthread = new WorkingThread(this,FArchiver,this);
		wthread->setStreamJid(AStreamJid);
		wthread->setArchiveRequest(ARequest);
		return wthread->executeAction(WorkingThread::RemoveCollection);
	}
	return QString::null;
}

QString FileMessageArchive::loadHeaders(const Jid &AStreamJid, const IArchiveRequest &ARequest, const QString &AAfter)
{
	Q_UNUSED(AAfter);
	if (AStreamJid.isValid() && isCapable(AStreamJid,ArchiveManagement))
	{
		WorkingThread *wthread = new WorkingThread(this,FArchiver,this);
		wthread->setStreamJid(AStreamJid);
		wthread->setArchiveRequest(ARequest);
		return wthread->executeAction(WorkingThread::LoadHeaders);
	}
	return QString::null;
}

QString FileMessageArchive::loadCollection(const Jid &AStreamJid, const IArchiveHeader &AHeader, const QString &AAfter)
{
	Q_UNUSED(AAfter);
	if (AStreamJid.isValid() && isCapable(AStreamJid,ArchiveManagement))
	{
		WorkingThread *wthread = new WorkingThread(this,FArchiver,this);
		wthread->setStreamJid(AStreamJid);
		wthread->setArchiveHeader(AHeader);
		return wthread->executeAction(WorkingThread::LoadCollection);
	}
	return QString::null;
}

QString FileMessageArchive::loadModifications(const Jid &AStreamJid, const QDateTime &AStart, int ACount, const QString &AAfter)
{
	Q_UNUSED(AAfter);
	if (AStreamJid.isValid() && isCapable(AStreamJid,Replication))
	{
		WorkingThread *wthread = new WorkingThread(this,FArchiver,this);
		wthread->setStreamJid(AStreamJid);
		wthread->setModificationsStart(AStart);
		wthread->setModificationsCount(ACount);
		return wthread->executeAction(WorkingThread::LoadModifications);
	}
	return QString::null;
}

QString FileMessageArchive::collectionDirName(const Jid &AWith) const
{
	Jid jid = AWith; //FArchiver->gateJid(AWith);
	QString dirName = Jid::encode(jid.pBare());
	if (!jid.resource().isEmpty())
		dirName += "/"+Jid::encode(jid.pResource());
	return dirName;
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
	bool noError = true;

	QDir dir(FPluginManager->homePath());
	if (!dir.exists(ARCHIVE_DIR_NAME))
		noError &= dir.mkdir(ARCHIVE_DIR_NAME);
	noError &= dir.cd(ARCHIVE_DIR_NAME);

	if (noError && AStreamJid.isValid())
	{
		QString streamDir = collectionDirName(AStreamJid.bare());
		if (!dir.exists(streamDir))
			noError &= dir.mkdir(streamDir);
		noError &= dir.cd(streamDir);

		if (noError && AWith.isValid())
		{
			QString withDir = collectionDirName(AWith);
			if (!dir.exists(withDir))
				noError &= dir.mkpath(withDir);
			noError &= dir.cd(withDir);
		}
	}

	return noError ? dir.path() : QString::null;
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

QStringList FileMessageArchive::findCollectionFiles(const Jid &AStreamJid, const IArchiveRequest &ARequest) const
{
	static const QString CollectionExt = COLLECTION_EXT;

	QStringList files;
	if (AStreamJid.isValid())
	{
		QMultiMap<QString,QString> filesMap;
		QString startName = collectionFileName(ARequest.start);
		QString endName = collectionFileName(ARequest.end);
		QDirIterator dirIt(collectionDirPath(AStreamJid,ARequest.with),QDir::Files,QDirIterator::Subdirectories);
		while (dirIt.hasNext())
		{
			QString fpath = dirIt.next();
			QString fname = dirIt.fileName();
			if (fname.endsWith(CollectionExt) && (startName.isEmpty() || startName<=fname) && (endName.isEmpty() || endName>=fname))
			{
				filesMap.insertMulti(fname,fpath);
				if (ARequest.count>0 && filesMap.count()>ARequest.count)
					filesMap.erase(ARequest.order==Qt::AscendingOrder ? --filesMap.end() : filesMap.begin());
			}
		}

		QMapIterator<QString,QString> fileIt(filesMap);
		if (ARequest.order == Qt::DescendingOrder)
			fileIt.toBack();
		while (ARequest.order==Qt::AscendingOrder ? fileIt.hasNext() : fileIt.hasPrevious())
		{
			if (ARequest.order == Qt::AscendingOrder)
				fileIt.next();
			else
				fileIt.previous();
			files.append(fileIt.value());
		}
	}
	return files;
}

IArchiveHeader FileMessageArchive::loadHeaderFromFile(const QString &AFileName) const
{
	IArchiveHeader header;
	QFile file(AFileName);
	if (file.open(QFile::ReadOnly))
	{
		QXmlStreamReader reader(&file);
		while (!reader.atEnd())
		{
			reader.readNext();
			if (reader.isStartElement() && reader.qualifiedName()=="chat")
			{
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
	return header;
}

IArchiveCollection FileMessageArchive::loadCollectionFromFile(const QString &AFileName) const
{
	IArchiveCollection collection;
	QFile file(AFileName);
	if (file.open(QFile::ReadOnly))
	{
		QDomDocument doc;
		doc.setContent(file.readAll(),true);
		FArchiver->elementToCollection(doc.documentElement(),collection);
		file.close();
	}
	return collection;
}

IArchiveModifications FileMessageArchive::loadFileModifications(const Jid &AStreamJid, const QDateTime &AStart, int ACount) const
{
	IArchiveModifications modifs;
	modifs.startTime = AStart.toUTC();

	QString dirPath = collectionDirPath(AStreamJid,Jid::null);
	if (!dirPath.isEmpty() && AStreamJid.isValid() && AStart.isValid())
	{
		QFile log(dirPath+"/"LOG_FILE_NAME);
		if (log.open(QFile::ReadOnly|QIODevice::Text))
		{
			qint64 sbound = 0;
			qint64 ebound = log.size();
			while (ebound - sbound > 1024)
			{
				log.seek((ebound + sbound)/2);
				log.readLine();
				DateTime logTime = QString::fromUtf8(log.readLine()).split(" ").value(0);
				if (!logTime.isValid())
					ebound = sbound;
				else if (logTime.toLocal() > AStart)
					ebound = log.pos();
				else
					sbound = log.pos();
			}
			log.seek(sbound);

			while (!log.atEnd() && modifs.items.count()<ACount)
			{
				QString logLine = QString::fromUtf8(log.readLine());
				QStringList logFields = logLine.split(" ",QString::KeepEmptyParts);
				if (logFields.count() >= 6)
				{
					DateTime logTime = logFields.at(0);
					if (logTime.toLocal() > AStart)
					{
						IArchiveModification modif;
						modif.header.with = logFields.at(2);
						modif.header.start = DateTime(logFields.at(3)).toLocal();
						modif.header.version = logFields.at(4).toInt();
						modifs.endTime = logTime;
						if (logFields.at(1) == LOG_ACTION_CREATE)
						{
							modif.action = IArchiveModification::Created;
							modifs.items.append(modif);
						}
						else if (logFields.at(1) == LOG_ACTION_MODIFY)
						{
							modif.action = IArchiveModification::Modified;
							modifs.items.append(modif);
						}
						else if (logFields.at(1) == LOG_ACTION_REMOVE)
						{
							modif.action = IArchiveModification::Removed;
							modifs.items.append(modif);
						}
					}
				}
			}
		}
	}
	return modifs;
}

bool FileMessageArchive::saveCollectionToFile(const Jid &AStreamJid, const IArchiveCollection &ACollection, const QString &ASaveMode, bool AAppend)
{
	if (AStreamJid.isValid() && ACollection.header.with.isValid() && ACollection.header.start.isValid())
	{
		QString fileName = collectionFilePath(AStreamJid,ACollection.header.with,ACollection.header.start);
		IArchiveCollection collection = loadCollectionFromFile(fileName);
		
		QString logAction = ACollection.header==collection.header ? LOG_ACTION_MODIFY : LOG_ACTION_CREATE;
		collection.header = ACollection.header;

		if (AAppend)
		{
			// TODO: Find message duplicates
			if (!ACollection.messages.isEmpty())
			{
				collection.messages += ACollection.messages;
				qSort(collection.messages);
			}
			if (!ACollection.notes.isEmpty())
			{
				collection.notes += ACollection.notes;
			}
		}
		else
		{
			collection.messages = ACollection.messages;
			collection.notes = ACollection.notes;
		}

		QFile file(fileName);
		if (file.open(QFile::WriteOnly|QFile::Truncate))
		{
			QDomDocument doc;
			QDomElement chatElem = doc.appendChild(doc.createElement("chat")).toElement();
			FArchiver->collectionToElement(collection,chatElem,ASaveMode);
			file.write(doc.toByteArray(2));
			file.close();
			saveFileModification(AStreamJid,collection.header,logAction);
			emit fileCollectionSaved(AStreamJid,collection.header);
			return true;
		}
	}
	return false;
}

bool FileMessageArchive::removeCollectionFile(const Jid &AStreamJid, const Jid &AWith, const QDateTime &AStart)
{
	QString fileName = collectionFilePath(AStreamJid,AWith,AStart);
	if (QFile::exists(fileName))
	{
		IArchiveHeader header = loadHeaderFromFile(fileName);
		delete findCollectionWriter(AStreamJid,header);
		if (QFile::remove(collectionFilePath(AStreamJid,AWith,AStart)))
		{
			saveFileModification(AStreamJid,header,LOG_ACTION_REMOVE);
			emit fileCollectionRemoved(AStreamJid,header);
			return true;
		}
	}
	return false;
}

IArchiveHeader FileMessageArchive::makeHeader(const Jid &AItemJid, const Message &AMessage) const
{
	IArchiveHeader header;
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

bool FileMessageArchive::saveFileModification(const Jid &AStreamJid, const IArchiveHeader &AHeader, const QString &AAction) const
{
	QString dirPath = collectionDirPath(AStreamJid,Jid::null);
	if (!dirPath.isEmpty() && AStreamJid.isValid() && AHeader.with.isValid() && AHeader.start.isValid())
	{
		QFile log(dirPath+"/"LOG_FILE_NAME);
		if (log.open(QFile::WriteOnly|QFile::Append|QIODevice::Text))
		{
			QStringList logFields;
			logFields << DateTime(QDateTime::currentDateTime()).toX85UTC();
			logFields << AAction;
			logFields << AHeader.with.eFull();
			logFields << DateTime(AHeader.start).toX85UTC();
			logFields << QString::number(AHeader.version);
			logFields << "\n";
			log.write(logFields.join(" ").toUtf8());
			log.close();
			return true;
		}
	}
	return false;
}

CollectionWriter *FileMessageArchive::findCollectionWriter(const Jid &AStreamJid, const IArchiveHeader &AHeader) const
{
	QList<CollectionWriter *> writers = FCollectionWriters.value(AStreamJid).values(AHeader.with);
	foreach(CollectionWriter *writer, writers)
		if (writer->header() == AHeader)
			return writer;
	return NULL;
}

CollectionWriter *FileMessageArchive::findCollectionWriter(const Jid &AStreamJid, const Jid &AWith, const QString &AThreadId) const
{
	QList<CollectionWriter *> writers = FCollectionWriters.value(AStreamJid).values(AWith);
	foreach(CollectionWriter *writer, writers)
		if (writer->header().threadId == AThreadId)
			return writer;
	return NULL;
}

CollectionWriter *FileMessageArchive::getCollectionWriter(const Jid &AStreamJid, const IArchiveHeader &AHeader)
{
	CollectionWriter *writer = findCollectionWriter(AStreamJid,AHeader);
	if (!writer && AHeader.with.isValid() && AHeader.start.isValid())
	{
		QString fileName = collectionFilePath(AStreamJid,AHeader.with,AHeader.start);
		writer = new CollectionWriter(AStreamJid,fileName,AHeader,this);
		if (writer->isOpened())
		{
			FCollectionWriters[AStreamJid].insert(AHeader.with,writer);
			connect(writer,SIGNAL(writerDestroyed(CollectionWriter *)),SLOT(onCollectionWriterDestroyed(CollectionWriter *)));
			emit fileCollectionOpened(AStreamJid,AHeader);
		}
		else
		{
			delete writer;
			writer = NULL;
		}
	}
	return writer;
}

void FileMessageArchive::onWorkingThreadFinished()
{
	WorkingThread *wthread = qobject_cast<WorkingThread *>(sender());
	if (wthread)
	{
		if (!wthread->hasError())
		{
			switch (wthread->workAction())
			{
			case WorkingThread::SaveCollection:
				emit collectionSaved(wthread->workId(),wthread->archiveHeader());
				break;
			case WorkingThread::RemoveCollection:
				emit collectionsRemoved(wthread->workId(),wthread->archiveRequest());
				break;
			case WorkingThread::LoadHeaders:
				emit headersLoaded(wthread->workId(),wthread->archiveHeaders(),wthread->archiveResultSet());
				break;
			case WorkingThread::LoadCollection:
				emit collectionLoaded(wthread->workId(),wthread->archiveCollection(),wthread->archiveResultSet());
				break;
			case WorkingThread::LoadModifications:
				emit modificationsLoaded(wthread->workId(),wthread->archiveModifications(),wthread->archiveResultSet());
				break;
			default:
				emit requestFailed(wthread->workId(),tr("Internal error"));
			}
		}
		else
		{
			emit requestFailed(wthread->workId(),wthread->errorString());
		}
		wthread->deleteLater();
	}
}

void FileMessageArchive::onArchivePrefsOpened(const Jid &AStreamJid)
{
	emit capabilitiesChanged(AStreamJid);
}

void FileMessageArchive::onArchivePrefsClosed(const Jid &AStreamJid)
{
	foreach(Jid streamJid, FCollectionWriters.keys())
		qDeleteAll(FCollectionWriters.take(streamJid));
	emit capabilitiesChanged(AStreamJid);
}

void FileMessageArchive::onCollectionWriterDestroyed(CollectionWriter *AWriter)
{
	FCollectionWriters[AWriter->streamJid()].remove(AWriter->header().with,AWriter);
	if (AWriter->recordsCount() > 0)
	{
		saveFileModification(AWriter->streamJid(),AWriter->header(),LOG_ACTION_CREATE);
		emit fileCollectionSaved(AWriter->streamJid(),AWriter->header());
	}
}

Q_EXPORT_PLUGIN2(plg_filemessagearchive, FileMessageArchive)
