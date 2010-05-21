#include "connectionmanager.h"

ConnectionManager::ConnectionManager()
{
	FEncryptedLabelId = -1;
	FAccountManager = NULL;
	FRostersViewPlugin = NULL;
	FOptionsManager = NULL;
}

ConnectionManager::~ConnectionManager()
{

}

void ConnectionManager::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Connection Manager");
	APluginInfo->description = tr("Allows to use different types of connections to a Jabber server");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
}

bool ConnectionManager::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
	QList<IPlugin *> plugins = APluginManager->pluginInterface("IConnectionPlugin");
	foreach (IPlugin *plugin, plugins)
	{
		IConnectionPlugin *cplugin = qobject_cast<IConnectionPlugin *>(plugin->instance());
		if (cplugin)
		{
			FPlugins.insert(cplugin->pluginId(), cplugin);
			connect(cplugin->instance(),SIGNAL(connectionCreated(IConnection *)),SIGNAL(connectionCreated(IConnection *)));
			connect(cplugin->instance(),SIGNAL(connectionDestroyed(IConnection *)),SIGNAL(connectionDestroyed(IConnection *)));
		}
	}

	IPlugin *plugin = APluginManager->pluginInterface("IAccountManager").value(0,NULL);
	if (plugin)
	{
		FAccountManager = qobject_cast<IAccountManager *>(plugin->instance());
		if (FAccountManager)
		{
			connect(FAccountManager->instance(),SIGNAL(shown(IAccount *)),SLOT(onAccountShown(IAccount *)));
			connect(FAccountManager->instance(),SIGNAL(changed(IAccount *, const OptionsNode &)),
			        SLOT(onAccountOptionsChanged(IAccount *, const OptionsNode &)));
		}
	}

	plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
	if (plugin)
	{
		FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IOptionsManager").value(0,NULL);
	if (plugin)
	{
		FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
	if (plugin)
	{
		IXmppStreams *xmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
		if (xmppStreams)
		{
			connect(xmppStreams->instance(),SIGNAL(opened(IXmppStream *)),SLOT(onStreamOpened(IXmppStream *)));
			connect(xmppStreams->instance(),SIGNAL(closed(IXmppStream *)),SLOT(onStreamClosed(IXmppStream *)));
		}
	}

	connect(Options::instance(),SIGNAL(optionsOpened()),SLOT(onOptionsOpened()));

	return !FPlugins.isEmpty();
}

bool ConnectionManager::initObjects()
{
	Options::setDefaultValue(OPV_ACCOUNT_CONNECTION_TYPE,QString("DefaultConnection"));

	Options::setDefaultValue(OPV_PROXY_DEFAULT,QString(APPLICATION_PROXY_REF_UUID));
	Options::setDefaultValue(OPV_PROXY_NAME,tr("New Proxy"));
	Options::setDefaultValue(OPV_PROXY_TYPE,(int)QNetworkProxy::NoProxy);

	if (FRostersViewPlugin)
	{
		QIcon icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_CONNECTION_ENCRYPTED);
		FEncryptedLabelId = FRostersViewPlugin->rostersView()->createIndexLabel(RLO_CONNECTION_ENCRYPTED,icon);
	}
	return true;
}

bool ConnectionManager::initSettings()
{
	if (FAccountManager && FOptionsManager)
	{
		FOptionsManager->insertOptionsHolder(this);
	}
	return true;
}

IOptionsWidget *ConnectionManager::optionsWidget(const QString &ANodeId, int &AOrder, QWidget *AParent)
{
	QStringList nodeTree = ANodeId.split(".",QString::SkipEmptyParts);
	if (nodeTree.count()==2 && nodeTree.at(0)==OPN_ACCOUNTS)
	{
		AOrder = OWO_ACCOUNT_CONNECTION;
		return new ConnectionOptionsWidget(this,Options::node(OPV_ACCOUNT_ITEM,nodeTree.at(1)),AParent);
	}
	return NULL;
}

QList<QString> ConnectionManager::pluginList() const
{
	return FPlugins.keys();
}

IConnectionPlugin *ConnectionManager::pluginById(const QString &APluginId) const
{
	return FPlugins.value(APluginId,NULL);
}

QList<QUuid> ConnectionManager::proxyList() const
{
	QList<QUuid> plist;
	foreach(QString proxyId, Options::node(OPV_PROXY_ROOT).childNSpaces("proxy")) {
		plist.append(proxyId); }
	return plist;
}

IConnectionProxy ConnectionManager::proxyById(const QUuid &AProxyId) const
{
	static const IConnectionProxy noProxy = {" "+tr("<No Proxy>"), QNetworkProxy(QNetworkProxy::NoProxy) };

	if (!AProxyId.isNull())
	{
		OptionsNode pnode;
		QList<QUuid> plist = proxyList();
		if (plist.contains(AProxyId))
			pnode = Options::node(OPV_PROXY_ITEM,AProxyId.toString());
		else if (plist.contains(defaultProxy()))
			pnode = Options::node(OPV_PROXY_ITEM,defaultProxy().toString());

		if (!pnode.isNull())
		{
			IConnectionProxy proxy;
			proxy.name = pnode.value("name").toString();
			proxy.proxy.setType((QNetworkProxy::ProxyType)pnode.value("type").toInt());
			proxy.proxy.setHostName(pnode.value("host").toString());
			proxy.proxy.setPort(pnode.value("port").toInt());
			proxy.proxy.setUser(pnode.value("user").toString());
			proxy.proxy.setPassword(Options::decrypt(pnode.value("pass").toByteArray()).toString());
			return proxy;
		}
	}
	return noProxy;
}

