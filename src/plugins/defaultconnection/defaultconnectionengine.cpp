#include "defaultconnectionengine.h"

#include <QPushButton>
#include <QMessageBox>
#include <QTextDocument>
#include <definitions/optionvalues.h>
#include <definitions/internalerrors.h>
#include <utils/options.h>
#include <utils/logger.h>

DefaultConnectionEngine::DefaultConnectionEngine()
{
	FXmppStreamManager = NULL;
	FOptionsManager = NULL;
	FConnectionManager = NULL;
}

DefaultConnectionEngine::~DefaultConnectionEngine()
{
	FCleanupHandler.clear();
}

void DefaultConnectionEngine::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Default Connection");
	APluginInfo->description = tr("Allows to set a standard TCP connection to Jabber server");
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->version = "1.0";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(CONNECTIONMANAGER_UUID);
}

bool DefaultConnectionEngine::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);

	IPlugin *plugin = APluginManager->pluginInterface("IConnectionManager").value(0,NULL);
	if (plugin)
	{
		FConnectionManager = qobject_cast<IConnectionManager *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IXmppStreamManager").value(0,NULL);
	if (plugin)
	{
		FXmppStreamManager = qobject_cast<IXmppStreamManager *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IOptionsManager").value(0,NULL);
	if (plugin)
	{
		FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());
	}

	return FConnectionManager!=NULL;
}

bool DefaultConnectionEngine::initObjects()
{
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_DEFAULTCONNECTION_CERT_NOT_TRUSTED,tr("Host certificate is not in trusted list"));

	if (FConnectionManager)
	{
		FConnectionManager->registerConnectionEngine(this);
	}

	return true;
}

bool DefaultConnectionEngine::initSettings()
{
	Options::setDefaultValue(OPV_ACCOUNT_CONNECTION_HOST,QString());
	Options::setDefaultValue(OPV_ACCOUNT_CONNECTION_PORT,5222);
	Options::setDefaultValue(OPV_ACCOUNT_CONNECTION_PROXY,QString(APPLICATION_PROXY_REF_UUID));
	Options::setDefaultValue(OPV_ACCOUNT_CONNECTION_SSLPROTOCOL,QSsl::SecureProtocols);
	Options::setDefaultValue(OPV_ACCOUNT_CONNECTION_USELEGACYSSL,false);
	Options::setDefaultValue(OPV_ACCOUNT_CONNECTION_CERTVERIFYMODE,IDefaultConnection::Manual);
	return true;
}

QString DefaultConnectionEngine::engineId() const
{
	static const QString id = "DefaultConnection";
	return id;
}

QString DefaultConnectionEngine::engineName() const
{
	return tr("Default Connection");
}

