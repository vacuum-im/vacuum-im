#include "imageviewer.h"

#include <QTextBlock>
#include <QTextCursor>
#include <QTextFormat>
#include <QImageReader>
#include <QBuffer>
#include <QRegularExpression>
#include <QCryptographicHash>
#include <QTimer>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTextBrowser>
#include <QSpinBox>
#include <QHostAddress>
#include <QHostInfo>
#include <QAbstractSocket>
#include <QSaveFile>
#include <QStandardPaths>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileDevice>
#include <QPointer>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrentRun>
#include <algorithm>

#include <definitions/messageviewurlhandlerorders.h>
#include <definitions/optionnodes.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/optionvalues.h>
#include <definitions/resources.h>

#include <interfaces/ipluginmanager.h>
#include <interfaces/ioptionsmanager.h>
#include <utils/pluginhelper.h>
#include <utils/textmanager.h>
#include <utils/options.h>
#include <utils/logger.h>

namespace
{
QString htmlAttribute(const QString &AValue)
{
	return AValue.toHtmlEscaped();
}

// Decodes the character references a html attribute value may carry. Only the
// references the html writers emit are handled: the named references of the
// xml special characters, nbsp and the numeric references. Anything else is
// kept as it is, a full html parser is not needed for this.
QString decodedHtmlAttribute(const QString &AValue)
{
	QString result;
	result.reserve(AValue.size());
	for (int i = 0; i < AValue.size();)
	{
		if (AValue.at(i) != QLatin1Char('&') || i + 2 >= AValue.size())
		{
			result.append(AValue.at(i));
			++i;
			continue;
		}
		// The reference is bounded with a semicolon close by, otherwise the
		// ampersand is a plain character.
		const int semicolon = AValue.indexOf(QLatin1Char(';'), i + 2);
		if (semicolon < 0 || semicolon - i > 10)
		{
			result.append(AValue.at(i));
			++i;
			continue;
		}
		const QString reference = AValue.mid(i + 1, semicolon - i - 1);
		QChar decoded;
		bool known = true;
		if (reference == QLatin1String("amp"))
			decoded = QLatin1Char('&');
		else if (reference == QLatin1String("lt"))
			decoded = QLatin1Char('<');
		else if (reference == QLatin1String("gt"))
			decoded = QLatin1Char('>');
		else if (reference == QLatin1String("quot"))
			decoded = QLatin1Char('"');
		else if (reference == QLatin1String("apos"))
			decoded = QLatin1Char('\'');
		else if (reference == QLatin1String("nbsp"))
			decoded = QChar(0x00a0);
		else if (reference.at(0) == QLatin1Char('#'))
		{
			bool ok = false;
			const uint code = reference.length() > 1 && (reference.at(1) == QLatin1Char('x') || reference.at(1) == QLatin1Char('X'))
				? reference.mid(2).toUInt(&ok, 16)
				: reference.mid(1).toUInt(&ok, 10);
			if (ok && code <= 0xffff && !(code >= 0xd800 && code <= 0xdfff))
				decoded = QChar(code);
			else
				known = false;
		}
		else
			known = false;
		if (known)
		{
			result.append(decoded);
			i = semicolon + 1;
		}
		else
		{
			result.append(AValue.at(i));
			++i;
		}
	}
	return result;
}
} // namespace

ImageViewer::ImageViewer()
	: FPluginManager(nullptr), FMessageWidgets(nullptr), FMessageContextManager(nullptr), FOptionsManager(nullptr),
	  FCacheManager(new CacheManager(this)), FNetworkManager(new NetworkManager(this))
{
}

ImageViewer::~ImageViewer()
{
	if (FMessageWidgets)
		FMessageWidgets->removeStyleHandler(MVUHO_IMAGEVIEWER, this);
}

void ImageViewer::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Image Viewer");
	APluginInfo->description = tr("Automatically displays images referenced by URLs in messages. "
		"Only the simple message style is supported: the Adium style is based on QtWebKit, "
		"which is going to be dropped, and is not supported.");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Vacuum-im Team";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(MESSAGEWIDGETS_UUID);
	APluginInfo->dependences.append(OPTIONSMANAGER_UUID);
}