void ConnectionManager::setProxy(const QUuid &AProxyId, const IConnectionProxy &AProxy)
{
	if (!AProxyId.isNull() && AProxyId!=APPLICATION_PROXY_REF_UUID)
	{
		OptionsNode pnode = Options::node(OPV_PROXY_ITEM,AProxyId.toString());
		pnode.setValue(AProxy.name,"name");
		pnode.setValue(AProxy.proxy.type(),"type");
		pnode.setValue(AProxy.proxy.hostName(),"host");
		pnode.setValue(AProxy.proxy.port(),"port");
		pnode.setValue(AProxy.proxy.user(),"user");
		pnode.setValue(Options::encrypt(AProxy.proxy.password()),"pass");
		emit proxyChanged(AProxyId, AProxy);
	}
}

void ConnectionManager::removeProxy(const QUuid &AProxyId)
{
	if (proxyList().contains(AProxyId))
	{
		if (defaultProxy() == AProxyId)
			setDefaultProxy(QUuid());
		Options::node(OPV_PROXY_ROOT).removeChilds("proxy",AProxyId.toString());
		emit proxyRemoved(AProxyId);
	}
}

QUuid ConnectionManager::defaultProxy() const
{
	return Options::node(OPV_PROXY_DEFAULT).value().toString();
}

void ConnectionManager::setDefaultProxy(const QUuid &AProxyId)
{
	if (defaultProxy()!=AProxyId && (AProxyId.isNull() || proxyList().contains(AProxyId)))
	{
		Options::node(OPV_PROXY_DEFAULT).setValue(AProxyId.toString());
		QNetworkProxy::setApplicationProxy(proxyById(AProxyId).proxy);
		emit defaultProxyChanged(AProxyId);
	}
}

QDialog *ConnectionManager::showEditProxyDialog(QWidget *AParent)
{
	EditProxyDialog *dialog = new EditProxyDialog(this, AParent);
	dialog->show();
	return dialog;
}

IOptionsWidget *ConnectionManager::proxySettingsWidget(const OptionsNode &ANode, QWidget *AParent)
{
	ProxySettingsWidget *widget = new ProxySettingsWidget(this,ANode,AParent);
	return widget;
}

void ConnectionManager::saveProxySettings(IOptionsWidget *AWidget, OptionsNode ANode)
{
	ProxySettingsWidget *widget = qobject_cast<ProxySettingsWidget *>(AWidget->instance());
	if (widget)
		widget->apply(ANode);
}

QUuid ConnectionManager::loadProxySettings(const OptionsNode &ANode) const
{
	return ANode.value().toString();
}

IConnection *ConnectionManager::updateAccountConnection(IAccount *AAccount) const
{
	if (AAccount->isActive())
	{
		OptionsNode aoptions = AAccount->optionsNode();
		QString pluginId = aoptions.value("connection-type").toString();
		IConnectionPlugin *plugin = FPlugins.contains(pluginId) ? FPlugins.value(pluginId) : FPlugins.values().value(0);
		IConnection *connection = AAccount->xmppStream()->connection();
		if (connection && connection->ownerPlugin()!=plugin)
		{
			AAccount->xmppStream()->setConnection(NULL);
			delete connection->instance();
			connection = NULL;
		}
		if (plugin!=NULL && connection==NULL)
		{
			connection = plugin->newConnection(aoptions.node("connection",pluginId),AAccount->xmppStream()->instance());
			AAccount->xmppStream()->setConnection(connection);
		}
		return connection;
	}
	return NULL;
}

void ConnectionManager::onAccountShown(IAccount *AAccount)
{
	updateAccountConnection(AAccount);
}

void ConnectionManager::onAccountOptionsChanged(IAccount *AAccount, const OptionsNode &ANode)
{
	const OptionsNode &aoptions = AAccount->optionsNode();
	if (aoptions.childPath(ANode) == "connection-type")
	{
		updateAccountConnection(AAccount);
	}
	else if (AAccount->isActive() && AAccount->xmppStream()->connection())
	{
		OptionsNode coptions = aoptions.node("connection",aoptions.value("connection-type").toString());
		if (coptions.isChildNode(ANode))
		{
			IConnectionPlugin *plugin = pluginById(coptions.nspace());
			if (plugin)
				plugin->loadConnectionSettings(AAccount->xmppStream()->connection(), coptions);
		}
	}
}

void ConnectionManager::onStreamOpened(IXmppStream *AXmppStream)
{
	if (FRostersViewPlugin && AXmppStream->connection() && AXmppStream->connection()->isEncrypted())
	{
		IRostersModel *model = FRostersViewPlugin->rostersView()->rostersModel();
		IRosterIndex *index = model!=NULL ? model->streamRoot(AXmppStream->streamJid()) : NULL;
		if (index!=NULL)
			FRostersViewPlugin->rostersView()->insertIndexLabel(FEncryptedLabelId,index);
	}
}

void ConnectionManager::onStreamClosed(IXmppStream *AXmppStream)
{
	if (FRostersViewPlugin)
	{
		IRostersModel *model = FRostersViewPlugin->rostersView()->rostersModel();
		IRosterIndex *index = model!=NULL ? model->streamRoot(AXmppStream->streamJid()) : NULL;
		if (index!=NULL)
			FRostersViewPlugin->rostersView()->removeIndexLabel(FEncryptedLabelId,index);
	}
}

void ConnectionManager::onOptionsOpened()
{
	QNetworkProxy::setApplicationProxy(proxyById(defaultProxy()).proxy);
}

Q_EXPORT_PLUGIN2(plg_connectionmanager, ConnectionManager)
