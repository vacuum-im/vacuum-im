#include "imageviewer.h"

#include <QTextBlock>
#include <QTextCursor>
#include <QTextImageFormat>
#include <QRegularExpression>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QNetworkReply>
#include <QTextBrowser>
#include <QApplication>
#include <QEventLoop>
#include <QTimer>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <definitions/messageviewurlhandlerorders.h>

#include <interfaces/ipluginmanager.h>
#include <utils/pluginhelper.h>
#include <utils/logger.h>

ImageViewer::ImageViewer()
	: FMessageWidgets(nullptr)
	, FNetworkManager(nullptr)
	, FPluginManager(nullptr)
{
}

ImageViewer::~ImageViewer()
{
}

void ImageViewer::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Image Viewer");
	APluginInfo->description = tr("Automatically displays images from URLs in messages");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Vacuum-im Team";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(MESSAGEWIDGETS_UUID);
}

bool ImageViewer::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
	
	FPluginManager = APluginManager;
	
	IPlugin *plugin = APluginManager->pluginInterface("IMessageWidgets").value(0, nullptr);
	if (plugin)
	{
		FMessageWidgets = qobject_cast<IMessageWidgets *>(plugin->instance());
	}
	
	FNetworkManager = new QNetworkAccessManager(this);
	return true;
}

bool ImageViewer::initObjects()
{
	initCacheDir();
	
	if (FMessageWidgets)
	{
		FMessageWidgets->insertStyleHandler(MVUHO_IMAGEVIEWER, this);
		LOG_INFO("Style handler registered");
	}
	
	return true;
}

bool ImageViewer::initSettings()
{
	return true;
}

	bool ImageViewer::messageStyleContentInsert(int AOrder, QWidget *AWidget, const QString &AHtml, QString &AModifiedHtml, const IMessageStyleContentOptions &AOptions)
{
	if (AOrder != MVUHO_IMAGEVIEWER)
		return false;

	LOG_INFO(QString("Processing message, HTML length=%1").arg(AHtml.length()));
	Q_UNUSED(AOptions);

	QString html = AHtml;
	
	// Match <a href="IMAGE_URL">...</a>
	QRegularExpression anchorRegex(R"(<a\b([^>]*?)href=["']([^"']+)["']([^>]*?)>(.*?)</a>)");
	QRegularExpressionMatchIterator anchorIt = anchorRegex.globalMatch(html);
	
	QList<QPair<QString, QUrl>> imageUrls;
	
	while (anchorIt.hasNext())
	{
		QRegularExpressionMatch match = anchorIt.next();
		QString url = match.captured(2);
		
		QUrl urlObj(url);
		QString lowerPath = urlObj.path().toLower();
		bool isImage = false;
		
		if (urlObj.scheme() == "http" || urlObj.scheme() == "https" || urlObj.scheme() == "data")
		{
			static const QStringList imageExts = { ".jpg", ".jpeg", ".png", ".gif", ".webp", ".bmp", ".svg", ".tiff", ".tif", ".ico", ".jfif" };
			for (const auto &ext : imageExts)
			{
				if (lowerPath.endsWith(ext))
				{
					isImage = true;
					break;
				}
			}
			if (!isImage && (lowerPath.contains("image") || lowerPath.contains("img") || lowerPath.contains("photo") || lowerPath.contains("avatar")))
				isImage = true;
		}
		
		if (isImage)
		{
			imageUrls.append({url, urlObj});
		}
	}
	
	if (imageUrls.isEmpty())
	{
		LOG_INFO("No image URLs found");
		AModifiedHtml = html;
		return true;
	}

		LOG_INFO(QString("Found %1 image URLs").arg(imageUrls.size()));
	
	// Download all images synchronously
	QList<PendingImage> pendingImages;
	QString modifiedHtml = html;
	
	for (const auto &imageUrl : imageUrls)
	{
		QImage image = downloadImageSync(imageUrl.second);
		if (!image.isNull())
		{
			QString placeholder = "IMG_PH_" + QCryptographicHash::hash(imageUrl.first.toUtf8(), QCryptographicHash::Sha1).toHex().left(16);
			
			// Replace <a href="URL">text</a> with placeholder in HTML
			QRegularExpressionMatchIterator it = anchorRegex.globalMatch(modifiedHtml);
			while (it.hasNext())
			{
				QRegularExpressionMatch match = it.next();
				QString url = match.captured(2);
				if (url == imageUrl.first)
				{
					int anchorStart = match.capturedStart(0);
					int anchorEnd = match.capturedEnd(0);
					modifiedHtml.replace(anchorStart, anchorEnd - anchorStart, placeholder);
					break;
				}
			}
			
			PendingImage pending;
			pending.url = imageUrl.second;
			pending.placeholder = placeholder;
			pendingImages.append(pending);
		}
	}
	
	AModifiedHtml = modifiedHtml;
	
	// Defer image insertion until after HTML is inserted into document
	QTextBrowser *textBrowser = qobject_cast<QTextBrowser *>(AWidget);
	QTextDocument *doc = textBrowser ? textBrowser->document() : nullptr;
	
	if (!pendingImages.isEmpty() && doc)
	{
		QTimer::singleShot(0, this, [this, doc, pendingImages]() {
			insertImagesInDocument(doc, pendingImages);
		});
	}
	
	return true;
}

