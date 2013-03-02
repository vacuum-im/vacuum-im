#ifndef URLPROCESSOR_H
#define URLPROCESSOR_H

#include <QNetworkAccessManager>
#include <interfaces/ipluginmanager.h>
#include <interfaces/iurlprocessor.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/iconnectionmanager.h>

class UrlProcessor : 
	public QNetworkAccessManager, 
	public IPlugin, 
	public IUrlProcessor, 
	public IOptionsHolder
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IUrlProcessor IOptionsHolder);
public:
	UrlProcessor(QObject *AParent = 0);
	// IPlugin
	QObject *instance() {return this;}
	virtual QUuid pluginUuid() const { return URLPROCESSOR_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin() { return true; }
	// IOptionsHolder
	virtual QMultiMap<int, IOptionsWidget *> optionsWidgets(const QString &ANodeId, QWidget *AParent);
	// IUrlProcessor
	virtual QNetworkAccessManager *networkAccessManager();
	virtual bool registerUrlHandler(const QString &AScheme, IUrlHandler *AUrlHandler);
protected:
	// QNetworkAccessManager
	virtual QNetworkReply *createRequest(QNetworkAccessManager::Operation AOperation, const QNetworkRequest &ARequest, QIODevice *AOutData=NULL);
protected slots:
	void onOptionsOpened();
	void onOptionsChanged(const OptionsNode &ANode);
	void onProxyAuthenticationRequired(const QNetworkProxy &AProxy, QAuthenticator *AAuth);
private:
	IOptionsManager *FOptionsManager;
	IConnectionManager *FConnectionManager;
private:
	QMultiMap<QString, IUrlHandler *> FHandlerList;
};

#endif // URLPROCESSOR_H
