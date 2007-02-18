#ifndef UNZIPFILE_H
#define UNZIPFILE_H

#include <QFile>
#include <QHash>
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
    FCashData = false;
  }
  UnzipFileData(const QString &AZipFileName)
  {
    FZipFileName = AZipFileName;
    FUNZFile = unzOpen(QFile::encodeName(AZipFileName));
    FCashData = false;
  }
  UnzipFileData(const UnzipFileData &AOther) :
    QSharedData(AOther)
  {
    FZipFileName = AOther.FZipFileName;
    FUNZFile = AOther.FUNZFile;  //unzOpen(QFile::encodeName(AZipFileName));
    FZippedFiles = AOther.FZippedFiles;
    FCashData = AOther.FCashData;
  }
  ~UnzipFileData()
  { 
    if (FUNZFile)
      unzClose(FUNZFile);
    qDeleteAll(FZippedFiles);
  }
public:
  unzFile FUNZFile;    
  QString FZipFileName;
  QHash<QString,ZippedFile *> FZippedFiles;
  bool FCashData;
};


class UTILS_EXPORT UnzipFile
{
public:
  UnzipFile();
  UnzipFile(const QString &AZipFileName);
  ~UnzipFile();

  bool openFile(const QString &AZipFileName);
  bool isValid() const;
  const QString &zipFileName() const;
  QList<QString> fileNames() const;
  unsigned long fileSize(const QString &AFileName) const;
  QByteArray fileData(const QString &AFileName) const;
protected:
  bool loadZippedFilesInfo();
  QByteArray loadZippedFileData(const QString &AFileName) const;
private:
  QSharedDataPointer<UnzipFileData> d;
};

#endif // UNZIPFILE_H