bool ImageViewer::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);

	FPluginManager = APluginManager;

	IPlugin *plugin = FPluginManager->pluginInterface("IMessageWidgets").value(0, nullptr);
	if (plugin)
	{
		FMessageWidgets = qobject_cast<IMessageWidgets *>(plugin->instance());
		FMessageContextManager = qobject_cast<IMessageDocumentContextManager *>(plugin->instance());
	}

	plugin = FPluginManager->pluginInterface("IOptionsManager").value(0, nullptr);
	if (plugin)
		FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());

	return true;
}

bool ImageViewer::initObjects()
{
	if (!FMessageWidgets || !FMessageContextManager)
	{
		LOG_WARNING("ImageViewer requires MessageWidgets message-context support; image display disabled");
		return false;
	}

	QString cachePath = FPluginManager->cachePath();
	if (cachePath.isEmpty() || !FCacheManager->init(cachePath))
	{
		LOG_WARNING("ImageViewer cache unavailable, image display disabled");
		return false;
	}
	LOG_INFO(QString("ImageViewer cache directory = %1").arg(FCacheManager->dir()));

	if (FMessageWidgets)
	{
		FMessageWidgets->insertStyleHandler(MVUHO_IMAGEVIEWER, this);
		LOG_INFO("ImageViewer style handler registered");
	}

	return true;
}

bool ImageViewer::initSettings()
{
	Options::setDefaultValue(OPV_MESSAGES_IMAGES_AUTOLOAD, false);
	Options::setDefaultValue(OPV_MESSAGES_IMAGES_BLOCKPRIVATEIPS, true);
	Options::setDefaultValue(OPV_MESSAGES_IMAGES_MAXDOWNLOADMB, IV_DEFAULT_MAX_DOWNLOAD_MB);
	Options::setDefaultValue(OPV_MESSAGES_IMAGES_MAXDIMENSION, IV_DEFAULT_MAX_DIMENSION);
	Options::setDefaultValue(OPV_MESSAGES_IMAGES_PREVIEWDIMENSION, IV_DEFAULT_PREVIEW_DIMENSION);
	Options::setDefaultValue(OPV_MESSAGES_IMAGES_CACHE_MAXBYTES, IV_DEFAULT_CACHE_MAX_BYTES);
	Options::setDefaultValue(OPV_MESSAGES_IMAGES_CACHE_MAXAGE_DAYS, IV_DEFAULT_CACHE_MAX_AGE_DAYS);
	Options::setDefaultValue(OPV_MESSAGES_IMAGES_CACHE_MAXCOUNT, IV_DEFAULT_CACHE_MAX_COUNT);

	if (FOptionsManager)
		FOptionsManager->insertOptionsDialogHolder(this);

	return true;
}

QMultiMap<int, IOptionsDialogWidget *> ImageViewer::optionsDialogWidgets(const QString &ANodeId, QWidget *AParent)
{
	QMultiMap<int, IOptionsDialogWidget *> widgets;
	if (FOptionsManager && ANodeId == OPN_MESSAGES)
	{
		widgets.insertMulti(OHO_MESSAGES_IMAGES, FOptionsManager->newOptionsDialogHeader(tr("Images"), AParent));
		widgets.insertMulti(OWO_MESSAGES_IMAGES_AUTOLOAD, FOptionsManager->newOptionsDialogWidget(Options::node(OPV_MESSAGES_IMAGES_AUTOLOAD), tr("Automatically load images from message URLs"), AParent));
		widgets.insertMulti(OWO_MESSAGES_IMAGES_BLOCKPRIVATEIPS, FOptionsManager->newOptionsDialogWidget(Options::node(OPV_MESSAGES_IMAGES_BLOCKPRIVATEIPS), tr("Block images from private or local IP addresses"), AParent));
		QSpinBox *dlSpin = new QSpinBox(AParent);
		dlSpin->setRange(1, 256);
		dlSpin->setSuffix(QStringLiteral(" ") + tr("MB", "Megabyte"));
		dlSpin->setValue(Options::node(OPV_MESSAGES_IMAGES_MAXDOWNLOADMB).value().toInt());
		widgets.insertMulti(OWO_MESSAGES_IMAGES_MAXDOWNLOADMB, FOptionsManager->newOptionsDialogWidget(Options::node(OPV_MESSAGES_IMAGES_MAXDOWNLOADMB), tr("Maximum download size per image:"), dlSpin, AParent));
		QSpinBox *dimSpin = new QSpinBox(AParent);
		dimSpin->setRange(256, 16384);
		dimSpin->setSingleStep(64);
		dimSpin->setSuffix(" px");
		dimSpin->setValue(Options::node(OPV_MESSAGES_IMAGES_MAXDIMENSION).value().toInt());
		widgets.insertMulti(OWO_MESSAGES_IMAGES_MAXDIMENSION, FOptionsManager->newOptionsDialogWidget(Options::node(OPV_MESSAGES_IMAGES_MAXDIMENSION), tr("Maximum decoded image dimension:"), dimSpin, AParent));
		QSpinBox *previewSpin = new QSpinBox(AParent);
		previewSpin->setRange(IV_MIN_PREVIEW_DIMENSION, IV_MAX_PREVIEW_DIMENSION);
		previewSpin->setSingleStep(16);
		previewSpin->setSuffix(" px");
		previewSpin->setValue(FCacheManager->previewDimension());
		widgets.insertMulti(OWO_MESSAGES_IMAGES_PREVIEWDIMENSION, FOptionsManager->newOptionsDialogWidget(Options::node(OPV_MESSAGES_IMAGES_PREVIEWDIMENSION), tr("Preview image size (longest side):"), previewSpin, AParent));
	}
	return widgets;
}

