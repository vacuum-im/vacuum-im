#include "defaultconnectionplugin.h"

DefaultConnectionPlugin::DefaultConnectionPlugin()
{
	FXmppStreams = NULL;
	FOptionsManager = NULL;
	FConnectionManager = NULL;
}

DefaultConnectionPlugin::~DefaultConnectionPlugin()
{

}

void DefaultConnectionPlugin::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Default Connection");
	APluginInfo->description = tr("Allows to set a standard TCP connection to Jabber server");
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->version = "1.0";
	APluginInfo->homePage = "http://www.vacuum-im.org";
}

bool DefaultConnectionPlugin::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
	IPlugin *plugin = APluginManager->pluginInterface("IOptionsManager").value(0,NULL);
	if (plugin)
		FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
	if (plugin)
		FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IConnectionManager").value(0,NULL);
	if (plugin)
		FConnectionManager = qobject_cast<IConnectionManager *>(plugin->instance());

	return true;
}

bool DefaultConnectionPlugin::initSettings()
{
	Options::setDefaultValue(OPV_ACCOUNT_CONNECTION_HOST,QString());
	Options::setDefaultValue(OPV_ACCOUNT_CONNECTION_PORT,5222);
	Options::setDefaultValue(OPV_ACCOUNT_CONNECTION_PROXY,QString(APPLICATION_PROXY_REF_UUID));
	Options::setDefaultValue(OPV_ACCOUNT_CONNECTION_USESSL,false);
	Options::setDefaultValue(OPV_ACCOUNT_CONNECTION_IGNORESSLERRORS,true);
	return true;
}

QString DefaultConnectionPlugin::pluginId() const
{
	static const QString id = "DefaultConnection";
	return id;
}

QString DefaultConnectionPlugin::pluginName() const
{
	return tr("Default Connection");
}

IConnection *DefaultConnectionPlugin::newConnection(const OptionsNode &ANode, QObject *AParent)
{
	DefaultConnection *connection = new DefaultConnection(this,AParent);
	connect(connection,SIGNAL(aboutToConnect()),SLOT(onConnectionAboutToConnect()));
	connect(connection,SIGNAL(connectionDestroyed()),SLOT(onConnectionDestroyed()));
	loadConnectionSettings(connection,ANode);
	FCleanupHandler.add(connection);
	emit connectionCreated(connection);
	return connection;
}

IOptionsWidget *DefaultConnectionPlugin::connectionSettingsWidget(const OptionsNode &ANode, QWidget *AParent)
{
	return new ConnectionOptionsWidget(FConnectionManager, ANode, AParent);
}

void DefaultConnectionPlugin::saveConnectionSettings(IOptionsWidget *AWidget, OptionsNode ANode)
{
	ConnectionOptionsWidget *widget = qobject_cast<ConnectionOptionsWidget *>(AWidget->instance());
	if (widget)
		widget->apply(ANode);
}

void DefaultConnectionPlugin::loadConnectionSettings(IConnection *AConnection, const OptionsNode &ANode)
{
	IDefaultConnection *connection = qobject_cast<IDefaultConnection *>(AConnection->instance());
	if (connection)
	{
		connection->setOption(IDefaultConnection::COR_HOST,ANode.value("host").toString());
		connection->setOption(IDefaultConnection::COR_PORT,ANode.value("port").toInt());
		connection->setOption(IDefaultConnection::COR_USE_SSL,ANode.value("use-ssl").toBool());
		connection->setOption(IDefaultConnection::COR_IGNORE_SSL_ERRORS,ANode.value("ignore-ssl-errors").toBool());
		if (FConnectionManager)
			connection->setProxy(FConnectionManager->proxyById(FConnectionManager->loadProxySettings(ANode.node("proxy"))).proxy);
	}
}

void DefaultConnectionPlugin::onConnectionAboutToConnect()
{
	DefaultConnection *connection = qobject_cast<DefaultConnection*>(sender());
	if (FXmppStreams && connection)
	{
		foreach(IXmppStream *stream, FXmppStreams->xmppStreams())
		if (stream->connection() == connection)
			connection->setOption(IDefaultConnection::COR_DOMAINE,stream->streamJid().pDomain());
	}
}

void DefaultConnectionPlugin::onConnectionDestroyed()
{
	IConnection *connection = qobject_cast<IConnection *>(sender());
	if (connection)
		emit connectionDestroyed(connection);
}

Q_EXPORT_PLUGIN2(plg_defaultconnection, DefaultConnectionPlugin)
