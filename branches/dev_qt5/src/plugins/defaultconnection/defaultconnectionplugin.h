#ifndef DEFAULTCONNECTIONPLUGIN_H
#define DEFAULTCONNECTIONPLUGIN_H

#include <QObjectCleanupHandler>
#include <definitions/optionvalues.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/iconnectionmanager.h>
#include <interfaces/idefaultconnection.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/ioptionsmanager.h>
#include <utils/options.h>
#include "defaultconnection.h"
#include "connectionoptionswidget.h"

class DefaultConnectionPlugin :
			public QObject,
			public IPlugin,
			public IConnectionPlugin,
			public IDefaultConnectionPlugin
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IConnectionPlugin IDefaultConnectionPlugin);
	Q_PLUGIN_METADATA(IID "org.jrudevels.vacuum.IDefaultConnectionPlugin");
public:
	DefaultConnectionPlugin();
	~DefaultConnectionPlugin();
	virtual QObject *instance() { return this; }
	//IPlugin
	virtual QUuid pluginUuid() const { return DEFAULTCONNECTION_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects() { return true; }
	virtual bool initSettings();
	virtual bool startPlugin() { return true; }
	//IConnectionPlugin
	virtual QString pluginId() const;
	virtual QString pluginName() const;
	virtual IConnection *newConnection(const OptionsNode &ANode, QObject *AParent);
	virtual IOptionsWidget *connectionSettingsWidget(const OptionsNode &ANode, QWidget *AParent);
	virtual void saveConnectionSettings(IOptionsWidget *AWidget, OptionsNode ANode = OptionsNode::null);
	virtual void loadConnectionSettings(IConnection *AConnection, const OptionsNode &ANode);
signals:
	void connectionCreated(IConnection *AConnection);
	void connectionDestroyed(IConnection *AConnection);
protected:
	IXmppStream *findXmppStream(IConnection *AConnection) const;
protected slots:
	void onConnectionAboutToConnect();
	void onConnectionSSLErrorsOccured(const QList<QSslError> &AErrors);
	void onConnectionDestroyed();
private:
	IXmppStreams *FXmppStreams;
	IOptionsManager *FOptionsManager;
	IConnectionManager *FConnectionManager;
private:
	QObjectCleanupHandler FCleanupHandler;
};

#endif // DEFAULTCONNECTIONPLUGIN_H
