#ifndef ICONNECTIONMANAGER_H
#define ICONNECTIONMANAGER_H

#define CONNECTIONMANAGER_UUID      "{B54F3B5E-3595-48c2-AB6F-249D4AD18327}"

#define APPLICATION_PROXY_REF_UUID  "{b919d5c9-6def-43ba-87aa-892d49b9ac67}"

#include <QUuid>
#include <QDialog>
#include <QNetworkProxy>
#include <QSslCertificate>
#include <interfaces/ioptionsmanager.h>
#include <utils/options.h>
#include <utils/xmpperror.h>

class IConnectionPlugin;

struct IConnectionProxy {
	QString name;
	QNetworkProxy proxy;
};

class IConnection
{
public:
	virtual QObject *instance() =0;
	virtual bool isOpen() const =0;
	virtual bool isEncrypted() const =0;
	virtual bool isEncryptionSupported() const =0;
	virtual bool connectToHost() =0;
	virtual bool startEncryption() =0;
	virtual void disconnectFromHost() =0;
	virtual void abortConnection(const XmppError &AError) =0;
	virtual qint64 write(const QByteArray &AData) =0;
	virtual QByteArray read(qint64 ABytes) =0;
	virtual IConnectionPlugin *ownerPlugin() const =0;
	virtual QSslCertificate hostCertificate() const =0;
protected:
	virtual void aboutToConnect() =0;
	virtual void connected() =0;
	virtual void encrypted() =0;
	virtual void readyRead(qint64 ABytes) =0;
	virtual void error(const XmppError &AError) =0;
	virtual void aboutToDisconnect() =0;
	virtual void disconnected() =0;
	virtual void connectionDestroyed() =0;
};

class IConnectionPlugin
{
public:
	virtual QObject *instance() =0;
	virtual QString pluginId() const =0;
	virtual QString pluginName() const =0;
	virtual IConnection *newConnection(const OptionsNode &ANode, QObject *AParent) =0;
	virtual IOptionsDialogWidget *connectionSettingsWidget(const OptionsNode &ANode, QWidget *AParent) =0;
	virtual void saveConnectionSettings(IOptionsDialogWidget *AWidget, OptionsNode ANode = OptionsNode::null) =0;
	virtual void loadConnectionSettings(IConnection *AConnection, const OptionsNode &ANode) =0;
protected:
	virtual void connectionCreated(IConnection *AConnection) =0;
	virtual void connectionDestroyed(IConnection *AConnection) =0;
};

class IConnectionManager
{
public:
	virtual QObject *instance() =0;
	virtual QList<QString> pluginList() const =0;
	virtual IConnectionPlugin *pluginById(const QString &APluginId) const =0;
	virtual QList<QUuid> proxyList() const =0;
	virtual IConnectionProxy proxyById(const QUuid &AProxyId) const =0;
	virtual void setProxy(const QUuid &AProxyId, const IConnectionProxy &AProxy) =0;
	virtual void removeProxy(const QUuid &AProxyId) =0;
	virtual QUuid defaultProxy() const =0;
	virtual void setDefaultProxy(const QUuid &AProxyId) =0;
	virtual QDialog *showEditProxyDialog(QWidget *AParent = NULL) =0;
	virtual IOptionsDialogWidget *proxySettingsWidget(const OptionsNode &ANode, QWidget *AParent) =0;
	virtual void saveProxySettings(IOptionsDialogWidget *AWidget, OptionsNode ANode = OptionsNode::null) =0;
	virtual QUuid loadProxySettings(const OptionsNode &ANode) const =0;
	virtual QList<QSslCertificate> trustedCaCertificates(bool AWithUsers=true) const =0;
	virtual void addTrustedCaCertificate(const QSslCertificate &ACertificate) =0;
protected:
	virtual void connectionCreated(IConnection *AConnection) =0;
	virtual void connectionDestroyed(IConnection *AConnection) =0;
	virtual void proxyChanged(const QUuid &AProxyId, const IConnectionProxy &AProxy) =0;
	virtual void proxyRemoved(const QUuid &AProxyId) =0;
	virtual void defaultProxyChanged(const QUuid &AProxyId) =0;
};

Q_DECLARE_INTERFACE(IConnection,"Vacuum.Plugin.IConnection/1.3")
Q_DECLARE_INTERFACE(IConnectionPlugin,"Vacuum.Plugin.IConnectionPlugin/1.3")
Q_DECLARE_INTERFACE(IConnectionManager,"Vacuum.Plugin.IConnectionManager/1.3")

#endif // ICONNECTIONMANAGER_H
