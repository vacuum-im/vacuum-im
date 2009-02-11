#ifndef FILESTORAGE_H
#define FILESTORAGE_H

#include <QList>
#include <QHash>
#include <QString>
#include <QDateTime>
#include <QStringList>
#include "utilsexport.h"

//Directories and files
#define STORAGE_DIR             "resources"
#define STORAGE_SHARED_DIR      "shared"
#define STORAGE_DEFFILES_MASK   "*.def.xml"

//Common storage options
#define STORAGE_NAME            "name"
#define STORAGE_DESCRIPTION     "description"
#define STORAGE_VERSION         "version"
#define STORAGE_AUTHOR          "author"

class UTILS_EXPORT FileStorage : 
  public QObject
{
  Q_OBJECT;
public:
  FileStorage(const QString &AStorage, const QString &ASubStorage = STORAGE_SHARED_DIR, QObject *AParent = NULL);
  virtual ~FileStorage();
  QString storage() const;
  QString subStorage() const;
  void setSubStorage(const QString &ASubStorage);
  QString storageRootDir() const;
  QString option(const QString &AOption) const;
  QList<QString> fileKeys() const;
  QList<QString> fileFirstKeys() const;
  int filesCount(const QString AKey) const;
  QString fileName(const QString AKey, int AIndex = 0) const;
  QString fileFullName(const QString AKey, int AIndex = 0) const;
  QString fileMime(const QString AKey, int AIndex = 0) const;
  QString fileOption(const QString AKey, const QString &AOption) const;
signals:
  void storageChanged();
public:
  static QStringList availStorages();
  static QStringList availSubStorages(const QString &AStorage);
  static FileStorage *staticStorage(const QString &AStorage);
protected:
  void loadDefinations(const QString &ADefFile, const QString APrefix);
private:
  QString FStorage;
  QString FSubStorage;
  QString FFilePrefix;
private:
  struct StorageObject {
    QList<int> fileTypes;
    QList<QString> fileNames;
    QHash<QString, QString> fileOptions;
  };
  QList<QString> FKeys;
  QHash<QString, uint> FKey2Object;
  QList<StorageObject> FObjects;
  QHash<QString, QString> FOptions;
private:
  static QList<QString> FMimeTypes;
  static QHash<QString, FileStorage *> FStaticStorages;
private:
  static QList<QString> FObjectTags;
  static QList<QString> FKeyTags;
  static QList<QString> FFileTags;
};

#endif // FILESTORAGE_H
