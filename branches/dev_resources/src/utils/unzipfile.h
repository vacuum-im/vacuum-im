#ifndef UNZIPFILE_H
#define UNZIPFILE_H

#include <QFile>
#include <QHash>
#include <QMetaType>
#include <QByteArray>
#include <QSharedData>
#include "utilsexport.h"
#include "../thirdparty/minizip/unzip.h"

class UnzipFileData :
  public QSharedData
{
public:
  struct ZippedFile {
    QString name;
    unsigned long size;
    QByteArray data;
  };
public:
  UnzipFileData()
  {
    FUNZFile = NULL;
    FFilesReaded = false;
  }
  UnzipFileData(const UnzipFileData &AOther) : QSharedData(AOther)
  {
    if (AOther.FUNZFile)
      FUNZFile = unzOpen(QFile::encodeName(FZipFileName));
    else
      FUNZFile = NULL;
    FFilesReaded = AOther.FFilesReaded;
    FZipFileName = AOther.FZipFileName;
    FZippedFiles = AOther.FZippedFiles;
  }
  ~UnzipFileData()
  { 
    if (FUNZFile)
      unzClose(FUNZFile);
    qDeleteAll(FZippedFiles);
  }
public:
  bool FFilesReaded;
  unzFile FUNZFile;    
  QString FZipFileName;
  QHash<QString,ZippedFile *> FZippedFiles;
};


class UTILS_EXPORT UnzipFile
{
public:
  UnzipFile();
  UnzipFile(const QString &AZipFileName, bool AReadFiles = false);
  ~UnzipFile();
  bool isValid() const;
  const QString &zipFileName() const;
  bool openFile(const QString &AZipFileName, bool AReadFiles = false);
  QList<QString> fileNames() const;
  unsigned long fileSize(const QString &AFileName) const;
  QByteArray fileData(const QString &AFileName) const;
protected:
  bool loadZippedFilesInfo(bool AReadFiles);
  QByteArray loadZippedFileData(const QString &AFileName) const;
private:
  QSharedDataPointer<UnzipFileData> d;
};

Q_DECLARE_METATYPE(UnzipFile);
#define UNZIPFILE_METATYPE_ID qMetaTypeId<UnzipFile>()

#endif // UNZIPFILE_H
