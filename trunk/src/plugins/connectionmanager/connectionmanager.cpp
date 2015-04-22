#include "connectionmanager.h"

#include <QDir>
#include <QSslSocket>
#include <QTextDocument>
#include <definitions/optionvalues.h>
#include <definitions/optionnodes.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/rosterlabels.h>
#include <definitions/rosterindexroles.h>
#include <definitions/rostertooltiporders.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/internalerrors.h>
#include <utils/widgetmanager.h>
#include <utils/filestorage.h>
#include <utils/xmpperror.h>
#include <utils/logger.h>

#define DIR_CERTIFICATES   "cacertificates"

ConnectionManager::ConnectionManager()
{
	FPluginManager = NULL;
	FXmppStreamManager = NULL;
	FAccountManager = NULL;
	FRostersViewPlugin = NULL;
	FOptionsManager = NULL;

	FEncryptedLabelId = 0;
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

bool ConnectionManager::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
	FPluginManager = APluginManager;

	IPlugin *plugin = APluginManager->pluginInterface("IAccountManager").value(0,NULL);
	if (plugin)
	{
		FAccountManager = qobject_cast<IAccountManager *>(plugin->instance());
		if (FAccountManager)
		{
			connect(FAccountManager->instance(),SIGNAL(accountActiveChanged(IAccount *, bool)),SLOT(onAccountActiveChanged(IAccount *, bool)));
			connect(FAccountManager->instance(),SIGNAL(accountOptionsChanged(IAccount *, const OptionsNode &)),SLOT(onAccountOptionsChanged(IAccount *, const OptionsNode &)));
		}
	}

	plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
	if (plugin)
	{
		FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
		if (FRostersViewPlugin)
		{
			connect(FRostersViewPlugin->rostersView()->instance(),SIGNAL(indexToolTips(IRosterIndex *, quint32, QMap<int,QString> &)),
				SLOT(onRosterIndexToolTips(IRosterIndex *, quint32, QMap<int,QString> &)));
		}
	}

	plugin = APluginManager->pluginInterface("IOptionsManager").value(0,NULL);
	if (plugin)
	{
		FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IXmppStreamManager").value(0,NULL);
	if (plugin)
	{
		FXmppStreamManager = qobject_cast<IXmppStreamManager *>(plugin->instance());
	}

	connect(Options::instance(),SIGNAL(optionsOpened()),SLOT(onOptionsOpened()));
	connect(Options::instance(),SIGNAL(optionsChanged(const OptionsNode &)),SLOT(onOptionsChanged(const OptionsNode &)));

	return true;
}

bool ConnectionManager::initObjects()
{
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_CONNECTIONMANAGER_CONNECT_ERROR,tr("Connection error"));

	if (FRostersViewPlugin)
	{
		AdvancedDelegateItem label(RLID_CONNECTION_ENCRYPTED);
		label.d->kind = AdvancedDelegateItem::CustomData;
		label.d->data = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_CONNECTION_ENCRYPTED);
		FEncryptedLabelId = FRostersViewPlugin->rostersView()->registerLabel(label);
	}

	if (FOptionsManager)
	{
		FOptionsManager->insertOptionsDialogHolder(this);
	}

	return true;
}

bool ConnectionManager::initSettings()
{
	Options::setDefaultValue(OPV_ACCOUNT_CONNECTION_TYPE,QString("DefaultConnection"));

	Options::setDefaultValue(OPV_PROXY_DEFAULT,QString());
	Options::setDefaultValue(OPV_PROXY_NAME,tr("New Proxy"));
	Options::setDefaultValue(OPV_PROXY_TYPE,(int)QNetworkProxy::NoProxy);

	return true;
}

QMultiMap<int, IOptionsDialogWidget *> ConnectionManager::optionsDialogWidgets(const QString &ANodeId, QWidget *AParent)
{
	QMultiMap<int, IOptionsDialogWidget *> widgets;
	QStringList nodeTree = ANodeId.split(".",QString::SkipEmptyParts);
	if (nodeTree.count()==3 && nodeTree.at(0)==OPN_ACCOUNTS && nodeTree.at(2)=="Parameters")
	{
		widgets.insertMulti(OHO_ACCOUNTS_PARAMS_CONNECTION,FOptionsManager->newOptionsDialogHeader(tr("Connection"),AParent));
		widgets.insertMulti(OWO_ACCOUNTS_PARAMS_CONNECTION, new ConnectionOptionsWidget(this,Options::node(OPV_ACCOUNT_ITEM,nodeTree.at(1)),AParent));
	}
	else if (ANodeId == OPN_ACCOUNTS)
	{
		widgets.insertMulti(OWO_ACCOUNTS_DEFAULTPROXY, proxySettingsWidget(Options::node(OPV_PROXY_DEFAULT),AParent));
	}
	return widgets;
}

