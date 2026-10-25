#include "cachemanager.h"

#include <QBuffer>
#include <QCryptographicHash>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QRegularExpression>
#include <QSaveFile>

#include <algorithm>

#include <definitions/optionvalues.h>

#include <utils/logger.h>
#include <utils/options.h>

namespace {
	bool isSafeCacheFileName(const QString &AName)
	{
		static const QRegularExpression re(QStringLiteral("^[0-9a-f]{64}(?:_preview)?\\.[a-z0-9]{1,8}$"),
										   QRegularExpression::CaseInsensitiveOption);
		return re.match(AName).hasMatch();
	}

	bool isPreviewCacheFileName(const QString &AName)
	{
		return AName.contains(IV_PREVIEW_SUFFIX);
	}

	// Common part of the cached file names: the hash of the source url.
	// An image and its preview share it and are handled as a single unit.
	QString cacheBaseName(const QString &AFileName)
	{
		const int dot = AFileName.indexOf(QLatin1Char('.'));
		if (dot <= 0)
			return QString();
		QString base = AFileName.left(dot);
		if (isPreviewCacheFileName(base))
			base.chop(static_cast<int>(QString(IV_PREVIEW_SUFFIX).length()));
		return base;
	}

	// Checks the payload against the decoding limits and reports the declared
	// image size. The payload is not decoded here, only its header is read.
	bool checkImagePayload(const QByteArray &AData, int AMaxDimension, qint64 AMaxBytes, QSize &ADeclaredSize)
	{
		ADeclaredSize = QSize();
		if (AData.isEmpty() || AData.size() > AMaxBytes)
			return false;

		QBuffer input;
		input.setData(AData);
		if (!input.open(QIODevice::ReadOnly))
			return false;

		const QByteArray sourceFormat = QImageReader::imageFormat(&input).toLower();
		if (sourceFormat == QByteArrayLiteral("svg") || sourceFormat == QByteArrayLiteral("svgz"))
			return false; // avoid rendering untrusted vector documents with external references
		if (!input.seek(0))
			return false;

		QImageReader reader(&input);
		reader.setAutoTransform(true);
		const QSize declaredSize = reader.size();
		constexpr qint64 maxPixels = 16LL * 1024 * 1024;
		if (!declaredSize.isValid() || declaredSize.isEmpty() ||
			declaredSize.width() > AMaxDimension || declaredSize.height() > AMaxDimension ||
			qint64(declaredSize.width()) * declaredSize.height() > maxPixels)
			return false;

		ADeclaredSize = declaredSize;
		return true;
	}
} // namespace

CacheManager::CacheManager(QObject *AParent)
	: QObject(AParent), FCacheDir(), FCacheAvailable(false), FTotalManifestBytes(0), FManifestCount(0)
{
}

bool CacheManager::init(const QString &ACacheRoot)
{
	FCacheDir.setPath(ACacheRoot);
	if (!FCacheDir.exists(DIR_IMAGEVIEWER) && !FCacheDir.mkdir(DIR_IMAGEVIEWER))
	{
		LOG_INFO(QString("ImageViewer cache cannot be created at %1").arg(FCacheDir.absolutePath()));
		FCacheAvailable = false;
		return false;
	}
	if (!FCacheDir.cd(DIR_IMAGEVIEWER))
	{
		LOG_INFO(QString("ImageViewer cache not accessible at %1").arg(FCacheDir.absolutePath()));
		FCacheAvailable = false;
		return false;
	}
	const QFileInfo dirInfo(FCacheDir.absolutePath());
	if (!dirInfo.isDir() || !dirInfo.isWritable())
	{
		LOG_INFO("ImageViewer cache directory is not writable");
		FCacheAvailable = false;
		return false;
	}

	FCacheAvailable = true;
	loadManifest();
	prune();
	return true;
}

bool CacheManager::isAvailable() const
{
	return FCacheAvailable;
}

QString CacheManager::dir() const
{
	return FCacheDir.absolutePath();
}

// The option keeps the megabytes, the rest of the code works with the bytes.
int CacheManager::maxDownloadBytes() const
{
	const qlonglong value = Options::node(OPV_MESSAGES_IMAGES_MAXDOWNLOADMB).value().toLongLong();
	constexpr qlonglong maxAllowed = 256;
	return static_cast<int>((value > 0 ? qMin(value, maxAllowed) : IV_DEFAULT_MAX_DOWNLOAD_MB) * 1024 * 1024);
}

