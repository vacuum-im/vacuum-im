#include "unzipfile.h"
#include <QFile>
#include <QHash>

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


UnzipFile::UnzipFile(QObject *AParent)
  : QObject(AParent)
{
  d = new UnzipFileData();
}

UnzipFile::UnzipFile(const QString &AZipFileName, QObject *AParent)
  : QObject(AParent)
{
  d = new UnzipFileData(AZipFileName);
  if (d->FUNZFile != NULL)
    loadZippedFilesInfo();
}

UnzipFile::~UnzipFile()
{

}

bool UnzipFile::openFile(const QString &AZipFileName)
{
  d->FZipFileName = AZipFileName;
  qDeleteAll(d->FZippedFiles);
  d->FUNZFile = unzOpen(QFile::encodeName(AZipFileName));
  if (d->FUNZFile != NULL)
    return loadZippedFilesInfo();
  return false;
}

bool UnzipFile::isValid() const 
{ 
  return d->FUNZFile != NULL; 
}

const QString &UnzipFile::zipFileName() const 
{ 
  return d->FZipFileName; 
}

QList<QString> UnzipFile::fileNames() const 
{ 
  return d->FZippedFiles.keys(); 
}

unsigned long UnzipFile::fileSize( const QString &AFileName ) const
{
  if (d->FZippedFiles.contains(AFileName))
    return d->FZippedFiles.value(AFileName)->size;
  return 0;
}

QByteArray UnzipFile::fileData( const QString &AFileName ) const
{
  if (d->FZippedFiles.contains(AFileName))
  {
    //UnzipFileData::ZippedFile *zippedFile = d->FZippedFiles.value(AFileName);
    //QByteArray unzippedData = zippedFile->data;
    //if (unzippedData.isEmpty())
    //{
    //  unzippedData = loadZippedFileData(AFileName);
    //  if (d->FCashData)
    //    zippedFile->data = unzippedData;
    //}
    //return unzippedData;
    return loadZippedFileData(AFileName);
  }
  return QByteArray();
}

bool UnzipFile::loadZippedFilesInfo()
{
  if (isValid() && unzGoToFirstFile(d->FUNZFile) == UNZ_OK)
  {
    unz_file_info fileInfo;
    char *fileNameBuff = new char[255];
    do 
    {
      if (unzGetCurrentFileInfo(d->FUNZFile,&fileInfo,fileNameBuff,255,NULL,0,NULL,0) == UNZ_OK)
      {
        UnzipFileData::ZippedFile *newZippedFile = new UnzipFileData::ZippedFile;
        newZippedFile->size = fileInfo.uncompressed_size;
        newZippedFile->name = QString(fileNameBuff);
        d->FZippedFiles.insert(newZippedFile->name,newZippedFile);
      }
    } while (unzGoToNextFile(d->FUNZFile) == UNZ_OK);
    delete [] fileNameBuff;
    return true;
  }
  return false;
}

QByteArray UnzipFile::loadZippedFileData( const QString &AFileName ) const
{
  if (isValid() && unzLocateFile(d->FUNZFile,QFile::encodeName(AFileName),0) == UNZ_OK)
  {
    int openResult = unzOpenCurrentFile(d->FUNZFile);
    if (openResult == UNZ_OK)
    {
      QByteArray unzippedData(fileSize(AFileName),0);
      int readBytes = unzReadCurrentFile(d->FUNZFile,unzippedData.data(),unzippedData.size());
      
      if (readBytes == unzippedData.size())
        return unzippedData;
    }
    unzCloseCurrentFile(d->FUNZFile);
  }
  return QByteArray();
}
