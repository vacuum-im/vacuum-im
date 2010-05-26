#include "filestorage.h"

#include <QDir>
#include <QDomDocument>
#include <QApplication>

QList<QString> FileStorage::FMimeTypes;
QList<QString> FileStorage::FResourceDirs;
QHash<QString, FileStorage *> FileStorage::FStaticStorages;

QList<QString> FileStorage::FObjectTags = QList<QString>() << "file" << "icon";
QList<QString> FileStorage::FKeyTags    = QList<QString>() << "key"  << "text" << "name";
QList<QString> FileStorage::FFileTags   = QList<QString>() << "object";

FileStorage::FileStorage(const QString &AStorage, const QString &ASubStorage, QObject *AParent) : QObject(AParent)
{
	FStorage = AStorage;
	setSubStorage(ASubStorage);
}

FileStorage::~FileStorage()
{

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
		FOptions.clear();
		FObjects.clear();
		FKey2Object.clear();
		FSubStorage = !ASubStorage.isEmpty() ? ASubStorage : STORAGE_SHARED_DIR;

		FSubPrefix = subStorageDir(FStorage,FSubStorage);
		if (!FSubPrefix.isEmpty())
		{
			QDir dir(FSubPrefix);
			FSubPrefix.append("/");
			if (FSubStorage!=STORAGE_SHARED_DIR && dir.exists())
			{
				foreach(QString file, dir.entryList(QStringList() << STORAGE_DEFFILES_MASK)) {
					loadDefinations(dir.absoluteFilePath(file),false); }
			}
		}

		FSharedPrefix = subStorageDir(FStorage,STORAGE_SHARED_DIR);
		if (!FSharedPrefix.isEmpty())
		{
			QDir dir(FSharedPrefix);
			FSharedPrefix.append("/");
			if (dir.exists())
			{
				foreach(QString file, dir.entryList(QStringList() << STORAGE_DEFFILES_MASK)) {
					loadDefinations(dir.absoluteFilePath(file),true); }
			}
		}

		emit storageChanged();
	}
}

QString FileStorage::option(const QString &AOption) const
{
	return FOptions.value(AOption);
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
			it++;
		}
		else
		{
			it = keys.erase(it);
		}
	}
	return keys;
}

int FileStorage::filesCount(const QString AKey) const
{
	return FObjects.value(FKey2Object.value(AKey)).fileNames.count();
}

QString FileStorage::fileName(const QString AKey, int AIndex) const
{
	return FObjects.value(FKey2Object.value(AKey,-1)).fileNames.value(AIndex);
}

QString FileStorage::fileFullName(const QString AKey, int AIndex) const
{
	QString name = fileName(AKey,AIndex);
	QString prefix = FObjects.value(FKey2Object.value(AKey,-1)).shared ? FSharedPrefix : FSubPrefix;
	return !name.isEmpty() ? prefix + name : name;
}

QString FileStorage::fileMime(const QString AKey, int AIndex) const
{
	return FMimeTypes.at(FObjects.value(FKey2Object.value(AKey)).fileTypes.value(AIndex));
}

QString FileStorage::fileOption(const QString AKey, const QString &AOption) const
{
	return FObjects.value(FKey2Object.value(AKey)).fileOptions.value(AOption);
}

QString FileStorage::fileCacheKey(const QString AKey, int AIndex) const
{
	QString name = fileName(AKey,AIndex);
	if (!name.isEmpty())
	{
		QString prefix = FObjects.at(FKey2Object.value(AKey,-1)).shared ? STORAGE_SHARED_DIR : FSubStorage;
		prefix.append("/");
		return prefix + name;
	}
	return QString::null;
}

QList<QString> FileStorage::availStorages()
{
	QList<QString> storages;
	foreach(QString dirPath, FResourceDirs)
	{
		QDir dir(dirPath);
		QList<QString> dirStorages = dir.entryList(QDir::Dirs|QDir::NoDotAndDotDot);
		for (QList<QString>::iterator it=dirStorages.begin(); it!=dirStorages.end(); it++)
			if (storages.contains(*it))
				it = dirStorages.erase(it);
		storages.append(dirStorages);
	}
	return storages;
}

