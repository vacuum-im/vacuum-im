#include "unzipfile.h"

#ifdef USE_SYSTEM_MINIZIP
#	include <minizip/unzip.h>
#else
#	include <thirdparty/minizip/unzip.h>
#endif

struct UnzipFileData::ZippedFile {
	QString name;
	unsigned long size;
	QByteArray data;
};

UnzipFileData::UnzipFileData()
{
	FUNZFile = NULL;
	FFilesReaded = false;
}

UnzipFileData::UnzipFileData(const UnzipFileData &AOther) : QSharedData(AOther)
{
	if (AOther.FUNZFile)
		FUNZFile = unzOpen(QFile::encodeName(FZipFileName));
	else
		FUNZFile = NULL;
	FFilesReaded = AOther.FFilesReaded;
	FZipFileName = AOther.FZipFileName;
	FZippedFiles = AOther.FZippedFiles;
}

UnzipFileData::~UnzipFileData()
{
	if (FUNZFile)
		unzClose(FUNZFile);
	qDeleteAll(FZippedFiles);
}


UnzipFile::UnzipFile()
{
	d = new UnzipFileData;
}

UnzipFile::UnzipFile(const QString &AZipFileName, bool AReadFiles)
{
	d = new UnzipFileData;
	openFile(AZipFileName,AReadFiles);
}

UnzipFile::~UnzipFile()
{

}

bool UnzipFile::isValid() const
{
	return d->FUNZFile != NULL;
}

const QString &UnzipFile::zipFileName() const
{
	return d->FZipFileName;
}

bool UnzipFile::openFile(const QString &AZipFileName, bool AReadFiles)
{
	if (d->FUNZFile)
		unzClose(d->FUNZFile);
	qDeleteAll(d->FZippedFiles);

	d->FZipFileName = AZipFileName;
	d->FUNZFile = unzOpen(QFile::encodeName(AZipFileName));
	if (d->FUNZFile)
		return loadZippedFilesInfo(AReadFiles);
	return false;
}

QList<QString> UnzipFile::fileNames() const
{
	return d->FZippedFiles.keys();
}

unsigned long UnzipFile::fileSize(const QString &AFileName) const
{
	if (d->FZippedFiles.contains(AFileName))
		return d->FZippedFiles.value(AFileName)->size;
	return 0;
}

QByteArray UnzipFile::fileData(const QString &AFileName) const
{
	UnzipFileData::ZippedFile *zippedFile = d->FFilesReaded ? d->FZippedFiles.value(AFileName) : NULL;
	QByteArray unzippedData = zippedFile!=NULL ? zippedFile->data : loadZippedFileData(AFileName);
	return unzippedData;
}

bool UnzipFile::loadZippedFilesInfo(bool AReadFiles)
{
	if (isValid() && unzGoToFirstFile(d->FUNZFile) == UNZ_OK)
	{
		d->FFilesReaded = AReadFiles;
		unz_file_info fileInfo;
		char *fileNameBuff = new char[255];
		do
		{
			if (unzGetCurrentFileInfo(d->FUNZFile,&fileInfo,fileNameBuff,255,NULL,0,NULL,0) == UNZ_OK)
			{
				UnzipFileData::ZippedFile *newZippedFile = new UnzipFileData::ZippedFile;
				newZippedFile->size = fileInfo.uncompressed_size;
				newZippedFile->name = QString(fileNameBuff);
				if (AReadFiles && unzOpenCurrentFile(d->FUNZFile) == UNZ_OK)
				{
					newZippedFile->data.resize(newZippedFile->size);
					unzReadCurrentFile(d->FUNZFile,newZippedFile->data.data(),newZippedFile->data.size());
					unzCloseCurrentFile(d->FUNZFile);
				}
				d->FZippedFiles.insert(newZippedFile->name,newZippedFile);
			}
		} while (unzGoToNextFile(d->FUNZFile) == UNZ_OK);
		delete [] fileNameBuff;
		return true;
	}
	return false;
}

QByteArray UnzipFile::loadZippedFileData(const QString &AFileName) const
{
	if (isValid() && unzLocateFile(d->FUNZFile,QFile::encodeName(AFileName),0) == UNZ_OK)
	{
		if (unzOpenCurrentFile(d->FUNZFile) == UNZ_OK)
		{
			QByteArray unzippedData(fileSize(AFileName),0);
			unzReadCurrentFile(d->FUNZFile,unzippedData.data(),unzippedData.size());
			unzCloseCurrentFile(d->FUNZFile);
			return unzippedData;
		}
	}
	return QByteArray();
}
