#ifndef INBANDSTREAMS_H
#define INBANDSTREAMS_H

#include <interfaces/ipluginmanager.h>
#include <interfaces/iinbandstreams.h>
#include <interfaces/idatastreamsmanager.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/iservicediscovery.h>
#include "inbandstream.h"
#include "inbandoptions.h"

class InBandStreams :
	public QObject,
	public IPlugin,
	public IInBandStreams
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IInBandStreams IDataStreamMethod);
public:
	InBandStreams();
	~InBandStreams();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return INBANDSTREAMS_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin() { return true; }
	//IDataStreamMethod
	virtual QString methodNS() const;
	virtual QString methodName() const;
	virtual QString methodDescription() const;
	virtual IDataStreamSocket *dataStreamSocket(const QString &AStreamId, const Jid &AStreamJid, const Jid &AContactJid, IDataStreamSocket::StreamKind AKind, QObject *AParent=NULL);
	virtual IOptionsWidget *methodSettingsWidget(const OptionsNode &ANode, bool AReadOnly, QWidget *AParent);
	virtual IOptionsWidget *methodSettingsWidget(IDataStreamSocket *ASocket, bool AReadOnly, QWidget *AParent);
	virtual void saveMethodSettings(IOptionsWidget *AWidget, OptionsNode ANode = OptionsNode::null);
	virtual void loadMethodSettings(IDataStreamSocket *ASocket, IOptionsWidget *AWidget);
	virtual void loadMethodSettings(IDataStreamSocket *ASocket, const OptionsNode &ANode);
signals:
	void socketCreated(IDataStreamSocket *ASocket);
private:
	IDataStreamsManager *FDataManager;
	IStanzaProcessor *FStanzaProcessor;
	IServiceDiscovery *FDiscovery;
};

#endif // INBANDSTREAMS_H
