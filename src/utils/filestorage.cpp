#include "filestorage.h"

#include <QDir>
#include <QSet>
#include <QDomDocument>
#include <QApplication>

QList<QString> FileStorage::FMimeTypes;
QList<QString> FileStorage::FResourceDirs;
QList<FileStorage *> FileStorage::FInstances;
QHash<QString, FileStorage *> FileStorage::FStaticStorages;

struct FileStorage::StorageObject 
{
	int prefix;
	QList<int> fileTypes;
	QList<QString> fileNames;
	QHash<QString, QString> fileProperties;
};

FileStorage::FileStorage(const QString &AStorage, const QString &ASubStorage, QObject *AParent) : QObject(AParent)
{
	FInstances.append(this);

	FStorage = AStorage;
	setSubStorage(ASubStorage);
}

FileStorage::~FileStorage()
{
	FInstances.removeAll(this);
}

bool FileStorage::isExist() const
{
	return !subStorageDirs(FStorage,FSubStorage).isEmpty();
}

QString FileStorage::storage() const
{
	return FStorage;
}

QString FileStorage::subStorage() const
{
	return FSubStorage;
}

void FileStorage::setSubStorage(const QString &ASubStorage)
{
	if (FSubStorage.isNull() || FSubStorage!=ASubStorage)
	{
		FSubStorage = !ASubStorage.isEmpty() ? ASubStorage : FILE_STORAGE_SHARED_DIR;
		reloadDefinitions();
	}
}

QList<QString> FileStorage::fileKeys() const
{
	return FKeys;
}

QList<QString> FileStorage::fileFirstKeys() const
{
	QList<QString> keys = FKeys;
	int lastIndex = -1;
	QList<QString>::iterator it = keys.begin();
	while (it<keys.end())
	{
		int index = FKey2Object.value(*it);
		if (index != lastIndex)
		{
			lastIndex = index;
			++it;
		}
		else
		{
			it = keys.erase(it);
		}
	}
	return keys;
}

int FileStorage::filesCount(const QString &AKey) const
{
	return FObjects.value(FKey2Object.value(AKey)).fileNames.count();
}

QString FileStorage::fileName(const QString &AKey, int AIndex) const
{
	return FObjects.value(FKey2Object.value(AKey,-1)).fileNames.value(AIndex);
}

QString FileStorage::fileFullName(const QString &AKey, int AIndex) const
{
	QString name = fileName(AKey,AIndex);
	if (!name.isEmpty())
	{
		int prefix = FObjects.value(FKey2Object.value(AKey,-1)).prefix;
		return FPrefixes.at(prefix) + name;
	}
	return QString::null;
}

QString FileStorage::fileMime(const QString &AKey, int AIndex) const
{
	return FMimeTypes.at(FObjects.value(FKey2Object.value(AKey)).fileTypes.value(AIndex));
}

QString FileStorage::fileCacheKey(const QString &AKey, int AIndex) const
{
	QString name = fileName(AKey,AIndex);
	if (!name.isEmpty())
		return FSubStorage + "/" + name;
	return QString::null;
}

QString FileStorage::storageProperty(const QString &AName, const QString &ADefValue) const
{
	return FProperties.value(AName,ADefValue);
}

QString FileStorage::fileProperty(const QString &AKey, const QString &AName, const QString &ADefValue) const
{
	return FObjects.value(FKey2Object.value(AKey)).fileProperties.value(AName,ADefValue);
}

void FileStorage::reloadDefinitions()
{
	FPrefixes.clear();
	FProperties.clear();
	FObjects.clear();
	FKey2Object.clear();

	QList<QString> subDirs = subStorageDirs(FStorage,FSubStorage);
	if (FSubStorage != FILE_STORAGE_SHARED_DIR)
		subDirs += subStorageDirs(FStorage,FILE_STORAGE_SHARED_DIR);

	int prefixIndex = 0;
	foreach(const QString &subDir, subDirs)
	{
		QDir dir(subDir);
		if (dir.exists())
		{
			FPrefixes.append(subDir+"/");
			foreach(const QString &file, dir.entryList(QStringList() << FILE_STORAGE_DEFINITIONS_MASK))
				loadDefinitions(dir.absoluteFilePath(file),prefixIndex);
			prefixIndex++;
		}
	}

	emit storageChanged();
}