bool ImageViewer::isAutoLoadEnabled() const
{
	return Options::node(OPV_MESSAGES_IMAGES_AUTOLOAD).value().toBool();
}

bool ImageViewer::isImageable(const QUrl &AUrl) const
{
	const QString scheme = AUrl.scheme().toLower();
	if (scheme == QStringLiteral("data"))
	{
		// Vector images are never fetched or shown: the svg payload is
		// rejected later anyway, fetching it would be a wasted download.
		return AUrl.toString().startsWith(QStringLiteral("data:image/"), Qt::CaseInsensitive) &&
			   !AUrl.toString().startsWith(QStringLiteral("data:image/svg"), Qt::CaseInsensitive);
	}
	if (scheme != QStringLiteral("http") && scheme != QStringLiteral("https"))
	{
		return false;
	}
	const QString path = AUrl.path().toLower();
	// svg and svgz are deliberately absent: an svg document is never shown
	// (it can carry external references), so it is not downloaded either.
	static const QRegularExpression extensionRegex(
		QStringLiteral(R"REGEX(\.(?:jpe?g|png|gif|webp|bmp|tiff?|ico|jfif)(?:$|/))REGEX"),
		QRegularExpression::CaseInsensitiveOption);
	if (extensionRegex.match(path).hasMatch())
	{
		return true;
	}

	// // Keep the heuristic conservative: only fetch likely media endpoints, not
	// // every hyperlink whose path happens to contain "img" or "image".
	// static const QRegularExpression mediaPathRegex(
	// 	QStringLiteral(R"REGEX((?:^|/)(?:images?|imgs?|photos?|avatars?|thumbnails?|thumb|media)(?:/|$))REGEX"),
	// 	QRegularExpression::CaseInsensitiveOption);
	// if (mediaPathRegex.match(path).hasMatch())
	// 	return true;

	const QString query = AUrl.query().toLower();
	static const QRegularExpression formatQueryRegex(
		QStringLiteral(R"REGEX((?:^|[&;])(?:format|fm|ext|filetype)=(?:jpe?g|png|gif|webp|bmp|jfif)(?:[&;]|$))REGEX"),
		QRegularExpression::CaseInsensitiveOption);
	return formatQueryRegex.match(query).hasMatch();
}

QByteArray ImageViewer::decodeDataUrlBytes(const QString &ADataUrl, qint64 AMaxBytes)
{
	if (AMaxBytes <= 0 || !ADataUrl.startsWith(QStringLiteral("data:image/"), Qt::CaseInsensitive))
		return QByteArray();
	const int comma = ADataUrl.indexOf(QLatin1Char(','));
	if (comma < 0 || comma > 128)
		return QByteArray();
	const QString metadata = ADataUrl.mid(5, comma - 5);
	const QString payload = ADataUrl.mid(comma + 1);
	const bool base64 = metadata.endsWith(QStringLiteral(";base64"), Qt::CaseInsensitive);
	const qint64 maxEncoded = base64 ? ((AMaxBytes + 2) / 3) * 4 + 8 : AMaxBytes * 3;
	if (payload.size() > maxEncoded)
		return QByteArray();
	const QByteArray bytes = base64 ? QByteArray::fromBase64(payload.toLatin1())
									: QByteArray::fromPercentEncoding(payload.toUtf8());
	return bytes.size() <= AMaxBytes ? bytes : QByteArray();
}