QList<QUuid> ConnectionManager::proxyList() const
{
	QList<QUuid> plist;
	foreach(const QString &proxyId, Options::node(OPV_PROXY_ROOT).childNSpaces("proxy"))
		plist.append(proxyId);
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
		LOG_INFO(QString("Proxy added or updated, id=%1").arg(AProxyId.toString()));
		OptionsNode pnode = Options::node(OPV_PROXY_ITEM,AProxyId.toString());
		pnode.setValue(AProxy.name,"name");
		pnode.setValue(AProxy.proxy.type(),"type");
		pnode.setValue(AProxy.proxy.hostName(),"host");
		pnode.setValue(AProxy.proxy.port(),"port");
		pnode.setValue(AProxy.proxy.user(),"user");
		pnode.setValue(Options::encrypt(AProxy.proxy.password()),"pass");
		emit proxyChanged(AProxyId, AProxy);
	}
	else
	{
		LOG_ERROR(QString("Failed to add or change proxy, id=%1: Invalid proxy Id").arg(AProxyId.toString()));
	}
}

void ConnectionManager::removeProxy(const QUuid &AProxyId)
{
	if (proxyList().contains(AProxyId))
	{
		LOG_INFO(QString("Proxy removed, id=%1").arg(AProxyId.toString()));
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
		LOG_INFO(QString("Default proxy changed, id=%1").arg(AProxyId.toString()));
		Options::node(OPV_PROXY_DEFAULT).setValue(AProxyId.toString());
	}
}

QDialog *ConnectionManager::showEditProxyDialog(QWidget *AParent)
{
	EditProxyDialog *dialog = new EditProxyDialog(this, AParent);
	dialog->show();
	return dialog;
}

IOptionsDialogWidget *ConnectionManager::proxySettingsWidget(const OptionsNode &ANode, QWidget *AParent)
{
	ProxySettingsWidget *widget = new ProxySettingsWidget(this,ANode,AParent);
	return widget;
}

void ConnectionManager::saveProxySettings(IOptionsDialogWidget *AWidget, OptionsNode ANode)
{
	ProxySettingsWidget *widget = qobject_cast<ProxySettingsWidget *>(AWidget->instance());
	if (widget)
		widget->apply(ANode);
}

QUuid ConnectionManager::loadProxySettings(const OptionsNode &ANode) const
{
	return ANode.value().toString();
}

QList<QSslCertificate> ConnectionManager::trustedCaCertificates(bool AWithUsers) const
{
	QList<QSslCertificate> certs;

	QList<QString> certDirs = FileStorage::resourcesDirs();
	if (AWithUsers)
		certDirs += FPluginManager->homePath();

	foreach(const QString &certDir, certDirs)
	{
		QDir dir(certDir);
		if (dir.cd(DIR_CERTIFICATES))
		{
			foreach(const QString &certFile, dir.entryList(QDir::Files))
			{
				QFile file(dir.absoluteFilePath(certFile));
				if (file.open(QFile::ReadOnly))
				{
					QSslCertificate cert(&file,QSsl::Pem);
					if (!cert.isNull())
						certs.append(cert);
					else
						LOG_ERROR(QString("Failed to load CA certificate from file=%1: Invalid format").arg(file.fileName()));
				}
				else
				{
					REPORT_ERROR(QString("Failed to load CA certificate from file: %1").arg(file.errorString()));
				}
			}
		}
	}
	return certs;
}

void ConnectionManager::addTrustedCaCertificate(const QSslCertificate &ACertificate)
{
	QDir dir(FPluginManager->homePath());
	if ((dir.exists(DIR_CERTIFICATES) || dir.mkdir(DIR_CERTIFICATES)) && dir.cd(DIR_CERTIFICATES))
	{
		QString certFile = QString::fromLocal8Bit(ACertificate.digest().toHex())+".pem";
		if (!ACertificate.isNull() && !dir.exists(certFile))
		{
			QFile file(dir.absoluteFilePath(certFile));
			if (file.open(QFile::WriteOnly|QFile::Truncate))
			{
				LOG_INFO(QString("Saved trusted CA certificate to file=%1").arg(file.fileName()));
				file.write(ACertificate.toPem());
				file.close();
			}
			else
			{
				REPORT_ERROR(QString("Failed to save trusted CA certificate to file: %1").arg(file.errorString()));
			}
		}
	}
}

