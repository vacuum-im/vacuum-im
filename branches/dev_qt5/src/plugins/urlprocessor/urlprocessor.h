#ifndef URLPROCESSOR_H
#define URLPROCESSOR_H

#include <QNetworkAccessManager>
#include <interfaces/ipluginmanager.h>
#include <interfaces/iurlprocessor.h>
#include <interfaces/iconnectionmanager.h>

class UrlProcessor : 
	public QNetworkAccessManager, 
	public IPlugin, 
	public IUrlProcessor
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IUrlProcessor);
#ifdef HAVE_QT5
	Q_PLUGIN_METADATA(IID "org.jrudevels.vacuum.IUrlProcessor");
#endif
public:
	UrlProcessor();
	~UrlProcessor();
	// IPlugin
	QObject *instance() {return this;}
	virtual QUuid pluginUuid() const { return URLPROCESSOR_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects() { return true; }
	virtual bool initSettings() { return true; }
	virtual bool startPlugin() { return true; }
	// IUrlProcessor
	virtual bool registerUrlHandler(const QString &AScheme, IUrlHandler *AUrlHandler);
protected:
	// QNetworkAccessManager
	virtual QNetworkAccessManager *networkAccessManager();
	virtual QNetworkReply *createRequest(QNetworkAccessManager::Operation AOperation, const QNetworkRequest &ARequest, QIODevice *AOutData=NULL);
protected slots:
	void onDefaultConnectionProxyChanged(const QUuid &AProxyId);
	void onProxyAuthenticationRequired(const QNetworkProxy &AProxy, QAuthenticator *AAuth);
private:
	IConnectionManager *FConnectionManager;
private:
	QMultiMap<QString, IUrlHandler *> FHandlerList;
};

#endif // URLPROCESSOR_H
