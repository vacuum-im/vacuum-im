#ifndef COMPRESSFEATUREFACTORY_H
#define COMPRESSFEATUREFACTORY_H

#include <interfaces/ipluginmanager.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/iaccountmanager.h>
#include <interfaces/ixmppstreammanager.h>
#include "compressfeature.h"

#define COMPRESS_UUID "{061D0687-B954-416d-B690-D1BA7D845D83}"

class CompressFeatureFactory :
	public QObject,
	public IPlugin,
	public IXmppFeatureFactory,
	public IOptionsDialogHolder
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IXmppFeatureFactory IOptionsDialogHolder);
	Q_PLUGIN_METADATA(IID "org.vacuum-im.plugins.Compress");
public:
	CompressFeatureFactory();
	~CompressFeatureFactory();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return COMPRESS_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin() { return true; }
	//IOptionsHolder
	virtual QMultiMap<int, IOptionsDialogWidget *> optionsDialogWidgets(const QString &ANodeId, QWidget *AParent);
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
	IOptionsManager *FOptionsManager;
	IAccountManager *FAccountManager;
};

#endif // COMPRESSFEATUREFACTORY_H
