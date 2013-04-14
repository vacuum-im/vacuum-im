#ifndef FILESTORAGE_H
#define FILESTORAGE_H

#include <QList>
#include <QHash>
#include <QString>
#include <QDateTime>
#include <QStringList>
#include "utilsexport.h"

//Directories and files
#define FILE_STORAGE_SHARED_DIR         "shared"
#define FILE_STORAGE_DEFINITIONS_MASK   "*def.xml"

//Common storage options
#define FILE_STORAGE_NAME               "name"
#define FILE_STORAGE_DESCRIPTION        "description"
#define FILE_STORAGE_VERSION            "version"
#define FILE_STORAGE_AUTHOR             "author"

class UTILS_EXPORT FileStorage :
	public QObject
{
	Q_OBJECT;
	struct StorageObject;
public:
	FileStorage(const QString &AStorage, const QString &ASubStorage = FILE_STORAGE_SHARED_DIR, QObject *AParent = NULL);
	virtual ~FileStorage();
	bool isExist() const;
	QString storage() const;
	QString subStorage() const;
	void setSubStorage(const QString &ASubStorage);
	QList<QString> fileKeys() const;
	QList<QString> fileFirstKeys() const;
	int filesCount(const QString &AKey) const;
	QString fileName(const QString &AKey, int AIndex = 0) const;
	QString fileFullName(const QString &AKey, int AIndex = 0) const;
	QString fileMime(const QString &AKey, int AIndex = 0) const;
	QString fileCacheKey(const QString &AKey, int AIndex =0) const;
	QString storageProperty(const QString &AName, const QString &ADefValue = QString::null) const;
	QString fileProperty(const QString &AKey, const QString &AName, const QString &ADefValue = QString::null) const;
	void reloadDefinitions();
signals:
	void storageChanged();
public:
	static QList<QString> availStorages();
	static QList<QString> availSubStorages(const QString &AStorage, bool ACheckDefs = true);
	static QList<QString> subStorageDirs(const QString &AStorage, const QString &ASubStorage);
	static QList<QString> resourcesDirs();
	static void setResourcesDirs(const QList<QString> &ADirs);
	static FileStorage *staticStorage(const QString &AStorage);
protected:
	void loadDefinitions(const QString &ADefFile, int APrefixIndex);
private:
	QString FStorage;
	QString FSubStorage;
	QList<QString> FPrefixes;
private:
	QList<QString> FKeys;
	QList<StorageObject> FObjects;
	QHash<QString, uint> FKey2Object;
	QHash<QString, QString> FProperties;
private:
	static QList<QString> FMimeTypes;
	static QList<QString> FResourceDirs;
	static QList<FileStorage *> FInstances;
	static QHash<QString, FileStorage *> FStaticStorages;
private:
	static QList<QString> FKeyTags;
	static QList<QString> FFileTags;
	static QList<QString> FObjectTags;
};

#endif // FILESTORAGE_H
