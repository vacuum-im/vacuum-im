#ifndef UNZIPFILE_H
#define UNZIPFILE_H

#include <QFile>
#include <QHash>
#include <QMetaType>
#include <QByteArray>
#include <QSharedData>
#include "utilsexport.h"

class UnzipFileData :
	public QSharedData
{
public:
	UnzipFileData();
	UnzipFileData(const UnzipFileData &AOther);
	~UnzipFileData();
	bool FFilesReaded;
	void *FUNZFile;
	struct ZippedFile;
	QString FZipFileName;
	QHash<QString, ZippedFile *> FZippedFiles;
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

#endif // UNZIPFILE_H
