#ifndef UNZIPFILE_H
#define UNZIPFILE_H

#include <QObject>
#include <QHash>
#include <QByteArray>
#include "utilsexport.h"
#include "../thirdparty/minizip/unzip.h"

class UTILS_EXPORT UnzipFile : 
  public QObject
{
  Q_OBJECT;

public:
  UnzipFile(const QString &AZipFileName, QObject *AParent = NULL);
  ~UnzipFile();

  bool openZipFile(const QString &AZipFileName);
  bool isValid() const { return FUNZFile != NULL; }
  const QString &zipFileName() const { return FZipFileName; }
  QList<QString> fileNames() const { return FZippedFiles.keys(); }
  unsigned long fileSize(const QString &AFileName) const;
  QByteArray fileData(const QString &AFileName) const;
protected:
  struct ZippedFile {
    QString name;
    unsigned long size;
    QByteArray data;
  };
protected:
  bool loadZippedFilesInfo();
  QByteArray loadZippedFileData(const QString &AFileName) const;
  void closeZipFile();
private:
  unzFile FUNZFile;    
  QString FZipFileName;
  QHash<QString,ZippedFile *> FZippedFiles;
  bool FCashData;
};

#endif // UNZIPFILE_H
