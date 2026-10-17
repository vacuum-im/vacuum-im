#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <QUuid>
#include <QString>
#include <QImage>
#include <QNetworkAccessManager>
#include <interfaces/imessagewidgets.h>

#define REAL_IMAGE_URL_PROPERTY 1001

#define IMAGEVIEWER_UUID "{B7D4E8F2-3A1C-4D5E-9F0B-8C7A6D5E4F3A}"

struct PendingImage {
	QUrl url;
	QString placeholder;
};

class ImageViewer :
	public QObject,
	public IPlugin,
	public IMessageStyleHandler
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IMessageStyleHandler);
	Q_PLUGIN_METADATA(IID "org.vacuum-im.plugins.ImageViewer");
public:
	ImageViewer();
	~ImageViewer();
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

private:
	IMessageWidgets *FMessageWidgets;
	QNetworkAccessManager *FNetworkManager;
	IPluginManager *FPluginManager;
	QString FCacheDir;
	
	QImage downloadImageSync(const QUrl &AUrl);
	void insertImagesInDocument(QTextDocument *doc, const QList<PendingImage> &pendingImages);
	QString getCacheFilePath(const QUrl &AUrl);
	bool initCacheDir();
};

#endif // IMAGEVIEWER_H
