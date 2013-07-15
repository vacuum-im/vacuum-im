#include "filemessagearchive.h"

#include <QDir>
#include <QStringRef>
#include <QDirIterator>
#include <QXmlStreamReader>
#include "workingthread.h"

#define ARCHIVE_DIR_NAME      "archive"
#define COLLECTION_EXT        ".xml"
#define LOG_FILE_NAME         "archive.dat"
#define GATEWAY_FILE_NAME     "gateways.dat"

#define CATEGORY_GATEWAY      "gateway"

#define LOG_ACTION_CREATE     "C"
#define LOG_ACTION_MODIFY     "M"
#define LOG_ACTION_REMOVE     "R"

FileMessageArchive::FileMessageArchive()
{
	FPluginManager = NULL;
	FArchiver = NULL;
	FDiscovery = NULL;
}

FileMessageArchive::~FileMessageArchive()
{
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

	connect(Options::instance(),SIGNAL(optionsOpened()),SLOT(onOptionsOpened()));

	return FArchiver!=NULL;
}

bool FileMessageArchive::initObjects()
{
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
	return tr("History of conversations is stored in local files");
}

IOptionsWidget *FileMessageArchive::engineSettingsWidget(QWidget *AParent)
{
	return new FileArchiveOptions(FPluginManager,AParent);
}

quint32 FileMessageArchive::capabilities(const Jid &AStreamJid) const
{
	if (FArchiver->isReady(AStreamJid))
		return DirectArchiving|ManualArchiving|ArchiveManagement|Replication|TextSearch;
	else if (AStreamJid.isValid())
		return ArchiveManagement|Replication|TextSearch;
	return 0;
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

		FThreadLock.lockForWrite();
		CollectionWriter *writer = findCollectionWriter(AStreamJid,with,AMessage.threadId());
		if (!writer)
		{
			FThreadLock.unlock();
			IArchiveHeader header = makeHeader(with,AMessage);
			QString fileName = collectionFilePath(AStreamJid,header.with,header.start);
			FThreadLock.lockForWrite();
			writer = newCollectionWriter(AStreamJid,header,fileName);
		}
		if (writer)
		{
			IArchiveItemPrefs prefs = FArchiver->archiveItemPrefs(AStreamJid,itemJid,AMessage.threadId());
			written = writer->writeMessage(AMessage,prefs.save,ADirectionIn);
		}
		FThreadLock.unlock();
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

		FThreadLock.lockForWrite();
		CollectionWriter *writer = findCollectionWriter(AStreamJid,with,AMessage.threadId());
		if (!writer)
		{
			FThreadLock.unlock();
			IArchiveHeader header = makeHeader(with,AMessage);
			QString fileName = collectionFilePath(AStreamJid,header.with,header.start);
			FThreadLock.lockForWrite();
			writer = newCollectionWriter(AStreamJid,header,fileName);
		}
		if (writer)
		{
			written = writer->writeNote(AMessage.body());
		}
		FThreadLock.unlock();
	}
	return written;
}

QString FileMessageArchive::saveCollection(const Jid &AStreamJid, const IArchiveCollection &ACollection)
{
	if (isCapable(AStreamJid,ManualArchiving) && ACollection.header.with.isValid() && ACollection.header.start.isValid())
	{
		WorkingThread *wthread = new WorkingThread(this,FArchiver,this);
		wthread->setStreamJid(AStreamJid);
		wthread->setArchiveCollection(ACollection);
		connect(wthread,SIGNAL(finished()),SLOT(onWorkingThreadFinished()));
		return wthread->executeAction(WorkingThread::SaveCollection);
	}
	return QString::null;
}