void ImageViewer::insertImagesInDocument(QTextDocument *doc, const QList<PendingImage> &pendingImages)
{
	if (!doc)
		return;
	
	QTextCursor cursor(doc);
	cursor.beginEditBlock();
	
	for (const auto &pending : pendingImages)
	{
		// Find placeholder in document
		for (QTextBlock block = doc->begin(); block.isValid(); block = block.next())
		{
			QString blockText = block.text();
			int placeholderIndex = blockText.indexOf(pending.placeholder);
			
			if (placeholderIndex >= 0)
			{
				// Download image
				QImage image = downloadImageSync(pending.url);
				if (image.isNull())
					continue;
				
				// Create image format
				QTextImageFormat imageFormat;
				const int maxDim = 800;
				int imgWidth = image.width();
				int imgHeight = image.height();
				if (imgWidth > maxDim || imgHeight > maxDim)
				{
					const double ratio = static_cast<double>(imgWidth) / imgHeight;
					if (imgWidth > imgHeight)
					{
						imgWidth = maxDim;
						imgHeight = qRound(maxDim / ratio);
					}
					else
					{
						imgHeight = maxDim;
						imgWidth = qRound(maxDim * ratio);
					}
				}
				imageFormat.setWidth(imgWidth);
				imageFormat.setHeight(imgHeight);
				
				// Store image as document resource
				QString imageId = QCryptographicHash::hash(
					QByteArray::number(QRandomGenerator::global()->generate64()) + pending.placeholder.toUtf8(),
					QCryptographicHash::Sha1
				).toHex();
				QString imageUrl = QString("imageviewer://%1").arg(imageId);
				
				QUrl imageResourceUrl(imageUrl);
				doc->addResource(QTextDocument::ImageResource, imageResourceUrl, QVariant::fromValue(image));
				imageFormat.setName(imageUrl);
				imageFormat.setProperty(REAL_IMAGE_URL_PROPERTY, pending.url.toString());
				
				// Replace placeholder
				QTextCursor replaceCursor(doc);
				replaceCursor.setPosition(block.position() + placeholderIndex);
				replaceCursor.setPosition(replaceCursor.position() + pending.placeholder.length(), QTextCursor::KeepAnchor);
				replaceCursor.removeSelectedText();
				replaceCursor.insertImage(imageFormat);
				
				// Insert "[Open]" link under the image
				QString openLink = QString("<br><a href=\"%1\">[URL]</a>").arg(pending.url.toString());
				replaceCursor.insertHtml(openLink);
				
				break;
			}
		}
	}
	
	cursor.endEditBlock();
}

QImage ImageViewer::downloadImageSync(const QUrl &AUrl)
{
	// Check cache first
	QString cachePath = getCacheFilePath(AUrl);
	QFile cacheFile(cachePath);
	if (cacheFile.exists())
	{
		if (cacheFile.open(QIODevice::ReadOnly))
		{
			QImage image;
			if (image.loadFromData(cacheFile.readAll()))
			{
				cacheFile.close();
				return image;
			}
			cacheFile.close();
		}
	}
	
	// Download from network
	QNetworkRequest request(AUrl);
	request.setRawHeader("User-Agent", "Vacuum-IM/1.0");
	
	QNetworkReply *reply = FNetworkManager->get(request);
	
	// Wait for reply with timeout
	QEventLoop loop;
	QTimer timer;
	timer.setSingleShot(true);
	connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
	connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
	timer.start(15000); // 15 second timeout
	loop.exec();
	timer.stop();
	
	if (reply->error() != QNetworkReply::NoError)
	{
		reply->deleteLater();
		return QImage();
	}
	
	QByteArray data = reply->readAll();
	reply->deleteLater();
	
	QImage image;
	if (!image.loadFromData(data))
		return QImage();
	
	// Save to cache
	QFile outFile(cachePath);
	if (outFile.open(QIODevice::WriteOnly))
	{
		outFile.write(data);
		outFile.close();
	}
	
	return image;
}

QString ImageViewer::getCacheFilePath(const QUrl &AUrl)
{
	QString hash = QCryptographicHash::hash(AUrl.toString().toUtf8(), QCryptographicHash::Sha256).toHex();
	QString ext = QFileInfo(AUrl.path()).suffix().toLower();
	if (ext.isEmpty() || !QImageReader::supportedImageFormats().contains(ext.toUtf8()))
		ext = "png";
	return FCacheDir + "/" + hash + "." + ext;
}

bool ImageViewer::initCacheDir()
{
	if (!FPluginManager)
	{
		LOG_WARNING("PluginManager is null, cannot initialize cache");
		return false;
	}
	FCacheDir = FPluginManager->homePath() + "/imageviewer-cache";
	LOG_INFO(QString("Cache dir=%1").arg(FCacheDir));
	QDir cacheDir(FCacheDir);
	if (!cacheDir.exists())
	{
		return cacheDir.mkpath(".");
	}
	return true;
}

#include "imageviewer.moc"
