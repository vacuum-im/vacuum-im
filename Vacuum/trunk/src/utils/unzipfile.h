#ifndef UNZIPFILE_H
#define UNZIPFILE_H

#include <QObject>
#include <QByteArray>
#include <QSharedData>
#include "utilsexport.h"
#include "../thirdparty/minizip/unzip.h"

class UnzipFileData;

class UTILS_EXPORT UnzipFile : 
  public QObject
{
  Q_OBJECT;

public:
  UnzipFile(QObject *AParent = NULL);
  UnzipFile(const QString &AZipFileName, QObject *AParent = NULL);
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