QList<QString> FileStorage::availSubStorages(const QString &AStorage, bool ACheckDefs)
{
	QList<QString> storages;
	foreach(QString dirPath, FResourceDirs)
	{
		QDir dir(dirPath);
		if (dir.exists() && dir.cd(AStorage))
		{
			QList<QString> dirStorages = dir.entryList(QDir::Dirs|QDir::NoDotAndDotDot);
			dirStorages.removeAll(STORAGE_SHARED_DIR);

			QList<QString>::iterator it=dirStorages.begin();
			while (it!=dirStorages.end())
			{
				if (dir.cd(*it))
				{
					if (storages.contains(*it))
						it = dirStorages.erase(it);
					else if (ACheckDefs && dir.entryList(QStringList()<<STORAGE_DEFFILES_MASK).isEmpty())
						it = dirStorages.erase(it);
					else
						it++;
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

QString FileStorage::subStorageDir(const QString &AStorage, const QString &ASubStorage)
{
	foreach(QString dirPath, FResourceDirs)
	{
		QDir dir(dirPath);
		if (dir.exists() && dir.cd(AStorage))
		{
			if (dir.entryList(QDir::Dirs|QDir::NoDotAndDotDot).contains(ASubStorage))
				return dir.absoluteFilePath(ASubStorage);
		}
	}
	return QString::null;
}

QList<QString> FileStorage::resourcesDirs()
{
	return FResourceDirs;
}

void FileStorage::setResourcesDirs(const QList<QString> &ADirs)
{
	FResourceDirs = ADirs;
}

FileStorage *FileStorage::staticStorage(const QString &ASubStorage)
{
	FileStorage *fileStorage = FStaticStorages.value(ASubStorage,NULL);
	if (!fileStorage)
	{
		fileStorage = new FileStorage(ASubStorage,STORAGE_SHARED_DIR,qApp);
		FStaticStorages.insert(ASubStorage,fileStorage);
	}
	return fileStorage;
}

void FileStorage::loadDefinations(const QString &ADefFile, bool AShared)
{
	QDomDocument doc;
	QFile file(ADefFile);
	if (file.open(QFile::ReadOnly) && doc.setContent(file.readAll(),false))
	{
		QDomElement objElem = doc.documentElement().firstChildElement();
		while (!objElem.isNull())
		{
			if (FObjectTags.contains(objElem.tagName()))
			{
				StorageObject object;
				object.shared = AShared;

				QList<QString> objKeys;
				QDomElement keyElem = objElem.firstChildElement();
				while (!keyElem.isNull())
				{
					if (FKeyTags.contains(keyElem.tagName()))
					{
						QString key = keyElem.text();
						if (!FKey2Object.contains(key))
							objKeys.append(key);
					}
					else if (FFileTags.contains(keyElem.tagName()))
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
						object.fileOptions.insert(keyElem.tagName(),keyElem.text());
					}
					keyElem = keyElem.nextSiblingElement();
				}

				if (!objKeys.isEmpty() && !object.fileNames.isEmpty())
				{
					bool valid = true;
					for (int i=0; valid && i<object.fileNames.count(); i++)
					{
						valid = QFile::exists((AShared ? FSharedPrefix : FSubPrefix) + object.fileNames.at(i));
					}
					if (valid)
					{
						foreach (QString key, objKeys)
						{
							FKeys.append(key);
							FKey2Object.insert(key,FObjects.count());
						}
						FObjects.append(object);
					}
				}
			}
			else if (objElem.firstChildElement().isNull() && objElem.attributes().count()==0 && !FOptions.contains(objElem.tagName()))
			{
				FOptions.insert(objElem.tagName(),objElem.text());
			}
			objElem = objElem.nextSiblingElement();
		}
		file.close();
	}
}