QList<QString> ConnectionManager::connectionEngines() const
{
	return FEngines.keys();
}

IConnectionEngine *ConnectionManager::findConnectionEngine(const QString &AEngineId) const
{
	return FEngines.value(AEngineId);
}

void ConnectionManager::registerConnectionEngine(IConnectionEngine *AEngine)
{
	if (AEngine)
	{
		FEngines.insert(AEngine->engineId(), AEngine);
		connect(AEngine->instance(),SIGNAL(connectionCreated(IConnection *)),SLOT(onConnectionCreated(IConnection *)),Qt::UniqueConnection);
		connect(AEngine->instance(),SIGNAL(connectionDestroyed(IConnection *)),SLOT(onConnectionDestroyed(IConnection *)),Qt::UniqueConnection);
		emit connectionEngineRegistered(AEngine);
	}
}

void ConnectionManager::updateAccountConnection(IAccount *AAccount) const
{
	if (AAccount->isActive())
	{
		OptionsNode aoptions = AAccount->optionsNode();
		QString engineId = aoptions.value("connection-type").toString();
		IConnectionEngine *engine = FEngines.contains(engineId) ? FEngines.value(engineId) : FEngines.values().value(0);
		IConnection *connection = AAccount->xmppStream()->connection();
		if (connection && connection->engine()!=engine)
		{
			LOG_STRM_INFO(AAccount->streamJid(),QString("Removing current stream connection"));
			AAccount->xmppStream()->setConnection(NULL);
			delete connection->instance();
			connection = NULL;
		}
		if (engine!=NULL && connection==NULL)
		{
			LOG_STRM_INFO(AAccount->streamJid(),QString("Setting new stream connection=%1").arg(engine->engineId()));
			connection = engine->newConnection(aoptions.node("connection",engineId),AAccount->xmppStream()->instance());
			AAccount->xmppStream()->setConnection(connection);
		}
	}
}

void ConnectionManager::updateConnectionSettings(IAccount *AAccount) const
{
	QList<IAccount *> accountList;
	if (AAccount != NULL)
		accountList.append(AAccount);
	else if (FAccountManager != NULL)
		accountList = FAccountManager->accounts();
		
	foreach(IAccount *account, accountList)
	{
		if (account->isActive() && account->xmppStream()->connection()!=NULL)
		{
			const OptionsNode &aoptions = account->optionsNode();
			const OptionsNode &coptions = aoptions.node("connection",aoptions.value("connection-type").toString());
			IConnectionEngine *engine = findConnectionEngine(coptions.nspace());
			if (engine)
				engine->loadConnectionSettings(account->xmppStream()->connection(), coptions);
		}
	}
}

IXmppStream *ConnectionManager::findConnectionStream(IConnection *AConnection) const
{
	if (FXmppStreamManager && AConnection)
	{
		foreach(IXmppStream *stream, FXmppStreamManager->xmppStreams())
			if (stream->connection() == AConnection)
				return stream;
	}
	return NULL;
}

void ConnectionManager::onConnectionEncrypted()
{
	IConnection *connection = qobject_cast<IConnection *>(sender());
	if (FRostersViewPlugin && connection)
	{
		IXmppStream *stream = findConnectionStream(connection);
		if (stream)
		{
			IRostersModel *model = FRostersViewPlugin->rostersView()->rostersModel();
			IRosterIndex *sindex = model!=NULL ? model->streamIndex(stream->streamJid()) : NULL;
			if (sindex)
				FRostersViewPlugin->rostersView()->insertLabel(FEncryptedLabelId,sindex);
		}
	}
}

void ConnectionManager::onConnectionDisconnected()
{
	IConnection *connection = qobject_cast<IConnection *>(sender());
	if (FRostersViewPlugin && connection)
	{
		IXmppStream *stream = findConnectionStream(connection);
		if (stream)
		{
			IRostersModel *model = FRostersViewPlugin->rostersView()->rostersModel();
			IRosterIndex *sindex = model!=NULL ? model->streamIndex(stream->streamJid()) : NULL;
			if (sindex)
				FRostersViewPlugin->rostersView()->removeLabel(FEncryptedLabelId,sindex);
		}
	}
}