QList<QString> FileStorage::availStorages()
{
	QList<QString> storages;
	foreach(const QString &dirPath, FResourceDirs)
	{
		QDir dir(dirPath);
		QList<QString> dirStorages = dir.entryList(QDir::Dirs|QDir::NoDotAndDotDot);

		QList<QString>::iterator it = dirStorages.begin();
		while(it != dirStorages.end())
		{
			if (storages.contains(*it))
				it = dirStorages.erase(it);
			else
				++it;
		}
		storages.append(dirStorages);
	}
	return storages;
}

QList<QString> FileStorage::availSubStorages(const QString &AStorage, bool ACheckDefs)
{
	QList<QString> storages;
	foreach(const QString &dirPath, FResourceDirs)
	{
		QDir dir(dirPath);
		if (dir.exists() && dir.cd(AStorage))
		{
			QList<QString> dirStorages = dir.entryList(QDir::Dirs|QDir::NoDotAndDotDot);
			dirStorages.removeAll(FILE_STORAGE_SHARED_DIR);

			QList<QString>::iterator it = dirStorages.begin();
			while (it != dirStorages.end())
			{
				if (dir.cd(*it))
				{
					if (storages.contains(*it))
						it = dirStorages.erase(it);
					else if (ACheckDefs && dir.entryList(QStringList()<<FILE_STORAGE_DEFINITIONS_MASK).isEmpty())
						it = dirStorages.erase(it);
					else
						++it;
					dir.cdUp();
				}
				else
				{
					it = dirStorages.erase(it);
				}
			}
			storages.append(dirStorages);
		}
	}
	return storages;
}

QList<QString> FileStorage::subStorageDirs(const QString &AStorage, const QString &ASubStorage)
{
	QList<QString> subDirs;
	foreach(const QString &dirPath, FResourceDirs)
	{
		QDir dir(dirPath);
		if (dir.exists() && dir.cd(AStorage))
		{
			if (dir.entryList(QDir::Dirs|QDir::NoDotAndDotDot).contains(ASubStorage))
				subDirs.append(QDir::cleanPath(dir.absoluteFilePath(ASubStorage)));
		}
	}
	return subDirs;
}

QList<QString> FileStorage::resourcesDirs()
{
	return FResourceDirs;
}

void FileStorage::setResourcesDirs(const QList<QString> &ADirs)
{
	QList<QString> cleanDirs;
	foreach(const QString &dir, ADirs)
	{
		if (!dir.isEmpty() && !cleanDirs.contains(dir) && QDir(dir).exists())
			cleanDirs.append(QDir::cleanPath(dir));
	}
	if (FResourceDirs != cleanDirs)
	{
		QList<FileStorage *> updateStorages;
		QSet<QString> oldDirs = FResourceDirs.toSet() - cleanDirs.toSet();
		QSet<QString> newDirs = cleanDirs.toSet() - FResourceDirs.toSet();

		foreach(FileStorage *fileStorage, FInstances)
		{
			bool update = false;
			for(QSet<QString>::const_iterator dirIt = oldDirs.constBegin(); !update && dirIt!=oldDirs.constEnd(); ++dirIt)
			{
				QList<QString> curPrefixes = fileStorage->FPrefixes;
				for(QList<QString>::const_iterator prefIt = curPrefixes.constBegin(); !update && prefIt!=curPrefixes.constEnd(); ++prefIt)
					update = (*prefIt).startsWith(*dirIt);
			}
			if (update)
			{
				updateStorages.append(fileStorage);
			}
		}
		
		FResourceDirs = cleanDirs;

		foreach(FileStorage *fileStorage, FInstances)
		{
			if (!updateStorages.contains(fileStorage))
			{
				bool update = false;
				for(QSet<QString>::const_iterator dirIt = newDirs.constBegin(); !update && dirIt!=newDirs.constEnd(); ++dirIt)
				{
					QList<QString> newPrefixes = subStorageDirs(fileStorage->storage(),fileStorage->subStorage());
					for(QList<QString>::const_iterator prefIt = newPrefixes.constBegin(); !update && prefIt!=newPrefixes.constEnd(); ++prefIt)
						update = (*prefIt).startsWith(*dirIt);
				}
				if (update)
				{
					updateStorages.append(fileStorage);
				}
			}
		}
	
		foreach(FileStorage *fileStorage, updateStorages)
		{
			fileStorage->reloadDefinitions();
		}
	}
}

