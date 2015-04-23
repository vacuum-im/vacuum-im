#ifndef STARTTLSFEATUREFACTORY_H
#define STARTTLSFEATUREFACTORY_H

#include <interfaces/ipluginmanager.h>
#include <interfaces/ixmppstreammanager.h>
#include "starttlsfeature.h"

#define STARTTLS_UUID "{F554544C-0851-4e2a-9158-99191911E468}"

class StartTLSFeatureFactory :
	public QObject,
	public IPlugin,
	public IXmppFeatureFactory
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IXmppFeatureFactory);
	Q_PLUGIN_METADATA(IID "org.jrudevels.vacuum.IStartTLSFeatureFactory");
public:
	StartTLSFeatureFactory();
	~StartTLSFeatureFactory();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return STARTTLS_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings() { return true; }
	virtual bool startPlugin() { return true; }
	//IXmppFeatureFactory
	virtual QList<QString> xmppFeatures() const;
	virtual IXmppFeature *newXmppFeature(const QString &AFeatureNS, IXmppStream *AXmppStream);
signals:
	void featureCreated(IXmppFeature *AFeature);
	void featureDestroyed(IXmppFeature *AFeature);
protected slots:
	void onFeatureDestroyed();
private:
	IXmppStreamManager *FXmppStreamManager;
};

#endif // STARTTLSFEATUREFACTORY_H