// Makes the link of an image short: only the name of the file is shown, the
// domain, the path and the parameters of the url are too long.
QString ImageViewer::shortenImageUrl(const QUrl &AUrl) const
{
	if (AUrl.scheme().compare(QStringLiteral("data"), Qt::CaseInsensitive) == 0)
	{
		// An inline image has no name, the type of it is shown instead.
		const QString text = AUrl.toString(QUrl::FullyEncoded);
		const int comma = text.indexOf(QLatin1Char(','));
		return comma > 0 ? text.left(comma) + QStringLiteral("...") : text;
	}

	const QString path = AUrl.path(QUrl::FullyDecoded);
	const int slash = path.lastIndexOf(QLatin1Char('/'));
	if (slash >= 0 && slash + 1 < path.length())
		return path.mid(slash + 1);
	return AUrl.host();
}

// Replaces the text of the link of the message with the given one, the link
// itself is kept. The other messages may have links with the same href, so
// only the links placed after the marker of the message are taken into account.
// The search is limited to the message range, same as in findAnchorBlock(), so
// that a fragment straddling a block boundary is never attributed to the wrong
// message.
bool ImageViewer::shortenLinkInDocument(QTextDocument *ADoc, const QString &AHref, const QString &AText, const Gallery &AGallery) const
{
	if (!ADoc || AHref.isEmpty() || AText.isEmpty() ||
		AGallery.context.document != ADoc || !FMessageContextManager ||
		!FMessageContextManager->isMessageContextValid(AGallery.context))
		return false;

	const int startPos = AGallery.context.atStart ? 0 : AGallery.context.marker.position() + 1;
	const int endPos = FMessageContextManager->nextMessagePosition(AGallery.context);
	for (QTextBlock block = ADoc->findBlock(startPos); block.isValid(); block = block.next())
	{
		if (endPos >= 0 && block.position() >= endPos)
			break;

		QList<QTextFragment> fragments;
		for (QTextBlock::Iterator it = block.begin(); !it.atEnd(); ++it)
		{
			const QTextFragment &fragment = it.fragment();
			const int fragStart = fragment.position();
			const int fragEnd = fragStart + fragment.length();
			if (fragEnd <= startPos)
				continue;
			if (endPos >= 0 && fragStart >= endPos)
				break;
			// Consider only fragments that fully belong to this message.
			if (fragStart < startPos || (endPos >= 0 && fragEnd > endPos))
				continue;

			const QTextCharFormat format = fragment.charFormat();
			if (!format.isAnchor() || format.anchorHref() != AHref)
				continue;
			// The images of the gallery are links to the same url, their text
			// must not be replaced.
			if (format.isImageFormat())
				continue;
			fragments.append(fragment);
		}

		// The first link of the block that is not shortened yet
		int index = -1;
		for (int i = 0; i < fragments.count(); ++i)
		{
			if (fragments[i].text() != AText)
			{
				index = i;
				break;
			}
		}
		if (index >= 0)
		{
			// The link may be split into a few fragments, all of them are replaced.
			int first = index;
			int last = index;
			while (first > 0 && fragments[first - 1].position() + fragments[first - 1].length() == fragments[first].position())
				--first;
			while (last + 1 < fragments.count() && fragments[last].position() + fragments[last].length() == fragments[last + 1].position())
				++last;

			QTextCharFormat format = fragments[index].charFormat();
			// The full url is kept in the format, the text of the link is
			// shortened for the display, but the full url is copied.
			format.setProperty(TMI_IMAGE_LINK_URL, AHref);
			QTextCursor cur(ADoc);
			cur.setPosition(fragments[first].position());
			cur.setPosition(fragments[last].position() + fragments[last].length(), QTextCursor::KeepAnchor);
			cur.setCharFormat(format);
			cur.insertText(AText);
			return true;
		}
	}
	// The link is not found: the message was trimmed or the view was closed.
	return false;
}