FileStorage *FileStorage::staticStorage(const QString &ASubStorage)
{
	FileStorage *fileStorage = FStaticStorages.value(ASubStorage,NULL);
	if (!fileStorage)
	{
		fileStorage = new FileStorage(ASubStorage,FILE_STORAGE_SHARED_DIR,qApp);
		FStaticStorages.insert(ASubStorage,fileStorage);
	}
	return fileStorage;
}

void FileStorage::loadDefinitions(const QString &ADefFile, int APrefixIndex)
{
	static const QList<QString> keyTags = QList<QString>() << "key"  << "text" << "name";
	static const QList<QString> fileTags = QList<QString>() << "object";
	static const QList<QString> objectTags = QList<QString>() << "file" << "icon";

	QDomDocument doc;
	QFile file(ADefFile);
	if (file.open(QFile::ReadOnly) && doc.setContent(&file,true))
	{
		QDomElement objElem = doc.documentElement().firstChildElement();
		while (!objElem.isNull())
		{
			if (objectTags.contains(objElem.tagName()))
			{
				StorageObject object;
				object.prefix = APrefixIndex;

				QList<QString> objKeys;
				QDomElement keyElem = objElem.firstChildElement();
				while (!keyElem.isNull())
				{
					if (keyTags.contains(keyElem.tagName()))
					{
						QString key = keyElem.text();
						if (!FKey2Object.contains(key))
							objKeys.append(key);
					}
					else if (fileTags.contains(keyElem.tagName()))
					{
						if (!keyElem.text().isEmpty())
						{
							QString mimeType = keyElem.attribute("mime");
							int typeIndex = FMimeTypes.indexOf(mimeType);
							if (typeIndex < 0)
							{
								typeIndex = FMimeTypes.count();
								FMimeTypes.append(mimeType);
							}
							object.fileTypes.append(typeIndex);
							object.fileNames.append(keyElem.text());
						}
					}
					else if (keyElem.firstChildElement().isNull() && keyElem.attributes().count()==0)
					{
						object.fileProperties.insert(keyElem.tagName(),keyElem.text());
					}
					keyElem = keyElem.nextSiblingElement();
				}

				if (!objKeys.isEmpty() && !object.fileNames.isEmpty())
				{
					bool valid = true;
					for (int i=0; valid && i<object.fileNames.count(); i++)
					{
						valid = QFile::exists(FPrefixes.at(object.prefix) + object.fileNames.at(i));
					}
					if (valid)
					{
						foreach (const QString &key, objKeys)
						{
							FKeys.append(key);
							FKey2Object.insert(key,FObjects.count());
						}
						FObjects.append(object);
					}
				}
			}
			else if (objElem.firstChildElement().isNull() && objElem.attributes().count()==0 && !FProperties.contains(objElem.tagName()))
			{
				FProperties.insert(objElem.tagName(),objElem.text());
			}
			objElem = objElem.nextSiblingElement();
		}
	}
}
