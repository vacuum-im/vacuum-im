#ifndef SASLFEATUREFACTORY_H
#define SASLFEATUREFACTORY_H

#include <interfaces/ipluginmanager.h>
#include <interfaces/ixmppstreammanager.h>
#include <interfaces/iconnectionmanager.h>
#include "saslauthfeature.h"
#include "saslbindfeature.h"
#include "saslsessionfeature.h"

#define SASLAUTH_UUID "{E583F155-BE87-4919-8769-5C87088F0F57}"

class SASLFeatureFactory :
	public QObject,
	public IPlugin,
	public IXmppFeatureFactory,
	public IXmppStanzaHadler
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IXmppFeatureFactory IXmppStanzaHadler);
	Q_PLUGIN_METADATA(IID "org.vacuum-im.plugins.SASLAuth");
public:
	SASLFeatureFactory();
	~SASLFeatureFactory();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return SASLAUTH_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings() { return true; }
	virtual bool startPlugin() { return true; }
	//IXmppStanzaHadler
	virtual bool xmppStanzaIn(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder);
	virtual bool xmppStanzaOut(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder);
	//IXmppFeatureFactory
	virtual QList<QString> xmppFeatures() const;
	virtual IXmppFeature *newXmppFeature(const QString &AFeatureNS, IXmppStream *AXmppStream);
signals:
	void featureCreated(IXmppFeature *AFeature);
	void featureDestroyed(IXmppFeature *AFeature);
protected slots:
	void onXmppStreamCreated(IXmppStream *AXmppStream);
	void onFeatureDestroyed();
private:
	IXmppStreamManager *FXmppStreamManager;
};

#endif // SASLFEATUREFACTORY_H