// Finds the block of the link of the message with the given href. The search is
// limited to the range of this message (from its marker to the marker of the
// next message), so that a link with the same href in another message is never
// matched by mistake. Fragment-level position checks handle blocks that straddle
// a message boundary.
bool ImageViewer::findAnchorBlock(QTextDocument *ADoc, const QString &AHref, const Gallery &AGallery, QTextBlock &ABlock) const
{
	if (!ADoc || AHref.isEmpty() || AGallery.context.document != ADoc ||
		!FMessageContextManager || !FMessageContextManager->isMessageContextValid(AGallery.context))
		return false;

	const int startPos = AGallery.context.atStart ? 0 : AGallery.context.marker.position() + 1;
	const int endPos = FMessageContextManager->nextMessagePosition(AGallery.context);
	for (QTextBlock block = ADoc->findBlock(startPos); block.isValid(); block = block.next())
	{
		if (endPos >= 0 && block.position() >= endPos)
			break;
		for (QTextBlock::Iterator it = block.begin(); !it.atEnd(); ++it)
		{
			const QTextFragment &fragment = it.fragment();
			const int fragStart = fragment.position();
			const int fragEnd = fragStart + fragment.length();
			// Skip fragments entirely before this message's range.
			if (fragEnd <= startPos)
				continue;
			// Fragments at/after the next message boundary belong to it.
			if (endPos >= 0 && fragStart >= endPos)
				break;
			// Reject a fragment that straddles the boundary: it can't be
			// uniquely attributed to this message.
			if (endPos >= 0 && fragEnd > endPos)
				continue;
			if (fragStart < startPos)
				continue;
			const QTextCharFormat format = fragment.charFormat();
			if (format.isAnchor() && format.anchorHref() == AHref)
			{
				ABlock = block;
				return true;
			}
		}
	}
	return false;
}

// Verifies that the message owning a pending insert is still present in the
// document when an asynchronous download completes. MessageWidgets invalidates
// the context on reset or history trimming; the source link is also checked
// within the message's own range.
bool ImageViewer::isMessagePresent(QTextDocument *ADoc, const PendingInsert &AInsert) const
{
	if (!ADoc || !AInsert.gallery || AInsert.gallery->context.document != ADoc ||
		!FMessageContextManager || !FMessageContextManager->isMessageContextValid(AInsert.gallery->context))
		return false;
	QTextBlock block;
	return findAnchorBlock(ADoc, AInsert.href, *AInsert.gallery, block);
}

void ImageViewer::registerGalleryResources(QTextDocument *ADoc) const
{
	if (!ADoc)
		return;
	// A transparent image used as the distance between the images of the gallery.
	QPixmap spacer(IV_GALLERY_SPACING, 1);
	spacer.fill(Qt::transparent);
	ADoc->addResource(QTextDocument::ImageResource, QUrl(IV_GALLERY_SPACER_URL), spacer);
}

QString ImageViewer::galleryImageHtml(const QString &APreviewPath, const QUrl &AUrl, const QString &AName) const
{
	const QString imageUrl = QUrl::fromLocalFile(APreviewPath).toString(QUrl::FullyEncoded);
	// The image is inline, so that the images of the gallery are placed in a row.
	// The previews are already scaled to the limit when they are made, the rich
	// text engine of Qt does not support max-width or height:auto anyway.
	const QString image = QStringLiteral("<img src=\"%1\" alt=\"%2\" style=\"vertical-align:top;\">")
			.arg(htmlAttribute(imageUrl), htmlAttribute(AName));
	if (AUrl.scheme().compare(QStringLiteral("data"), Qt::CaseInsensitive) == 0)
		return image;
	// The image is a link to the original one, so it can be opened by a click.
	return QStringLiteral("<a href=\"%1\">%2</a>")
			.arg(htmlAttribute(AUrl.toString(QUrl::FullyEncoded)), image);
}