int CacheManager::maxDimension() const
{
	const int value = Options::node(OPV_MESSAGES_IMAGES_MAXDIMENSION).value().toInt();
	return value > 0 ? qBound(256, value, 16384) : IV_DEFAULT_MAX_DIMENSION;
}

int CacheManager::previewDimension() const
{
	const int value = Options::node(OPV_MESSAGES_IMAGES_PREVIEWDIMENSION).value().toInt();
	return value > 0 ? qBound(IV_MIN_PREVIEW_DIMENSION, value, IV_MAX_PREVIEW_DIMENSION)
					 : IV_DEFAULT_PREVIEW_DIMENSION;
}

QString CacheManager::filePath(const QUrl &AUrl) const
{
	QString hash = QCryptographicHash::hash(AUrl.toString().toUtf8(), QCryptographicHash::Sha256).toHex();
	QString ext = QFileInfo(AUrl.path()).suffix().toLower();
	if (ext.isEmpty() || !QImageReader::supportedImageFormats().contains(ext.toUtf8()))
		ext = "png";
	return FCacheDir.filePath(hash + "." + ext);
}

QString CacheManager::dataUrlFilePath(const QString &ADataUrl, const QByteArray &AData) const
{
	QString hash = QCryptographicHash::hash(ADataUrl.toUtf8(), QCryptographicHash::Sha256).toHex();
	QByteArray format;
	{
		QBuffer buffer;
		buffer.setData(AData);
		if (buffer.open(QIODevice::ReadOnly))
			format = QImageReader::imageFormat(&buffer).toLower();
	}
	QString ext = QString::fromLatin1(format);
	if (ext == QLatin1String("jpeg"))
		ext = "jpg";
	if (ext.isEmpty() || !QImageReader::supportedImageFormats().contains(ext.toUtf8()))
		ext = "png";
	return FCacheDir.filePath(hash + "." + ext);
}

QString CacheManager::previewFilePath(const QString &ACachePath) const
{
	const QFileInfo info(ACachePath);
	return info.absolutePath() + QLatin1Char('/') +
		   info.completeBaseName() + IV_PREVIEW_SUFFIX + ".png";
}

// Originals are limited by the download limit, previews by their own limit:
// a scaled png may be bigger than the original it was made from.
qint64 CacheManager::cacheSizeLimit(const QString &AFileName) const
{
	return isPreviewCacheFileName(AFileName) ? IV_MAX_PREVIEW_BYTES : maxDownloadBytes();
}

QByteArray CacheManager::readImage(const QString &ACachePath) const
{
	return readCacheFile(ACachePath, maxDownloadBytes());
}

bool CacheManager::writeImage(const QString &ACachePath, const QByteArray &AData) const
{
	return writeCacheFile(ACachePath, AData, maxDownloadBytes());
}

QByteArray CacheManager::readCacheFile(const QString &ACachePath, qint64 AMaxBytes) const
{
	const QFileInfo info(ACachePath);
	if (!info.exists() || !info.isFile() || info.isSymLink() || info.size() <= 0 || info.size() > AMaxBytes)
		return QByteArray();

	QFile file(ACachePath);
	if (!file.open(QIODevice::ReadOnly))
		return QByteArray();
	QByteArray data = file.read(AMaxBytes + 1LL);
	if (data.size() > AMaxBytes || file.error() != QFileDevice::NoError)
		return QByteArray();
	return data;
}