IConnection *DefaultConnectionEngine::newConnection(const OptionsNode &ANode, QObject *AParent)
{
	LOG_DEBUG("Default connection created");
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

IOptionsDialogWidget *DefaultConnectionEngine::connectionSettingsWidget(const OptionsNode &ANode, QWidget *AParent)
{
	return FConnectionManager!=NULL ? new ConnectionOptionsWidget(FConnectionManager, ANode, AParent) : NULL;
}

void DefaultConnectionEngine::saveConnectionSettings(IOptionsDialogWidget *AWidget, OptionsNode ANode)
{
	ConnectionOptionsWidget *widget = qobject_cast<ConnectionOptionsWidget *>(AWidget->instance());
	if (widget)
		widget->apply(ANode);
}

void DefaultConnectionEngine::loadConnectionSettings(IConnection *AConnection, const OptionsNode &ANode)
{
	IDefaultConnection *connection = qobject_cast<IDefaultConnection *>(AConnection->instance());
	if (connection)
	{
		if (FConnectionManager)
			connection->setProxy(FConnectionManager->proxyById(FConnectionManager->loadProxySettings(ANode.node("proxy"))).proxy);
		connection->setOption(IDefaultConnection::Host,ANode.value("host").toString());
		connection->setOption(IDefaultConnection::Port,ANode.value("port").toInt());
		connection->setOption(IDefaultConnection::UseLegacySsl,ANode.value("use-legacy-ssl").toBool());
		connection->setOption(IDefaultConnection::CertVerifyMode,ANode.value("cert-verify-mode").toInt());
		connection->setProtocol((QSsl::SslProtocol)ANode.value("ssl-protocol").toInt());
	}
}

IXmppStream *DefaultConnectionEngine::findConnectionStream(IConnection *AConnection) const
{
	if (FXmppStreamManager && AConnection)
	{
		foreach(IXmppStream *stream, FXmppStreamManager->xmppStreams())
			if (stream->connection() == AConnection)
				return stream;
	}
	return NULL;
}

void DefaultConnectionEngine::onConnectionAboutToConnect()
{
	IDefaultConnection *connection = qobject_cast<IDefaultConnection*>(sender());
	IXmppStream *stream = findConnectionStream(connection);
	if (connection && stream)
	{
		if (FConnectionManager)
		{
			bool withUsers = connection->option(IDefaultConnection::CertVerifyMode).toInt()!=IDefaultConnection::TrustedOnly;
			connection->addCaSertificates(FConnectionManager->trustedCaCertificates(withUsers));
		}
		connection->setOption(IDefaultConnection::Domain,stream->streamJid().pDomain());
	}
}

void DefaultConnectionEngine::onConnectionSSLErrorsOccured(const QList<QSslError> &AErrors)
{
	IDefaultConnection *connection = qobject_cast<IDefaultConnection*>(sender());
	QSslCertificate peerCert = connection!=NULL ? connection->hostCertificate() : QSslCertificate();
	if (!peerCert.isNull())
	{
		if (!connection->caCertificates().contains(peerCert))
		{
			QString domain = connection->option(IDefaultConnection::Domain).toString();
			QSslCertificate trustCert(Options::fileValue("connection.trusted-ssl-certificate",domain).toByteArray());
			if (peerCert != trustCert)
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

				IXmppStream *stream = findConnectionStream(connection);
				if (stream)
					stream->setKeepAliveTimerActive(false);

				int verifyMode = connection->option(IDefaultConnection::CertVerifyMode).toInt();

				QString errorList = "<ul>";
				foreach(const QSslError &error, AErrors)
					errorList += "<li>"+error.errorString()+"</li>";
				errorList += "</ul>";

				QStringList certInfo;
				certInfo += tr("Certificate holder:");
				for (uint i=0; i<certInfoNamesCount; i++)
				{
					QString value = peerCert.subjectInfo(certInfoNames[i].info);
					if (!value.isEmpty())
						certInfo += "   " + certInfoNames[i].name.arg(Qt::escape(value));
				}
				certInfo += "\n" + tr("Certificate issuer:");
				for (uint i=0; i<certInfoNamesCount; i++)
				{
					QString value = peerCert.issuerInfo(certInfoNames[i].info);
					if (!value.isEmpty())
						certInfo += "   " + certInfoNames[i].name.arg(Qt::escape(value));
				}
				certInfo += "\n" + tr("Certificate details:");
				certInfo += "   " + tr("Effective from: %1").arg(peerCert.effectiveDate().date().toString());
				certInfo += "   " + tr("Expired at: %1").arg(peerCert.expiryDate().date().toString());
				certInfo += "   " + tr("Serial number: %1").arg(QString::fromLocal8Bit(peerCert.serialNumber().toUpper()));

				QMessageBox dialog;
				dialog.setIcon(QMessageBox::Warning);
				dialog.setWindowTitle(tr("SSL Authentication Error"));
				dialog.setText(tr("Connection to <b>%1</b> can not be considered completely safe due to errors in servers certificate check:").arg(domain));
				dialog.setInformativeText(errorList);
				dialog.setDetailedText(certInfo.join("\n"));
				dialog.addButton(QMessageBox::No)->setText(tr("Disconnect"));
				connect(connection->instance(),SIGNAL(disconnected()),&dialog,SLOT(reject()));

				QPushButton *connectOnce = dialog.addButton(QMessageBox::Yes);
				connectOnce->setText(tr("Connect Once"));
				connectOnce->setEnabled(verifyMode==IDefaultConnection::Manual);

				QPushButton *connectAlways = dialog.addButton(QMessageBox::Save);
				connectAlways->setText(tr("Connect Always"));
				connectAlways->setEnabled(verifyMode==IDefaultConnection::Manual);

				switch (dialog.exec())
				{
				case QMessageBox::Save:
					if (FConnectionManager)
						FConnectionManager->addTrustedCaCertificate(peerCert);
				case QMessageBox::Yes:
					connection->ignoreSslErrors();
					break;
				default:
					break;
				}

				if (stream)
					stream->setKeepAliveTimerActive(true);
			}
			else
			{
				if (FConnectionManager)
					FConnectionManager->addTrustedCaCertificate(peerCert);
				connection->ignoreSslErrors();
			}
		}
		else
		{
			connection->ignoreSslErrors();
		}
	}
}

void DefaultConnectionEngine::onConnectionDestroyed()
{
	IDefaultConnection *connection = qobject_cast<IDefaultConnection *>(sender());
	if (connection)
	{
		LOG_DEBUG("Default connection destroyed");
		emit connectionDestroyed(connection);
	}
}

Q_EXPORT_PLUGIN2(plg_defaultconnection, DefaultConnectionEngine)
