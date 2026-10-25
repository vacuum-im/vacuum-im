#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <QUuid>
#include <QString>
#include <QTextDocument>
#include <QTextBlock>
#include <QTextFrame>
#include <QTextCursor>
#include <QSharedPointer>
#include <QHash>
#include <QPointer>
#include <QByteArray>
#include <QUrl>

#include <interfaces/ipluginmanager.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/imessagedocumentcontext.h>
#include <interfaces/imessagestylemanager.h>
#include <interfaces/ioptionsmanager.h>

#include "cachemanager.h"
#include "networkmanager.h"

#define IMAGEVIEWER_UUID "{B7D4E8F2-3A1C-4D5E-9F0B-8C7A6D5E4F3A}"

// Distance between the images of the gallery
#define IV_GALLERY_SPACING           4
// Resource of the transparent image used as the distance between the images
#define IV_GALLERY_SPACER_URL        "ivgallery://spacer.png"

class ImageViewer :
    public QObject,
    public IPlugin,
    public IMessageStyleHandler,
    public IOptionsDialogHolder
{
    Q_OBJECT;
    Q_INTERFACES(IPlugin IMessageStyleHandler IOptionsDialogHolder);
    Q_PLUGIN_METADATA(IID "org.vacuum-im.plugins.ImageViewer");
public:
    ImageViewer();
    ~ImageViewer() override;
    // IPlugin
    virtual QObject *instance() { return this; }
    virtual QUuid pluginUuid() const { return QUuid(IMAGEVIEWER_UUID); }
    virtual void pluginInfo(IPluginInfo *APluginInfo);
    virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
    virtual bool initObjects();
    virtual bool initSettings();
    virtual bool startPlugin() { return true; }
    // IMessageStyleHandler
    virtual bool messageStyleContentInsert(int AOrder, QWidget *AWidget, const QString &AHtml, QString &AModifiedHtml, const IMessageStyleContentOptions &AOptions);
    // IOptionsDialogHolder
    virtual QMultiMap<int, IOptionsDialogWidget *> optionsDialogWidgets(const QString &ANodeId, QWidget *AParent);
private:
    struct Gallery
    {
        // Amount of the image links of the message
        int linkCount = 0;
        // The ready previews by the slot of the link
        QHash<int, QString> previews;
        // The original urls of the links by the slot
        QHash<int, QUrl> urls;
        // The frame of the message the gallery belongs to
        QPointer<QTextFrame> frame;
        // The block of the link the gallery is placed after
        QTextBlock anchorBlock;
        // The block the gallery images are placed in
        QTextBlock galleryBlock;
        // MessageWidgets-owned identity and boundary information.
        MessageDocumentContext context;
        bool started = false;
    };
    struct PendingInsert
    {
        // The link of the image as it is in the message
        QString href;
        // The position of the link in the message
        int slot = 0;
        QPointer<QTextDocument> doc;
        QSharedPointer<Gallery> gallery;
    };
private:
    bool isAutoLoadEnabled() const;
    bool isImageable(const QUrl &AUrl) const;
    static QByteArray decodeDataUrlBytes(const QString &ADataUrl, qint64 AMaxBytes);
    // Makes the link of an image short
    QString shortenImageUrl(const QUrl &AUrl) const;
    // Replaces the text of the link of the message with the given one
    bool shortenLinkInDocument(QTextDocument *ADoc, const QString &AHref, const QString &AText, const Gallery &AGallery) const;
    // Finds the block of the link of the message with the given href
    bool findAnchorBlock(QTextDocument *ADoc, const QString &AHref, const Gallery &AGallery, QTextBlock &ABlock) const;
    // Places the preview at the end of the message
    bool addImageToGallery(QTextDocument *ADoc, const QSharedPointer<Gallery> &AGallery, int ASlot, const QString &AHref, const QString &APreviewPath);
    void rebuildGallery(QTextDocument *ADoc, const QSharedPointer<Gallery> &AGallery);
    void registerGalleryResources(QTextDocument *ADoc) const;
    QString galleryImageHtml(const QString &APreviewPath, const QUrl &AUrl, const QString &AName) const;
	// Caches the downloaded image and shows it. A known reusable preview path
	// skips the decoding worker entirely.
	void scheduleEmbed(const PendingInsert &AInsert, const QUrl &AUrl, const QByteArray &AData, const QString &AKnownPreviewPath = QString());
	void scheduleDataUrlEmbed(const PendingInsert &AInsert, const QString &ADataUrl);
	// Validates the image and makes its preview outside the gui thread, then
	// applies the result to the cache and shows it on the gui thread.
	void schedulePreviewJob(const PendingInsert &AInsert, const QByteArray &AData, const QString &ACachePath, const QUrl &ADisplayUrl);
	// Shortens the link of the image and adds the ready preview to the gallery.
	void embedPreview(const PendingInsert &AInsert, const QUrl &ADisplayUrl, const QString &APreviewPath);
    // Shows the cached image or downloads it
    void startNetworkOrCacheDownload(const QString &AUrlStr, const QUrl &AUrl, const PendingInsert &AInsert);
    // Checks that the message owning a pending insert is still present in the
    // document when an asynchronous download completes.
    bool isMessagePresent(QTextDocument *ADoc, const PendingInsert &AInsert) const;
private:
    IPluginManager *FPluginManager;
    IMessageWidgets *FMessageWidgets;
    IMessageDocumentContextManager *FMessageContextManager;
    IOptionsManager *FOptionsManager;
    CacheManager *FCacheManager;
    NetworkManager *FNetworkManager;
};

#endif // IMAGEVIEWER_H