QString FileMessageArchive::loadHeaders(const Jid &AStreamJid, const IArchiveRequest &ARequest)
{
	if (isCapable(AStreamJid,ArchiveManagement))
	{
		WorkingThread *wthread = new WorkingThread(this,FArchiver,this);
		wthread->setStreamJid(AStreamJid);
		wthread->setArchiveRequest(ARequest);
		connect(wthread,SIGNAL(finished()),SLOT(onWorkingThreadFinished()));
		return wthread->executeAction(WorkingThread::LoadHeaders);
	}
	return QString::null;
}

QString FileMessageArchive::loadCollection(const Jid &AStreamJid, const IArchiveHeader &AHeader)
{
	if (isCapable(AStreamJid,ArchiveManagement))
	{
		WorkingThread *wthread = new WorkingThread(this,FArchiver,this);
		wthread->setStreamJid(AStreamJid);
		wthread->setArchiveHeader(AHeader);
		connect(wthread,SIGNAL(finished()),SLOT(onWorkingThreadFinished()));
		return wthread->executeAction(WorkingThread::LoadCollection);
	}
	return QString::null;
}

QString FileMessageArchive::removeCollections(const Jid &AStreamJid, const IArchiveRequest &ARequest)
{
	if (isCapable(AStreamJid,ArchiveManagement))
	{
		WorkingThread *wthread = new WorkingThread(this,FArchiver,this);
		wthread->setStreamJid(AStreamJid);
		wthread->setArchiveRequest(ARequest);
		connect(wthread,SIGNAL(finished()),SLOT(onWorkingThreadFinished()));
		return wthread->executeAction(WorkingThread::RemoveCollection);
	}
	return QString::null;
}

QString FileMessageArchive::loadModifications(const Jid &AStreamJid, const QDateTime &AStart, int ACount)
{
	if (isCapable(AStreamJid,Replication))
	{
		WorkingThread *wthread = new WorkingThread(this,FArchiver,this);
		wthread->setStreamJid(AStreamJid);
		wthread->setModificationsStart(AStart);
		wthread->setModificationsCount(ACount);
		connect(wthread,SIGNAL(finished()),SLOT(onWorkingThreadFinished()));
		return wthread->executeAction(WorkingThread::LoadModifications);
	}
	return QString::null;
}

QString FileMessageArchive::archiveHomePath() const
{
	return FArchiveHomePath;
}

