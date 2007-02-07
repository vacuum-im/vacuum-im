#include "unzipfile.h"
#include <QFile>

UnzipFile::UnzipFile(const QString &AZipFileName, QObject *AParent)
  : QObject(AParent)
{
  FUNZFile = NULL;
  FCashData = true;
  openZipFile(AZipFileName);
}

UnzipFile::~UnzipFile()
{
  closeZipFile();
}

bool UnzipFile::openZipFile( const QString &AZipFileName )
{
  closeZipFile();
  FZipFileName = AZipFileName;
  FUNZFile = unzOpen(QFile::encodeName(FZipFileName));

  if (FUNZFile != NULL)
    return loadZippedFilesInfo();

  return false;
}

unsigned long UnzipFile::fileSize( const QString &AFileName ) const
{
  if (FZippedFiles.contains(AFileName))
    return FZippedFiles.value(AFileName)->size;
  return 0;
}

QByteArray UnzipFile::fileData( const QString &AFileName ) const
{
  if (FZippedFiles.contains(AFileName))
  {
    ZippedFile *zippedFile = FZippedFiles.value(AFileName);
    QByteArray unzippedData = zippedFile->data;
    if (unzippedData.isEmpty())
    {
      unzippedData = loadZippedFileData(AFileName);
      if (FCashData)
        zippedFile->data = unzippedData;
    }
    return unzippedData;
  }
  return QByteArray();
}

bool UnzipFile::loadZippedFilesInfo()
{
  if (isValid() && unzGoToFirstFile(FUNZFile) == UNZ_OK)
  {
    unz_file_info fileInfo;
    char *fileNameBuff = new char[255];
    do 
    {
      if (unzGetCurrentFileInfo(FUNZFile,&fileInfo,fileNameBuff,255,NULL,0,NULL,0) == UNZ_OK)
      {
        ZippedFile *newZippedFile = new ZippedFile;
        newZippedFile->size = fileInfo.uncompressed_size;
        newZippedFile->name = QString(fileNameBuff);
        FZippedFiles.insert(newZippedFile->name,newZippedFile);
      }
    } while (unzGoToNextFile(FUNZFile) == UNZ_OK);
    delete [] fileNameBuff;
    return true;
  }
  return false;
}

QByteArray UnzipFile::loadZippedFileData( const QString &AFileName ) const
{
  if (isValid() && unzLocateFile(FUNZFile,QFile::encodeName(AFileName),0) == UNZ_OK)
  {
    int openResult = unzOpenCurrentFile(FUNZFile);
    if (openResult == UNZ_OK)
    {
      QByteArray unzippedData(fileSize(AFileName),0);
      int readBytes = unzReadCurrentFile(FUNZFile,unzippedData.data(),unzippedData.size());
      
      if (readBytes == unzippedData.size())
        return unzippedData;
    }
    unzCloseCurrentFile(FUNZFile);
  }
  return QByteArray();
}

void UnzipFile::closeZipFile()
{
  if (isValid())
    unzClose(FUNZFile);
  FUNZFile = NULL;
  qDeleteAll(FZippedFiles);
}

