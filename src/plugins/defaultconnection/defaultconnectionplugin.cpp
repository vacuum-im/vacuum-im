#include "defaultconnectionplugin.h"

#include <QPushButton>
#include <QMessageBox>

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
	Options::setDefaultValue(OPV_ACCOUNT_CONNECTION_USELEGACYSSL,false);
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
	connect(connection,SIGNAL(sslErrorsOccured(const QList<QSslError> &)),
		SLOT(onConnectionSSLErrorsOccured(const QList<QSslError> &)));
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
		connection->setOption(IDefaultConnection::COR_USE_LEGACY_SSL,ANode.value("use-legacy-ssl").toBool());
		if (FConnectionManager)
			connection->setProxy(FConnectionManager->proxyById(FConnectionManager->loadProxySettings(ANode.node("proxy"))).proxy);
	}
}

IXmppStream *DefaultConnectionPlugin::findXmppStream(IConnection *AConnection) const
{
	if (FXmppStreams && AConnection)
	{
		foreach(IXmppStream *stream, FXmppStreams->xmppStreams())
			if (stream->connection() == AConnection)
				return stream;
	}
	return NULL;
}

void DefaultConnectionPlugin::onConnectionAboutToConnect()
{
	DefaultConnection *connection = qobject_cast<DefaultConnection*>(sender());
	IXmppStream *stream = findXmppStream(connection);
	if (connection && stream)
		connection->setOption(IDefaultConnection::COR_DOMAINE,stream->streamJid().pDomain());
}

void DefaultConnectionPlugin::onConnectionSSLErrorsOccured(const QList<QSslError> &AErrors)
{
	DefaultConnection *connection = qobject_cast<DefaultConnection*>(sender());
	if (connection && connection->peerCertificate().isValid())
	{
		QString domain = connection->option(IDefaultConnection::COR_DOMAINE).toString();
		QSslCertificate peerCert = connection->peerCertificate();
		QSslCertificate trustCert(Options::fileValue("connection.trusted-ssl-certificate",domain).toByteArray());
		if (peerCert != trustCert)
		{
			IXmppStream *stream = findXmppStream(connection);
			if (stream)
				stream->setKeepAliveTimerActive(false);

			QString errorList = "<ul>";
			foreach(const QSslError &error, AErrors)
				errorList += "<li>"+error.errorString()+"</li>";
			errorList += "</ul>";

			QStringList certInfo = QStringList()
				<< tr("Organization: %1").arg(peerCert.subjectInfo(QSslCertificate::Organization))
				<< tr("Subunit: %1").arg(peerCert.subjectInfo(QSslCertificate::OrganizationalUnitName))
				<< tr("Country: %1").arg(peerCert.subjectInfo(QSslCertificate::CountryName))
				<< tr("Locality: %1").arg(peerCert.subjectInfo(QSslCertificate::LocalityName))
				<< tr("State/Province: %1").arg(peerCert.subjectInfo(QSslCertificate::StateOrProvinceName))
				<< tr("Common Name: %1").arg(peerCert.subjectInfo(QSslCertificate::CommonName))
				<< QString::null
				<< tr("Issuer Organization: %1").arg(peerCert.issuerInfo(QSslCertificate::Organization))
				<< tr("Issuer Unit Name: %1").arg(peerCert.issuerInfo(QSslCertificate::OrganizationalUnitName))
				<< tr("Issuer Country: %1").arg(peerCert.issuerInfo(QSslCertificate::CountryName))
				<< tr("Issuer Locality: %1").arg(peerCert.issuerInfo(QSslCertificate::LocalityName))
				<< tr("Issuer State/Province: %1").arg(peerCert.issuerInfo(QSslCertificate::StateOrProvinceName))
				<< tr("Issuer Common Name: %1").arg(peerCert.issuerInfo(QSslCertificate::CommonName));

			QMessageBox dialog;
			dialog.setIcon(QMessageBox::Warning);
			dialog.setWindowTitle(tr("SSL Authentication Error"));
			dialog.setText(tr("Unable to validate the connection to %1 due to errors:").arg(domain));
			dialog.setInformativeText(errorList);
			dialog.setDetailedText(certInfo.join("\n"));
			dialog.addButton(QMessageBox::Yes)->setText(tr("Connect"));
			dialog.addButton(QMessageBox::No)->setText(tr("Disconnect"));
			dialog.addButton(QMessageBox::Save)->setText(tr("Trust this certificate"));

			switch (dialog.exec())
			{
			case QMessageBox::Save:
				Options::setFileValue(connection->peerCertificate().toPem(),"connection.trusted-ssl-certificate",domain);
			case QMessageBox::Yes:
				connection->ignoreSslErrors();
			default:
				break;
			}

			if (stream)
				stream->setKeepAliveTimerActive(true);
		}
		else
		{
			connection->ignoreSslErrors();
		}
	}
}

void DefaultConnectionPlugin::onConnectionDestroyed()
{
	IConnection *connection = qobject_cast<IConnection *>(sender());
	if (connection)
		emit connectionDestroyed(connection);
}

Q_EXPORT_PLUGIN2(plg_defaultconnection, DefaultConnectionPlugin)