QString FileMessageArchive::collectionDirName(const Jid &AWith) const
{
	Jid jid = !AWith.node().isEmpty() ? gatewayJid(AWith) : AWith;

	QString dirName = Jid::encode(jid.pBare());
	if (!jid.resource().isEmpty())
		dirName += "/" + Jid::encode(jid.pResource());

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

	QDir dir(archiveHomePath());
	if (!dir.exists(ARCHIVE_DIR_NAME))
	{
		FThreadLock.lockForWrite();
		noError &= dir.mkdir(ARCHIVE_DIR_NAME);
		FThreadLock.unlock();
	}
	noError &= dir.cd(ARCHIVE_DIR_NAME);

	if (noError && AStreamJid.isValid())
	{
		QString streamDir = collectionDirName(AStreamJid.bare());
		if (!dir.exists(streamDir))
		{
			FThreadLock.lockForWrite();
			noError &= dir.mkdir(streamDir);
			FNewDirs.prepend(dir.absoluteFilePath(streamDir));
			FThreadLock.unlock();
		}
		noError &= dir.cd(streamDir);

		if (noError && AWith.isValid())
		{
			QString withDir = collectionDirName(AWith);
			if (!dir.exists(withDir))
			{
				FThreadLock.lockForWrite();
				foreach(QString subDir, withDir.split("/"))
				{
					if (!dir.exists(subDir))
					{
						noError &= dir.mkdir(subDir);
						FNewDirs.prepend(dir.absoluteFilePath(subDir));
					}
					noError &= dir.cd(subDir);
				}
				FThreadLock.unlock();
			}
			else
			{
				noError &= dir.cd(withDir);
			}
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
		QDirIterator::IteratorFlags flags = ARequest.with.isValid() && ARequest.exactmatch ? QDirIterator::NoIteratorFlags : QDirIterator::Subdirectories;
		
		QList<QString> dirPaths;
		if (ARequest.with.node().isEmpty() && !ARequest.exactmatch)
		{
			QString gateDomain = gatewayJid(ARequest.with).pDomain();
			QString encResource = Jid::encode(ARequest.with.pResource());
			QString streamDirPath = collectionDirPath(AStreamJid,Jid::null);
			
			if (ARequest.with.pDomain() != gateDomain)
				dirPaths.append(collectionDirPath(AStreamJid,ARequest.with));

			QDirIterator dirIt(streamDirPath,QDir::Dirs|QDir::NoDotAndDotDot);
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

		for (int i=0; i<dirPaths.count(); i++)
		{
			QDirIterator dirIt(dirPaths.at(i),QDir::Files,flags);
			while (dirIt.hasNext())
			{
				QString fpath = dirIt.next();
				QString fname = dirIt.fileName();
				if (fname.endsWith(CollectionExt) && (startName.isEmpty() || startName<=fname) && (endName.isEmpty() || endName>=fname))
				{
					if (checkCollectionFile(fpath,ARequest))
					{
						filesMap.insertMulti(fname,fpath);
						if (ARequest.maxItems>0 && filesMap.count()>ARequest.maxItems)
							filesMap.erase(ARequest.order==Qt::AscendingOrder ? --filesMap.end() : filesMap.begin());
					}
				}
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
	FThreadLock.lockForRead();
	IArchiveHeader header;
	CollectionWriter *writer = FWritingFiles.value(AFileName,NULL);
	if (writer == NULL)
	{
		QFile file(AFileName);
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
	FThreadLock.unlock();
	return header;
}

IArchiveCollection FileMessageArchive::loadCollectionFromFile(const QString &AFileName) const
{
	FThreadLock.lockForRead();
	IArchiveCollection collection;
	CollectionWriter *writer = FWritingFiles.value(AFileName,NULL);
	if (writer==NULL || writer->recordsCount()>0)
	{
		QFile file(AFileName);
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
	FThreadLock.unlock();
	return collection;
}

IArchiveModifications FileMessageArchive::loadFileModifications(const Jid &AStreamJid, const QDateTime &AStart, int ACount) const
{
	IArchiveModifications modifs;
	modifs.startTime = AStart.toUTC();

	QString dirPath = collectionDirPath(AStreamJid,Jid::null);
	if (!dirPath.isEmpty() && AStreamJid.isValid() && AStart.isValid())
	{
		FThreadLock.lockForRead();
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
						modif.header.engineId = engineId();
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
		FThreadLock.unlock();
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

		FThreadLock.lockForWrite();
		QFile file(fileName);
		if (file.open(QFile::WriteOnly|QFile::Truncate))
		{
			QDomDocument doc;
			QDomElement chatElem = doc.appendChild(doc.createElement("chat")).toElement();
			FArchiver->collectionToElement(collection,chatElem,ASaveMode);
			file.write(doc.toByteArray(2));
			file.close();
			FThreadLock.unlock();
			saveFileModification(AStreamJid,collection.header,logAction);
			emit fileCollectionSaved(AStreamJid,collection.header); // TODO: emit signal from main thread
			return true;
		}
		FThreadLock.unlock();
	}
	return false;
}

bool FileMessageArchive::removeCollectionFile(const Jid &AStreamJid, const Jid &AWith, const QDateTime &AStart)
{
	QString fileName = collectionFilePath(AStreamJid,AWith,AStart);
	if (QFile::exists(fileName))
	{
		IArchiveHeader header = loadHeaderFromFile(fileName);
		FThreadLock.lockForWrite();
		CollectionWriter *writer = findCollectionWriter(AStreamJid,header);
		if (writer)
		{
			FThreadLock.unlock();
			removeCollectionWriter(writer);
			FThreadLock.lockForWrite();
		}
		if (QFile::remove(fileName))
		{
			FThreadLock.unlock();
			saveFileModification(AStreamJid,header,LOG_ACTION_REMOVE);
			emit fileCollectionRemoved(AStreamJid,header);  // TODO: emit signal from main thread
			return true;
		}
		FThreadLock.unlock();
	}
	return false;
}

void FileMessageArchive::loadGatewayTypes()
{
	FGatewayTypes.clear();
	QString dirPath = collectionDirPath(Jid::null,Jid::null);
	QFile gateways(dirPath+"/"GATEWAY_FILE_NAME);
	if (!dirPath.isEmpty() && gateways.open(QFile::ReadOnly|QFile::Text))
	{
		while (!gateways.atEnd())
		{
			QStringList gateway = QString::fromUtf8(gateways.readLine()).split(" ");
			if (!gateway.value(0).isEmpty() && !gateway.value(1).isEmpty())
				FGatewayTypes.insert(gateway.value(0),gateway.value(1));
		}
	}
	gateways.close();
}

Jid FileMessageArchive::gatewayJid(const Jid &AJid) const
{
	Jid jid = AJid;
	FThreadLock.lockForRead();
	if (FGatewayTypes.contains(jid.domain()))
		jid.setDomain(QString("%1.gateway").arg(FGatewayTypes.value(jid.domain())));
	FThreadLock.unlock();
	return jid;
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

bool FileMessageArchive::checkCollectionFile(const QString &AFileName, const IArchiveRequest &ARequest) const
{
	QFile file(AFileName);
	if (file.open(QFile::ReadOnly))
	{
		QXmlStreamReader reader(&file);
		reader.setNamespaceProcessing(false);

		Qt::CheckState validCheck = Qt::PartiallyChecked;
		Qt::CheckState textState = ARequest.text.isEmpty() ? Qt::Checked : Qt::PartiallyChecked;
		Qt::CheckState threadState = ARequest.threadId.isEmpty() ? Qt::Checked : Qt::PartiallyChecked;

		QStringList elemStack;
		bool checkElemText = false;
		while (!reader.atEnd() && validCheck!=Qt::Unchecked && textState!=Qt::Unchecked && threadState!=Qt::Unchecked && 
			(validCheck==Qt::PartiallyChecked || textState==Qt::PartiallyChecked || threadState==Qt::PartiallyChecked))
		{
			reader.readNext();
			if (reader.isStartElement())
			{
				elemStack.append(reader.qualifiedName().toString().toLower());
				QString elemPath = elemStack.join("/");
				if (elemPath == "chat")
				{
					if (reader.attributes().value("with").isEmpty())
						validCheck = Qt::Unchecked;
					else if (reader.attributes().value("start").isEmpty())
						validCheck = Qt::Unchecked;
					else
						validCheck = Qt::Checked;

					if (reader.attributes().value("thread").compare(ARequest.threadId)==0)
						threadState = Qt::Checked;
					else if (threadState == Qt::PartiallyChecked)
						threadState = Qt::Unchecked;

					if (textState != Qt::Checked)
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
				else if (textState != Qt::Checked)
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
		return validCheck==Qt::Checked && textState==Qt::Checked && threadState==Qt::Checked;
	}
	return false;
}

bool FileMessageArchive::saveFileModification(const Jid &AStreamJid, const IArchiveHeader &AHeader, const QString &AAction) const
{
	QString dirPath = collectionDirPath(AStreamJid,Jid::null);
	if (!dirPath.isEmpty() && AStreamJid.isValid() && AHeader.with.isValid() && AHeader.start.isValid())
	{
		FThreadLock.lockForWrite();
		QFile log(dirPath+"/"LOG_FILE_NAME);
		if (log.open(QFile::WriteOnly|QFile::Append|QIODevice::Text))
		{
			QStringList logFields;
			logFields << DateTime(QDateTime::currentDateTime()).toX85UTC();
			logFields << AAction;
			logFields << AHeader.with.full();
			logFields << DateTime(AHeader.start).toX85UTC();
			logFields << QString::number(AHeader.version);
			logFields << "\n";
			log.write(logFields.join(" ").toUtf8());
			log.close();
			FThreadLock.unlock();
			return true;
		}
		FThreadLock.unlock();
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

CollectionWriter *FileMessageArchive::newCollectionWriter(const Jid &AStreamJid, const IArchiveHeader &AHeader, const QString &AFileName)
{
	if (AHeader.with.isValid() && AHeader.start.isValid() && !AFileName.isEmpty() && !FWritingFiles.contains(AFileName))
	{
		CollectionWriter *writer = new CollectionWriter(AStreamJid,AFileName,AHeader,this);
		if (writer->isOpened())
		{
			FWritingFiles.insert(writer->fileName(),writer);
			FCollectionWriters[AStreamJid].insert(AHeader.with,writer);
			connect(writer,SIGNAL(writerDestroyed(CollectionWriter *)),SLOT(onCollectionWriterDestroyed(CollectionWriter *)));
			emit fileCollectionOpened(AStreamJid,AHeader);
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

void FileMessageArchive::removeCollectionWriter(CollectionWriter *AWriter)
{
	FThreadLock.lockForWrite();
	if (FWritingFiles.contains(AWriter->fileName()))
	{
		AWriter->closeAndDeleteLater();
		FWritingFiles.remove(AWriter->fileName());
		FCollectionWriters[AWriter->streamJid()].remove(AWriter->header().with,AWriter);
		if (AWriter->recordsCount() > 0)
		{
			FThreadLock.unlock();
			saveFileModification(AWriter->streamJid(),AWriter->header(),LOG_ACTION_CREATE);
			emit fileCollectionSaved(AWriter->streamJid(),AWriter->header());
		}
		else
		{
			FThreadLock.unlock();
		}
	}
	else
	{
		FThreadLock.unlock();
	}
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
				emit headersLoaded(wthread->workId(),wthread->archiveHeaders());
				break;
			case WorkingThread::LoadCollection:
				emit collectionLoaded(wthread->workId(),wthread->archiveCollection());
				break;
			case WorkingThread::LoadModifications:
				emit modificationsLoaded(wthread->workId(),wthread->archiveModifications());
				break;
			}
		}
		else
		{
			emit requestFailed(wthread->workId(),wthread->error());
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
	removeCollectionWriter(AWriter);
}

void FileMessageArchive::onOptionsOpened()
{
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
	FArchiveHomePath = FPluginManager->homePath();
}

void FileMessageArchive::onDiscoInfoReceived(const IDiscoInfo &AInfo)
{
	if (AInfo.node.isEmpty() && AInfo.contactJid.node().isEmpty() &&  AInfo.contactJid.resource().isEmpty() && !FGatewayTypes.contains(AInfo.contactJid))
	{
		foreach(IDiscoIdentity identity, AInfo.identity)
		{
			if (identity.category==CATEGORY_GATEWAY && !identity.type.isEmpty())
			{
				QString dirPath = collectionDirPath(Jid::null,Jid::null);
				QFile gateways(dirPath+"/"GATEWAY_FILE_NAME);
				if (!dirPath.isEmpty() && gateways.open(QFile::WriteOnly|QFile::Append|QFile::Text))
				{
					QStringList gateway;
					gateway << AInfo.contactJid.pDomain();
					gateway << identity.type;
					gateway << "\n";
					gateways.write(gateway.join(" ").toUtf8());
					gateways.close();
				}
				FGatewayTypes.insert(AInfo.contactJid,identity.type);
				break;
			}
		}
	}
}

#ifndef HAVE_QT5
Q_EXPORT_PLUGIN2(plg_filemessagearchive, FileMessageArchive)
#endif
