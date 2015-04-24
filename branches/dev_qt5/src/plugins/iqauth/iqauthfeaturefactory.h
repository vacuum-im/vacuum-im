#ifndef IQAUTHFEATUREFACTORY_H
#define IQAUTHFEATUREFACTORY_H

#include <QDomDocument>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ixmppstreammanager.h>
#include "iqauthfeature.h"

#define IQAUTH_UUID "{1E3645BC-313F-49e9-BD00-4CC062EE76A7}"

class IqAuthFeatureFactory :
	public QObject,
	public IPlugin,
	public IXmppFeatureFactory
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IXmppFeatureFactory);
	Q_PLUGIN_METADATA(IID "org.vacuum-im.plugins.IqAuth");
public:
	IqAuthFeatureFactory();
	~IqAuthFeatureFactory();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return IQAUTH_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings() { return true; }
	virtual bool startPlugin() { return true; }
	//IXmppFeatureFactory
	virtual QList<QString> xmppFeatures() const;
	virtual IXmppFeature *newXmppFeature(const QString &AFeatureNS, IXmppStream *AXmppStream);
signals:
	void featureCreated(IXmppFeature *AStreamFeature);
	void featureDestroyed(IXmppFeature *AStreamFeature);
protected slots:
	void onFeatureDestroyed();
private:
	IXmppStreamManager *FXmppStreamManager;
};

#endif // IQAUTHFEATUREFACTORY_H