void ConnectionManager::onConnectionCreated(IConnection *AConnection)
{
	connect(AConnection->instance(),SIGNAL(encrypted()),SLOT(onConnectionEncrypted()));
	connect(AConnection->instance(),SIGNAL(disconnected()),SLOT(onConnectionDisconnected()));
	emit connectionCreated(AConnection);
}

void ConnectionManager::onConnectionDestroyed(IConnection *AConnection)
{
	emit connectionDestroyed(AConnection);
}

void ConnectionManager::onOptionsOpened()
{
	onOptionsChanged(Options::node(OPV_PROXY_DEFAULT));
}

void ConnectionManager::onOptionsChanged(const OptionsNode &ANode)
{
	if (ANode.path() == OPV_PROXY_DEFAULT)
	{
		QUuid proxyId = ANode.value().toString();
		QNetworkProxy::setApplicationProxy(proxyById(proxyId).proxy);
		updateConnectionSettings();
		emit defaultProxyChanged(proxyId);
	}
	else if (Options::node(OPV_PROXY_ROOT).isChildNode(ANode))
	{
		updateConnectionSettings();
	}
}

void ConnectionManager::onAccountActiveChanged(IAccount *AAccount, bool AActive)
{
	if (AActive)
		updateAccountConnection(AAccount);
}

void ConnectionManager::onAccountOptionsChanged(IAccount *AAccount, const OptionsNode &ANode)
{
	const OptionsNode &aoptions = AAccount->optionsNode();
	const OptionsNode &coptions = aoptions.node("connection",aoptions.value("connection-type").toString());
	if (aoptions.childPath(ANode) == "connection-type")
		updateAccountConnection(AAccount);
	else if (coptions.isChildNode(ANode))
		updateConnectionSettings(AAccount);
}

void ConnectionManager::onRosterIndexToolTips(IRosterIndex *AIndex, quint32 ALabelId, QMap<int,QString> &AToolTips)
{
	if (ALabelId == FEncryptedLabelId)
	{
		IXmppStream *stream = FXmppStreamManager!=NULL ? FXmppStreamManager->findXmppStream(AIndex->data(RDR_STREAM_JID).toString()) : NULL;
		IConnection *connection = stream!=NULL ? stream->connection() : NULL;
		if (connection && !connection->hostCertificate().isNull())
		{
			static const struct { QSslCertificate::SubjectInfo info; QString name; } certInfoNames[] = {
				{ QSslCertificate::CommonName,             tr("Name: %1")           },
				{ QSslCertificate::Organization,           tr("Organization: %1")   },
				{ QSslCertificate::OrganizationalUnitName, tr("Subunit: %1")        },
				{ QSslCertificate::CountryName,            tr("Country: %1")        },
				{ QSslCertificate::LocalityName,           tr("Locality: %1")       },
				{ QSslCertificate::StateOrProvinceName,    tr("State/Province: %1") },
			};
			static const uint certInfoNamesCount = sizeof(certInfoNames)/sizeof(certInfoNames[0]);

			QStringList tooltips;
			QSslCertificate cert = connection->hostCertificate();

			tooltips += tr("<b>Certificate holder:</b>");
			for (uint i=0; i<certInfoNamesCount; i++)
			{
				QString value = cert.subjectInfo(certInfoNames[i].info);
				if (!value.isEmpty())
					tooltips += certInfoNames[i].name.arg(Qt::escape(value));
			}

			tooltips += "<br>" + tr("<b>Certificate issuer:</b>");
			for (uint i=0; i<certInfoNamesCount; i++)
			{
				QString value = cert.issuerInfo(certInfoNames[i].info);
				if (!value.isEmpty())
					tooltips += certInfoNames[i].name.arg(Qt::escape(value));
			}

			tooltips += "<br>" + tr("<b>Certificate details:</b>");
			tooltips += tr("Effective from: %1").arg(cert.effectiveDate().date().toString());
			tooltips += tr("Expired at: %1").arg(cert.expiryDate().date().toString());
			tooltips += tr("Serial number: %1").arg(QString::fromLocal8Bit(cert.serialNumber().toUpper()));

			AToolTips.insert(RTTO_CONNECTIONMANAGER_HOSTCERT,tooltips.join("<br>"));
		}
	}
}

Q_EXPORT_PLUGIN2(plg_connectionmanager, ConnectionManager)