// Adds the ready preview to the gallery of the message. The gallery is placed
// at the end of the message and holds up to IV_GALLERY_ROW_IMAGES images in a row.
bool ImageViewer::addImageToGallery(QTextDocument *ADoc, const QSharedPointer<Gallery> &AGallery, int ASlot, const QString &AHref, const QString &APreviewPath)
{
	if (!ADoc || !AGallery || APreviewPath.isEmpty())
		return false;

	if (!AGallery->started)
	{
		QTextBlock block;
		// Only the links of this message are taken into account, the other
		// messages may have links with the same href.
		if (!findAnchorBlock(ADoc, AHref, *AGallery, block))
			return false;
		AGallery->anchorBlock = block;

		// The gallery is placed at the end of the frame of the message, so that
		// it does not break the text of the message.
		QTextFrame *frame = ADoc->frameAt(block.position());
		AGallery->frame = (frame && frame != ADoc->rootFrame()) ? frame : nullptr;

		QTextCursor cur(ADoc);
		if (AGallery->frame)
		{
			// The gallery is a new block at the end of the frame of the message.
			const QTextBlock lastBlock = AGallery->frame->lastCursorPosition().block();
			if (!lastBlock.isValid())
				return false;
			cur.setPosition(lastBlock.position() + lastBlock.length() - 1);
			cur.insertBlock();
			AGallery->galleryBlock = cur.block();
		}
		else
		{
			// The message is not in a frame, so the messages of the document may
			// share a block. The gallery is placed at the end of the message,
			// which is the position before the next message.
			const int nextPosition = FMessageContextManager
				? FMessageContextManager->nextMessagePosition(AGallery->context) : -1;
			if (nextPosition >= 0)
			{
				cur.setPosition(nextPosition);
				// The content of the next message goes to a new block and the
				// gallery gets a block of its own.
				cur.insertBlock();
				cur.insertBlock();
				AGallery->galleryBlock = cur.block().previous();

				// The styles end a message with a line break, which is at the
				// start of the next message. The block of the gallery replaces
				// it, so the next message is not moved down by an empty line.
				const QTextBlock nextBlock = cur.block();
				int pos = nextBlock.position();
				const int end = nextBlock.position() + nextBlock.length() - 1;
				while (pos < end)
				{
					QTextCursor character(ADoc);
					character.setPosition(pos);
					character.setPosition(pos + 1, QTextCursor::KeepAnchor);
					const QString text = character.selectedText();
					if (text == QChar(QChar::LineSeparator))
					{
						character.removeSelectedText();
						continue;
					}
					if (!text.at(0).isSpace())
						break;
					pos++;
				}
			}
			else
			{
				// The message is the last one, the gallery is a new block after
				// the link.
				cur.setPosition(block.position() + block.length() - 1);
				cur.insertBlock();
				AGallery->galleryBlock = cur.block();
			}
		}
		AGallery->started = true;
	}

	AGallery->previews.insert(ASlot, APreviewPath);
	AGallery->urls.insert(ASlot, QUrl(AHref));
	rebuildGallery(ADoc, AGallery);
	return true;
}

// Fills the gallery with the ready previews, in the order of the links of the message.
void ImageViewer::rebuildGallery(QTextDocument *ADoc, const QSharedPointer<Gallery> &AGallery)
{
	if (!ADoc || !AGallery || !AGallery->started)
		return;

	const QTextBlock block = AGallery->galleryBlock;
	if (!block.isValid())
		return;
	if (AGallery->frame)
	{
		// The message must not be removed from the document.
		if (AGallery->frame->lastCursorPosition().block() != block)
			return;
	}
	else if (!AGallery->anchorBlock.isValid())
	{
		return;
	}

	// The previous content of the gallery is dropped.
	QTextCursor cur(ADoc);
	cur.setPosition(block.position());
	cur.setPosition(block.position() + block.length() - 1, QTextCursor::KeepAnchor);
	cur.removeSelectedText();

	// The images are placed one after another, the rows are made by the layout
	// of the text when they do not fit into the width of the message.
	bool first = true;
	for (int slot = 0; slot < AGallery->linkCount; ++slot)
	{
		const QString path = AGallery->previews.value(slot);
		if (path.isEmpty())
			continue;
		if (!first)
		{
			// The distance between the images.
			cur.insertHtml(QStringLiteral("<img src=\"%1\">").arg(IV_GALLERY_SPACER_URL));
		}
		const QUrl url = AGallery->urls.value(slot);
		cur.insertHtml(galleryImageHtml(path, url, shortenImageUrl(url)));
		first = false;
	}

	// The images are placed at the top of the lines, so the height of the lines
	// makes the distance between the rows of the gallery.
	int maxHeight = 0;
	for (auto it = AGallery->previews.constBegin(); it != AGallery->previews.constEnd(); ++it)
	{
		QImageReader reader(it.value());
		const QSize size = reader.size();
		if (size.isValid())
			maxHeight = qMax(maxHeight, size.height());
	}
	if (maxHeight > 0)
	{
		QTextBlockFormat format = block.blockFormat();
		format.setLineHeight(maxHeight + IV_GALLERY_SPACING, QTextBlockFormat::MinimumHeight);
		QTextCursor blockCursor(block);
		blockCursor.setBlockFormat(format);
	}
}

