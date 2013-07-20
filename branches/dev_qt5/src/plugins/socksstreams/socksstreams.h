#ifndef SOCKSSTREAMS_H
#define SOCKSSTREAMS_H

#include <QTcpServer>
#include <definitions/namespaces.h>
#include <definitions/optionvalues.h>
#include <definitions/internalerrors.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/isocksstreams.h>
#include <interfaces/idatastreamsmanager.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/iconnectionmanager.h>
#include <interfaces/idefaultconnection.h>
#include <interfaces/iservicediscovery.h>
#include <interfaces/iconnectionmanager.h>
#include <utils/options.h>
#include "socksstream.h"
#include "socksoptions.h"

class SocksStreams :
	public QObject,
	public IPlugin,
	public ISocksStreams
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin ISocksStreams IDataStreamMethod);
	Q_PLUGIN_METADATA(IID "org.jrudevels.vacuum.ISocksStreams");
public:
	SocksStreams();
	~SocksStreams();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return SOCKSSTREAMS_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin() { return true; }
	//IDataStreamMethod
	virtual QString methodNS() const;
	virtual QString methodName() const;
	virtual QString methodDescription() const;
	virtual IDataStreamSocket *dataStreamSocket(const QString &ASocketId, const Jid &AStreamJid, const Jid &AContactJid, IDataStreamSocket::StreamKind AKind, QObject *AParent=NULL);
	virtual IOptionsWidget *methodSettingsWidget(const OptionsNode &ANode, bool AReadOnly, QWidget *AParent);
	virtual IOptionsWidget *methodSettingsWidget(IDataStreamSocket *ASocket, bool AReadOnly, QWidget *AParent);
	virtual void saveMethodSettings(IOptionsWidget *AWidget, OptionsNode ANode = OptionsNode::null);
	virtual void loadMethodSettings(IDataStreamSocket *ASocket, IOptionsWidget *AWidget);
	virtual void loadMethodSettings(IDataStreamSocket *ASocket, const OptionsNode &ANode);
	//ISocksStreams
	virtual quint16 listeningPort() const;
	virtual QString accountStreamProxy(const Jid &AStreamJid) const;
	virtual QNetworkProxy accountNetworkProxy(const Jid &AStreamJid) const;
	virtual QString connectionKey(const QString &ASessionId, const Jid &AInitiator, const Jid &ATarget) const;
	virtual bool appendLocalConnection(const QString &AKey);
	virtual void removeLocalConnection(const QString &AKey);
signals:
	//IDataStreamMethod
	void socketCreated(IDataStreamSocket *ASocket);
	//ISocksStreams
	void localConnectionAccepted(const QString &AKey, QTcpSocket *ATcpSocket);
protected slots:
	void onXmppStreamOpened(IXmppStream *AStream);
	void onXmppStreamClosed(IXmppStream *AStream);
	void onDiscoItemsReceived(const IDiscoItems &AItems);
	void onNewServerConnection();
	void onServerConnectionReadyRead();
	void onServerConnectionDisconnected();
private:
	IXmppStreams *FXmppStreams;
	IDataStreamsManager *FDataManager;
	IStanzaProcessor *FStanzaProcessor;
	IServiceDiscovery *FDiscovery;
	IConnectionManager *FConnectionManager;
private:
	QTcpServer FServer;
	QList<QString> FLocalKeys;
	QMap<Jid, QString> FStreamProxy;
};

#endif // SOCKSSTREAMS_H
