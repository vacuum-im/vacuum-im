#include "urlprocessor.h"
#include <definitions/optionwidgetorders.h>
#include <definitions/optionnodeorders.h>
#include <definitions/optionnodes.h>
#include <definitions/optionvalues.h>

#include <QAuthenticator>

UrlProcessor::UrlProcessor(QObject *AParent) : QNetworkAccessManager(AParent)
{
	FOptionsManager = NULL;
	FConnectionManager = NULL;

	connect(this, SIGNAL(proxyAuthenticationRequired(const QNetworkProxy &, QAuthenticator *)),
		SLOT(onProxyAuthenticationRequired(const QNetworkProxy &, QAuthenticator *)));
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
		FConnectionManager = qobject_cast<IConnectionManager *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IOptionsManager").value(0,NULL);
	if (plugin)
		FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());

	connect(Options::instance(), SIGNAL(optionsOpened()), SLOT(onOptionsOpened()));
	connect(Options::instance(), SIGNAL(optionsChanged(const OptionsNode &)), SLOT(onOptionsChanged(const OptionsNode &)));

	return true;
}

bool UrlProcessor::initObjects()
{
	return true;
}

bool UrlProcessor::initSettings()
{
	Options::setDefaultValue(OPV_MISC_URLPROXY, APPLICATION_PROXY_REF_UUID);

	//if (FOptionsManager)
	//	FOptionsManager->insertOptionsHolder(this);

	return true;
}

QMultiMap<int, IOptionsWidget *> UrlProcessor::optionsWidgets(const QString &ANodeId, QWidget *AParent)
{
	QMultiMap<int, IOptionsWidget *> widgets;
	if (ANodeId == OPN_MISC)
	{
		if (FConnectionManager)
			widgets.insertMulti(OWO_MISC_URLPROXY, FConnectionManager->proxySettingsWidget(Options::node(OPV_MISC_URLPROXY), AParent));
	}
	return widgets;
}

QNetworkAccessManager *UrlProcessor::networkAccessManager()
{
	return this;
}

bool UrlProcessor::registerUrlHandler(const QString &AScheme, IUrlHandler *AUrlHandler)
{
	if (!AScheme.isEmpty() && AUrlHandler!=NULL)
	{
		FHandlerList.insertMulti(AScheme, AUrlHandler);
		return true;
	}
	return false;
}

QNetworkReply *UrlProcessor::createRequest(QNetworkAccessManager::Operation AOperation, const QNetworkRequest &ARequerst, QIODevice *AOutData)
{
	foreach (IUrlHandler *urlHandler, FHandlerList.values(ARequerst.url().scheme())) // Iterate thru a list of handlers, asssociated with the scheme
	{                                                                                // Try every handler:
		QNetworkReply *reply = urlHandler->request(AOperation, ARequerst, AOutData);  // Get reply
		if (reply) 
			return reply;                                                              // If repy received, return it!
	}
	return QNetworkAccessManager::createRequest(AOperation, ARequerst, AOutData);    // Fallback to parent createRequext() procedure!
}

void UrlProcessor::onOptionsOpened()
{
	onOptionsChanged(Options::node(OPV_MISC_URLPROXY));
}

void UrlProcessor::onOptionsChanged(const OptionsNode &ANode)
{
	if(ANode.path() == OPV_MISC_URLPROXY) // Proxy
	{
		if (FConnectionManager)
			setProxy(FConnectionManager->proxyById(ANode.value().toString()).proxy);
	}
}

void UrlProcessor::onProxyAuthenticationRequired(const QNetworkProxy &AProxy, QAuthenticator *AAuth)
{
	AAuth->setUser(AProxy.user());
	AAuth->setPassword(AProxy.password());
}

Q_EXPORT_PLUGIN2(plg_urlprocessor, UrlProcessor)