void ImageViewer::embedPreview(const PendingInsert &AInsert, const QUrl &ADisplayUrl, const QString &APreviewPath)
{
	// The link of the image is shortened and the preview is placed at the
	// end of the message.
	shortenLinkInDocument(AInsert.doc.data(), AInsert.href, shortenImageUrl(ADisplayUrl), *AInsert.gallery);
	addImageToGallery(AInsert.doc.data(), AInsert.gallery, AInsert.slot, AInsert.href, APreviewPath);
}

void ImageViewer::scheduleEmbed(const PendingInsert &AInsert, const QUrl &AUrl, const QByteArray &AData, const QString &AKnownPreviewPath)
{
	QTimer::singleShot(0, this, [this, AInsert, AUrl, AData, AKnownPreviewPath]()
					   {
		if (!AInsert.doc)
			return;
		if (!isMessagePresent(AInsert.doc.data(), AInsert))
			return;
		if (!AKnownPreviewPath.isEmpty())
		{
			embedPreview(AInsert, AUrl, AKnownPreviewPath);
			return;
		}
		schedulePreviewJob(AInsert, AData, FCacheManager->filePath(AUrl), AUrl); });
}

void ImageViewer::scheduleDataUrlEmbed(const PendingInsert &AInsert, const QString &ADataUrl)
{
	QTimer::singleShot(0, this, [this, AInsert, ADataUrl]()
					   {
		if (!AInsert.doc)
			return;
		if (!isMessagePresent(AInsert.doc.data(), AInsert))
			return;
		const QByteArray data = decodeDataUrlBytes(ADataUrl, FCacheManager->maxDownloadBytes());
		if (data.isEmpty())
			return;
		schedulePreviewJob(AInsert, data, FCacheManager->dataUrlFilePath(ADataUrl, data), QUrl(ADataUrl)); });
}

// The decoding and the scaling of the image may take a noticeable time for the
// big images, they are made outside the gui thread. Everything that touches
// the cache manifest, the options or the document stays on the gui thread.
void ImageViewer::schedulePreviewJob(const PendingInsert &AInsert, const QByteArray &AData, const QString &ACachePath, const QUrl &ADisplayUrl)
{
	// The limits are read here, the worker thread must not touch the options.
	const int maxDimension = FCacheManager->maxDimension();
	const int previewDimension = FCacheManager->previewDimension();
	const qint64 maxBytes = FCacheManager->maxDownloadBytes();

	auto *watcher = new QFutureWatcher<CacheManager::PreviewResult>(this);
	connect(watcher, &QFutureWatcherBase::finished, this, [this, watcher, AInsert, ADisplayUrl, AData, ACachePath]()
			{
		watcher->deleteLater();
		// The message may have been removed while the image was being decoded.
		if (!AInsert.doc || !isMessagePresent(AInsert.doc.data(), AInsert))
			return;
		const CacheManager::PreviewResult result = watcher->result();
		QString previewPath;
		if (FCacheManager->isAvailable() && result.valid &&
			FCacheManager->applyPreview(ACachePath, AData, result.previewPng, previewPath))
		{
			FCacheManager->prune();
		}
		if (!previewPath.isEmpty())
			embedPreview(AInsert, ADisplayUrl, previewPath); });
	watcher->setFuture(QtConcurrent::run([AData, maxDimension, maxBytes, previewDimension]()
		{ return CacheManager::makePreviewData(AData, maxDimension, maxBytes, previewDimension); }));
}