bool CacheManager::writeCacheFile(const QString &ACachePath, const QByteArray &AData, qint64 AMaxBytes) const
{
	if (AData.isEmpty() || AData.size() > AMaxBytes)
		return false;

	QSaveFile file(ACachePath);
	file.setDirectWriteFallback(false);
	if (!file.open(QIODevice::WriteOnly))
		return false;
	if (file.write(AData) != AData.size())
	{
		file.cancelWriting();
		return false;
	}
	if (!file.commit())
		return false;
	QFile::setPermissions(ACachePath, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
	return true;
}

void CacheManager::touch(const QString &ACachePath, qint64 ASize)
{
	const QString name = QFileInfo(ACachePath).fileName();
	qint64 now = QDateTime::currentDateTimeUtc().toSecsSinceEpoch();
	qint64 oldSize = FManifestSizes.value(name, 0);
	bool isNew = !FManifestAccessed.contains(name);
	FManifestAccessed[name] = now;
	FManifestSizes[name] = ASize;
	if (isNew)
	{
		FManifestCount++;
		FTotalManifestBytes += ASize;
	}
	else if (ASize != oldSize)
	{
		FTotalManifestBytes += ASize - oldSize;
	}
}

// Removes a cached file and forgets about it in the manifest.
void CacheManager::drop(const QString &ACachePath)
{
	const QString name = QFileInfo(ACachePath).fileName();
	QFile::remove(ACachePath);
	FTotalManifestBytes = qMax<qint64>(0, FTotalManifestBytes - FManifestSizes.take(name));
	FManifestAccessed.remove(name);
	FManifestCount = FManifestAccessed.size();
}

// Removes a cached file with its preview.
void CacheManager::dropImage(const QString &ACachePath)
{
	drop(ACachePath);
	// The preview of a removed image is not usable anymore.
	drop(previewFilePath(ACachePath));
}

void CacheManager::loadManifest()
{
	FManifestAccessed.clear();
	FManifestSizes.clear();
	FTotalManifestBytes = 0;
	FManifestCount = 0;

	const QString manifestPath = FCacheDir.filePath(IV_MANIFEST_FILENAME);
	QFile mfile(manifestPath);
	if (mfile.open(QIODevice::ReadOnly) && mfile.size() <= 4 * 1024 * 1024)
	{
		QJsonParseError err;
		const QJsonDocument doc = QJsonDocument::fromJson(mfile.readAll(), &err);
		mfile.close();
		if (err.error == QJsonParseError::NoError && doc.isObject())
		{
			const QJsonArray arr = doc.object().value(QStringLiteral("files")).toArray();
			for (const QJsonValue &value : arr)
			{
				if (!value.isObject())
					continue;
				const QJsonObject object = value.toObject();
				const QString name = object.value(QStringLiteral("name")).toString();
				if (!isSafeCacheFileName(name))
					continue;

				const QFileInfo fi(FCacheDir.filePath(name));
				if (!fi.exists() || !fi.isFile() || fi.isSymLink() || fi.size() <= 0 || fi.size() > cacheSizeLimit(name))
					continue;

				const qint64 accessed = object.value(QStringLiteral("accessed")).toVariant().toLongLong();
				const qint64 safeAccessed = qBound<qint64>(0, accessed, QDateTime::currentDateTimeUtc().toSecsSinceEpoch());
				FManifestAccessed.insert(name, safeAccessed);
				FManifestSizes.insert(name, fi.size());
			}
		}
	}

	// Reconcile the manifest with real files and account for files left by a
	// crash before the last manifest commit. Never follow symlinks here.
	const QFileInfoList files = FCacheDir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
	for (const QFileInfo &fi : files)
	{
		const QString name = fi.fileName();
		if (name == IV_MANIFEST_FILENAME)
			continue;
		if (!isSafeCacheFileName(name) || fi.isSymLink() || fi.size() <= 0 || fi.size() > cacheSizeLimit(name))
		{
			if (fi.isSymLink() || isSafeCacheFileName(name))
				QFile::remove(fi.filePath());
			continue;
		}
		if (!FManifestAccessed.contains(name))
			FManifestAccessed.insert(name, fi.lastModified().toUTC().toSecsSinceEpoch());
		FManifestSizes.insert(name, fi.size());
	}

	for (auto it = FManifestAccessed.begin(); it != FManifestAccessed.end();)
	{
		if (!FManifestSizes.contains(it.key()))
			it = FManifestAccessed.erase(it);
		else
			++it;
	}

	FManifestCount = FManifestAccessed.size();
	FTotalManifestBytes = 0;
	for (auto it = FManifestSizes.constBegin(); it != FManifestSizes.constEnd(); ++it)
		FTotalManifestBytes += it.value();
}

void CacheManager::saveManifest() const
{
	if (!FCacheAvailable)
		return;

	QJsonArray files;
	for (auto it = FManifestAccessed.constBegin(); it != FManifestAccessed.constEnd(); ++it)
	{
		if (!isSafeCacheFileName(it.key()))
			continue;
		const QFileInfo fi(FCacheDir.filePath(it.key()));
		if (!fi.exists() || !fi.isFile() || fi.isSymLink())
			continue;
		QJsonObject object;
		object[QStringLiteral("name")] = it.key();
		object[QStringLiteral("size")] = static_cast<qint64>(fi.size());
		object[QStringLiteral("accessed")] = static_cast<qint64>(it.value());
		files.append(object);
	}

	QJsonObject root;
	root[QStringLiteral("files")] = files;
	QSaveFile file(FCacheDir.filePath(IV_MANIFEST_FILENAME));
	file.setDirectWriteFallback(false);
	if (!file.open(QIODevice::WriteOnly))
		return;
	const QByteArray json = QJsonDocument(root).toJson(QJsonDocument::Indented);
	if (file.write(json) != json.size())
	{
		file.cancelWriting();
		return;
	}
	file.commit();
}

void CacheManager::prune()
{
	if (!FCacheAvailable)
		return;

	qint64 maxBytes = Options::node(OPV_MESSAGES_IMAGES_CACHE_MAXBYTES).value().toLongLong();
	if (maxBytes <= 0)
		maxBytes = IV_DEFAULT_CACHE_MAX_BYTES;
	int maxAgeDays = Options::node(OPV_MESSAGES_IMAGES_CACHE_MAXAGE_DAYS).value().toInt();
	if (maxAgeDays <= 0)
		maxAgeDays = IV_DEFAULT_CACHE_MAX_AGE_DAYS;
	int maxCount = Options::node(OPV_MESSAGES_IMAGES_CACHE_MAXCOUNT).value().toInt();
	if (maxCount <= 0)
		maxCount = IV_DEFAULT_CACHE_MAX_COUNT;

	const qint64 now = QDateTime::currentDateTimeUtc().toSecsSinceEpoch();
	// An image and its preview share the same base name and are pruned as a
	// single unit, so that a preview never outlives the image it was made from.
	struct Group
	{
		QStringList names;
		qint64 size = 0;
		qint64 accessed = 0;
	};
	QHash<QString, Group> groups;
	for (auto it = FManifestAccessed.constBegin(); it != FManifestAccessed.constEnd(); ++it)
	{
		if (!isSafeCacheFileName(it.key()))
			continue;
		Group &group = groups[cacheBaseName(it.key())];
		if (group.names.isEmpty())
		{
			group.accessed = it.value();
		}
		else
		{
			group.accessed = qMin(group.accessed, it.value());
		}
		group.names.append(it.key());
		group.size += FManifestSizes.value(it.key(), 0);
	}

	QList<Group> ordered = groups.values();
	std::sort(ordered.begin(), ordered.end(), [](const Group &a, const Group &b)
			  { return a.accessed < b.accessed; });
	for (const Group &group : ordered)
	{
		const bool expired = group.accessed <= now && (now - group.accessed) > qint64(maxAgeDays) * 86400;
		const bool overLimit = FTotalManifestBytes > maxBytes || FManifestCount > maxCount;
		if (!expired && !overLimit)
			continue;

		for (const QString &name : group.names)
		{
			const qint64 size = FManifestSizes.value(name, 0);
			const QString path = FCacheDir.filePath(name);
			const QFileInfo fi(path);
			if (fi.isSymLink() || !fi.exists() || QFile::remove(path))
			{
				FManifestSizes.remove(name);
				FManifestAccessed.remove(name);
				FTotalManifestBytes = qMax<qint64>(0, FTotalManifestBytes - size);
				FManifestCount = qMax(0, FManifestCount - 1);
			}
		}
	}
	// Run a reconciliation after possible removals and persist current access times.
	FManifestCount = FManifestAccessed.size();
	FTotalManifestBytes = 0;
	for (auto it = FManifestSizes.constBegin(); it != FManifestSizes.constEnd(); ++it)
		FTotalManifestBytes += it.value();
	saveManifest();
}

bool CacheManager::validateImageData(const QByteArray &AData) const
{
	QSize declaredSize;
	// Refuse formats/readers that cannot report dimensions before decoding.
	// This avoids attempting an unbounded allocation for malformed payloads.
	if (!checkImagePayload(AData, maxDimension(), maxDownloadBytes(), declaredSize))
		return false;

	QBuffer buffer;
	buffer.setData(AData);
	if (!buffer.open(QIODevice::ReadOnly))
		return false;

	QImageReader reader(&buffer);
	reader.setAutoTransform(true);
	const QImage image = reader.read();
	const int limit = maxDimension();
	constexpr qint64 maxPixels = 16LL * 1024 * 1024;
	return !image.isNull() && image.width() <= limit && image.height() <= limit &&
		   qint64(image.width()) * image.height() <= maxPixels;
}

CacheManager::PreviewResult CacheManager::makePreviewData(const QByteArray &AData, int AMaxDimension, qint64 AMaxBytes, int APreviewDimension)
{
	PreviewResult result;
	QSize declaredSize;
	if (!checkImagePayload(AData, AMaxDimension, AMaxBytes, declaredSize))
		return result;

	QBuffer input;
	input.setData(AData);
	if (!input.open(QIODevice::ReadOnly))
		return result;

	QImageReader reader(&input);
	reader.setAutoTransform(true);
	QImage image = reader.read();
	constexpr qint64 maxPixels = 16LL * 1024 * 1024;
	if (image.isNull() || image.width() > AMaxDimension || image.height() > AMaxDimension ||
		qint64(image.width()) * image.height() > maxPixels)
		return result;
	// The image is scaled here and not by the reader, so that the preview size
	// is exact and the preview can be reused later. The decoded size is used,
	// it may differ from the declared one when the image is rotated.
	if (image.width() > APreviewDimension || image.height() > APreviewDimension)
		image = image.scaled(QSize(APreviewDimension, APreviewDimension), Qt::KeepAspectRatio, Qt::SmoothTransformation);
	if (image.isNull())
		return result;

	QByteArray png;
	QBuffer output(&png);
	if (!output.open(QIODevice::WriteOnly) || !image.save(&output, "PNG"))
		return result;
	if (png.size() > IV_MAX_PREVIEW_BYTES)
		return result;

	result.valid = true;
	result.previewPng = png;
	return result;
}

bool CacheManager::reusePreview(const QString &ACachePath, const QByteArray &AData, QString &APreviewPath) const
{
	APreviewPath.clear();
	QSize declaredSize;
	if (!checkImagePayload(AData, maxDimension(), maxDownloadBytes(), declaredSize))
		return false;
	const QString previewPath = previewFilePath(ACachePath);
	const QSize expectedSize = declaredSize.scaled(previewDimension(), previewDimension(), Qt::KeepAspectRatio);

	// An existing preview is reused when it still matches the current limit.
	// This also invalidates the previews made with another limit.
	const QFileInfo previewInfo(previewPath);
	if (!previewInfo.exists() || !previewInfo.isFile() || previewInfo.isSymLink() ||
		previewInfo.size() <= 0 || previewInfo.size() > IV_MAX_PREVIEW_BYTES)
		return false;
	QImageReader existing(previewPath);
	const QSize actualSize = existing.size();
	// The size may be transposed when the image carries a rotation.
	const QSize transposedSize(expectedSize.height(), expectedSize.width());
	if (!actualSize.isValid() || (actualSize != expectedSize && actualSize != transposedSize))
		return false;
	APreviewPath = previewPath;
	return true;
}

bool CacheManager::applyPreview(const QString &ACachePath, const QByteArray &AData, const QByteArray &APreviewPng, QString &APreviewPath)
{
	APreviewPath.clear();
	if (!FCacheAvailable || AData.isEmpty())
		return false;

	const QByteArray existing = readImage(ACachePath);
	bool cached = existing == AData;
	if (!cached)
	{
		cached = writeImage(ACachePath, AData);
		if (cached)
			// The preview was made from the previous content of the file.
			drop(previewFilePath(ACachePath));
	}
	if (!cached)
		return false;
	touch(ACachePath, AData.size());

	QString reused;
	if (reusePreview(ACachePath, AData, reused))
	{
		touch(reused, QFileInfo(reused).size());
		APreviewPath = reused;
		return true;
	}
	if (APreviewPng.isEmpty() || !writeCacheFile(previewFilePath(ACachePath), APreviewPng, IV_MAX_PREVIEW_BYTES))
		return false;
	touch(previewFilePath(ACachePath), APreviewPng.size());
	APreviewPath = previewFilePath(ACachePath);
	return true;
}
