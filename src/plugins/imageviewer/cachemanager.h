#ifndef CACHEMANAGER_H
#define CACHEMANAGER_H

#include <QObject>
#include <QByteArray>
#include <QDir>
#include <QHash>
#include <QString>
#include <QUrl>

#define DIR_IMAGEVIEWER "imageviewer"
#define IV_MANIFEST_FILENAME "index.json"

#define IV_DEFAULT_MAX_DIMENSION     4096
#define IV_DEFAULT_MAX_DOWNLOAD_MB   5
#define IV_DEFAULT_MAX_DOWNLOAD_BYTES (IV_DEFAULT_MAX_DOWNLOAD_MB * 1024 * 1024)
#define IV_DEFAULT_CACHE_MAX_BYTES   (256 * 1024 * 1024)
#define IV_DEFAULT_CACHE_MAX_AGE_DAYS 30
#define IV_DEFAULT_CACHE_MAX_COUNT   2000

#define IV_DEFAULT_PREVIEW_DIMENSION 280
#define IV_MIN_PREVIEW_DIMENSION     64
#define IV_MAX_PREVIEW_DIMENSION     2048
#define IV_MAX_PREVIEW_BYTES         (32 * 1024 * 1024)
#define IV_PREVIEW_SUFFIX            "_preview"

// The images downloaded from the messages are stored here, together with a
// smaller preview of every image and a manifest of the stored files.
class CacheManager : public QObject
{
	Q_OBJECT
public:
	// The outcome of the preview computation made in a worker thread.
	struct PreviewResult
	{
		bool valid = false;
		QByteArray previewPng;
	};
	explicit CacheManager(QObject *AParent = nullptr);
	// Prepares the cache directory, loads the manifest and prunes the cache
	bool init(const QString &ACacheRoot);
	bool isAvailable() const;
	// The directory the cached files are stored in
	QString dir() const;
	// The path of the cached file of the url
	QString filePath(const QUrl &AUrl) const;
	// The path of the cached file of an inline image
	QString dataUrlFilePath(const QString &ADataUrl, const QByteArray &AData) const;
	// The path of the preview of a cached file
	QString previewFilePath(const QString &ACachePath) const;
	// The cached image
	QByteArray readImage(const QString &ACachePath) const;
	bool writeImage(const QString &ACachePath, const QByteArray &AData) const;
	// Remembers the file in the manifest
	void touch(const QString &ACachePath, qint64 ASize);
	// Removes the file and forgets about it
	void drop(const QString &ACachePath);
	// Removes the file with its preview and forgets about both of them
	void dropImage(const QString &ACachePath);
	void loadManifest();
	void saveManifest() const;
	void prune();
	// Validates the image data and makes the png of the preview of it. This is
	// pure computation without any file or option access, it is safe to call
	// from a worker thread with the limits read on the main thread.
	static PreviewResult makePreviewData(const QByteArray &AData, int AMaxDimension, qint64 AMaxBytes, int APreviewDimension);
	// The stored preview of the data can be reused when it matches the current
	// limits. Only the image headers are read, the data is not decoded.
	bool reusePreview(const QString &ACachePath, const QByteArray &AData, QString &APreviewPath) const;
	// Stores the data with its ready preview in the cache and updates the
	// manifest entries of both files. Must be called on the main thread.
	bool applyPreview(const QString &ACachePath, const QByteArray &AData, const QByteArray &APreviewPng, QString &APreviewPath);
	// The data is an image within the limits
	bool validateImageData(const QByteArray &AData) const;
	// The limits of the cached images
	int maxDownloadBytes() const;
	int maxDimension() const;
	int previewDimension() const;
private:
	qint64 cacheSizeLimit(const QString &AFileName) const;
	QByteArray readCacheFile(const QString &ACachePath, qint64 AMaxBytes) const;
	bool writeCacheFile(const QString &ACachePath, const QByteArray &AData, qint64 AMaxBytes) const;
private:
	QDir FCacheDir;
	bool FCacheAvailable;
	QHash<QString, qint64> FManifestSizes;
	QHash<QString, qint64> FManifestAccessed;
	qint64 FTotalManifestBytes;
	int FManifestCount;
};

#endif // CACHEMANAGER_H
