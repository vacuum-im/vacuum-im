#ifndef DEFAULTCONNECTIONENGINE_H
#define DEFAULTCONNECTIONENGINE_H

#include <QObjectCleanupHandler>
#include <interfaces/ipluginmanager.h>
#include <interfaces/idefaultconnection.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/ioptionsmanager.h>
#include "defaultconnection.h"
#include "connectionoptionswidget.h"

class DefaultConnectionEngine :
	public QObject,
	public IPlugin,
	public IDefaultConnectionEngine
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IConnectionEngine IDefaultConnectionEngine);
public:
	DefaultConnectionEngine();
	~DefaultConnectionEngine();
	virtual QObject *instance() { return this; }
	//IPlugin
	virtual QUuid pluginUuid() const { return DEFAULTCONNECTION_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin() { return true; }
	//IConnectionEngine
	virtual QString engineId() const;
	virtual QString engineName() const;
	virtual IConnection *newConnection(const OptionsNode &ANode, QObject *AParent);
	virtual IOptionsDialogWidget *connectionSettingsWidget(const OptionsNode &ANode, QWidget *AParent);
	virtual void saveConnectionSettings(IOptionsDialogWidget *AWidget, OptionsNode ANode = OptionsNode::null);
	virtual void loadConnectionSettings(IConnection *AConnection, const OptionsNode &ANode);
signals:
	void connectionCreated(IConnection *AConnection);
	void connectionDestroyed(IConnection *AConnection);
protected:
	IXmppStream *findConnectionStream(IConnection *AConnection) const;
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

#endif // DEFAULTCONNECTIONENGINE_H