void ImageViewer::startNetworkOrCacheDownload(const QString &AUrlStr, const QUrl &AUrl, const PendingInsert &AInsert)
{
	if (FNetworkManager->isHostBlocked(AUrl))
	{
		// The link of the message is left as it is.
		LOG_INFO(QString("ImageViewer: blocked host for %1").arg(AUrlStr));
		return;
	}

	// The cached image is shown without a download.
	if (FCacheManager->isAvailable())
	{
		const QString cachePath = FCacheManager->filePath(AUrl);
		const QByteArray cached = FCacheManager->readImage(cachePath);
		if (!cached.isEmpty())
		{
			// A matching preview means the image was valid when it was cached,
			// in this common case it is not decoded again.
			QString previewPath;
			if (FCacheManager->reusePreview(cachePath, cached, previewPath))
			{
				FCacheManager->touch(cachePath, cached.size());
				FCacheManager->touch(previewPath, QFileInfo(previewPath).size());
				FCacheManager->saveManifest();
				scheduleEmbed(AInsert, AUrl, cached, previewPath);
				return;
			}
			if (FCacheManager->validateImageData(cached))
			{
				FCacheManager->touch(cachePath, cached.size());
				FCacheManager->saveManifest();
				scheduleEmbed(AInsert, AUrl, cached);
				return;
			}
			// The cached file is broken, it is removed with its preview.
			FCacheManager->dropImage(cachePath);
			FCacheManager->saveManifest();
		}
	}

	FNetworkManager->request(AUrlStr, AUrl, FCacheManager->maxDownloadBytes(), [this, AInsert, AUrl](const QByteArray &AData)
							  {
		// The links of the failed images are left as they are.
		if (AData.isEmpty())
			return;
		scheduleEmbed(AInsert, AUrl, AData); });
}

bool ImageViewer::messageStyleContentInsert(int AOrder, QWidget *AWidget, const QString &AHtml, QString &AModifiedHtml, const IMessageStyleContentOptions &AOptions)
{
	Q_UNUSED(AOptions);

	if (AOrder != MVUHO_IMAGEVIEWER)
		return false;

	QTextDocument *doc = nullptr;
	if (AWidget)
	{
		QTextBrowser *browser = qobject_cast<QTextBrowser *>(AWidget);
		doc = browser ? browser->document() : nullptr;
	}
	if (!doc)
	{
		AModifiedHtml = AHtml;
		return true;
	}

	// MessageWidgets owns message boundaries and their lifetime. Register every
	// message before the auto-load check, including messages without image URLs.
	MessageDocumentContext messageContext;
	if (!FMessageContextManager || !FMessageContextManager->beginMessageContext(doc, messageContext))
	{
		AModifiedHtml = AHtml;
		return true;
	}

	if (!isAutoLoadEnabled())
	{
		AModifiedHtml = AHtml;
		return true;
	}

	LOG_DEBUG(QString("ImageViewer: processing message html length=%1").arg(AHtml.length()));

	struct AnchorMatch
	{
		int start;
		int end;
		QString href;
	};
	QList<AnchorMatch> matches;
	const QRegularExpression anchorRegex(
		QStringLiteral(R"REGEX(<a\b(?:(?!>).)*?(?<![\w-])href\s*=\s*(?:"([^"]*)"|'([^']*)'|([^\s"'`=<>]+))(?:(?!>).)*?>.*?</a\s*>)REGEX"),
		QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
	auto it = anchorRegex.globalMatch(AHtml);
	while (it.hasNext())
	{
		const auto match = it.next();
		const QString rawHref = !match.captured(1).isNull() ? match.captured(1) : (!match.captured(2).isNull() ? match.captured(2) : match.captured(3));
		const QString href = decodedHtmlAttribute(rawHref).trimmed();
		const QUrl url(href);
		if (isImageable(url))
			matches.append({match.capturedStart(0), match.capturedEnd(0), href});
	}

	if (matches.isEmpty())
	{
		AModifiedHtml = AHtml;
		return true;
	}

	LOG_DEBUG(QString("ImageViewer: found %1 image URL(s)").arg(matches.size()));

	// The links of the message are left untouched. The images are placed at the
	// end of the message and the links are shortened when the image is ready.
	QSharedPointer<Gallery> gallery(new Gallery());
	gallery->linkCount = matches.size();
	gallery->context = messageContext;
	registerGalleryResources(doc);

	for (int i = 0; i < matches.size(); ++i)
	{
		const AnchorMatch &match = matches[i];
		QUrl url(match.href);
		QString scheme = url.scheme().toLower();

		PendingInsert insert;
		insert.href = match.href;
		insert.slot = i;
		insert.doc = QPointer<QTextDocument>(doc);
		insert.gallery = gallery;

		if (scheme == "data")
		{
			// Inline images are cached and previewed like the downloaded ones,
			// so that no image data is stored in the message html.
			scheduleDataUrlEmbed(insert, match.href);
		}
		else if (scheme == "http" || scheme == "https")
		{
			startNetworkOrCacheDownload(match.href, url, insert);
		}
	}

	AModifiedHtml = AHtml;
	return true;
}
