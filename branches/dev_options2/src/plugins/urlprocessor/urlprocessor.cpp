#include "urlprocessor.h"

#include <QAuthenticator>
#include <utils/logger.h>

UrlProcessor::UrlProcessor() : QNetworkAccessManager(NULL)
{
	FConnectionManager = NULL;

	connect(this, SIGNAL(proxyAuthenticationRequired(const QNetworkProxy &, QAuthenticator *)),
		SLOT(onProxyAuthenticationRequired(const QNetworkProxy &, QAuthenticator *)));
}

UrlProcessor::~UrlProcessor()
{

}

void UrlProcessor::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("URL processor");
	APluginInfo->description = tr("Allows other plugins to load data from custom types of URLs");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Road Works Software";
	APluginInfo->homePage = "http://www.eyecu.ru";
}

bool UrlProcessor::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);

	IPlugin *plugin = APluginManager->pluginInterface("IConnectionManager").value(0,NULL);
	if (plugin)
	{
		FConnectionManager = qobject_cast<IConnectionManager *>(plugin->instance());
		if (FConnectionManager)
			connect(FConnectionManager->instance(),SIGNAL(defaultProxyChanged(const QUuid &)),SLOT(onDefaultConnectionProxyChanged(const QUuid &)));
	}

	return true;
}

bool UrlProcessor::registerUrlHandler(const QString &AScheme, IUrlHandler *AUrlHandler)
{
	if (!AScheme.isEmpty() && AUrlHandler!=NULL)
	{
		LOG_DEBUG(QString("Url handler registered, cheme=%1, address=%2").arg(AScheme).arg((quint64)AUrlHandler));
		FHandlerList.insertMulti(AScheme, AUrlHandler);
		return true;
	}
	return false;
}

QNetworkAccessManager *UrlProcessor::networkAccessManager()
{
	return this;
}

QNetworkReply *UrlProcessor::createRequest(QNetworkAccessManager::Operation AOperation, const QNetworkRequest &ARequerst, QIODevice *AOutData)
{
	foreach (IUrlHandler *urlHandler, FHandlerList.values(ARequerst.url().scheme()))
	{
		QNetworkReply *reply = urlHandler->request(AOperation, ARequerst, AOutData);
		if (reply) 
			return reply;
	}
	return QNetworkAccessManager::createRequest(AOperation, ARequerst, AOutData);
}

void UrlProcessor::onDefaultConnectionProxyChanged(const QUuid &AProxyId)
{
	setProxy(FConnectionManager->proxyById(AProxyId).proxy);
}

void UrlProcessor::onProxyAuthenticationRequired(const QNetworkProxy &AProxy, QAuthenticator *AAuth)
{
	AAuth->setUser(AProxy.user());
	AAuth->setPassword(AProxy.password());
}

Q_EXPORT_PLUGIN2(plg_urlprocessor, UrlProcessor)
