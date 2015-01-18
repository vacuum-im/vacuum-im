#ifndef CONNECTIONMANAGER_H
#define CONNECTIONMANAGER_H

#include <QComboBox>
#include <interfaces/ipluginmanager.h>
#include <interfaces/iconnectionmanager.h>
#include <interfaces/idefaultconnection.h>
#include <interfaces/iaccountmanager.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/irostersview.h>
#include <interfaces/ioptionsmanager.h>
#include "editproxydialog.h"
#include "proxysettingswidget.h"
#include "connectionoptionswidget.h"

class ConnectionManager :
	public QObject,
	public IPlugin,
	public IConnectionManager,
	public IOptionsDialogHolder
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IConnectionManager IOptionsDialogHolder);
public:
	ConnectionManager();
	~ConnectionManager();
	virtual QObject *instance() { return this; }
	//IPlugin
	virtual QUuid pluginUuid() const { return CONNECTIONMANAGER_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin() { return true; }
	//IOptionsHolder
	virtual QMultiMap<int, IOptionsDialogWidget *> optionsDialogWidgets(const QString &ANodeId, QWidget *AParent);
	//IConnectionManager
	virtual QList<QString> pluginList() const;
	virtual IConnectionPlugin *pluginById(const QString &APluginId) const;
	virtual QList<QUuid> proxyList() const;
	virtual IConnectionProxy proxyById(const QUuid &AProxyId) const;
	virtual void setProxy(const QUuid &AProxyId, const IConnectionProxy &AProxy);
	virtual void removeProxy(const QUuid &AProxyId);
	virtual QUuid defaultProxy() const;
	virtual void setDefaultProxy(const QUuid &AProxyId);
	virtual QDialog *showEditProxyDialog(QWidget *AParent = NULL);
	virtual IOptionsDialogWidget *proxySettingsWidget(const OptionsNode &ANode, QWidget *AParent);
	virtual void saveProxySettings(IOptionsDialogWidget *AWidget, OptionsNode ANode = OptionsNode::null);
	virtual QUuid loadProxySettings(const OptionsNode &ANode) const;
	virtual QList<QSslCertificate> trustedCaCertificates(bool AWithUsers=true) const;
	virtual void addTrustedCaCertificate(const QSslCertificate &ACertificate);
signals:
	void connectionCreated(IConnection *AConnection);
	void connectionDestroyed(IConnection *AConnection);
	void proxyChanged(const QUuid &AProxyId, const IConnectionProxy &AProxy);
	void proxyRemoved(const QUuid &AProxyId);
	void defaultProxyChanged(const QUuid &AProxyId);
protected:
	void updateAccountConnection(IAccount *AAccount) const;
	void updateConnectionSettings(IAccount *AAccount = NULL) const;
	IXmppStream *findConnectionStream(IConnection *AConnection) const;
protected slots:
	void onConnectionEncrypted();
	void onConnectionDisconnected();
	void onConnectionCreated(IConnection *AConnection);
	void onConnectionDestroyed(IConnection *AConnection);
protected slots:
	void onOptionsOpened();
	void onOptionsChanged(const OptionsNode &ANode);
	void onAccountShown(IAccount *AAccount);
	void onAccountOptionsChanged(IAccount *AAccount, const OptionsNode &ANode);
	void onRosterIndexToolTips(IRosterIndex *AIndex, quint32 ALabelId, QMap<int,QString> &AToolTips);
private:
	IPluginManager *FPluginManager;
	IXmppStreams *FXmppStreams;
	IAccountManager *FAccountManager;
	IOptionsManager *FOptionsManager;
	IRostersViewPlugin *FRostersViewPlugin;
private:
	quint32 FEncryptedLabelId;
	QMap<QString, IConnectionPlugin *> FPlugins;
};

#endif // CONNECTIONMANAGER_H
