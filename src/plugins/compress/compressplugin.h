#ifndef COMPRESSPLUGIN_H
#define COMPRESSPLUGIN_H

#include <QObject>
#include <QObjectCleanupHandler>
#include <definitions/namespaces.h>
#include <definitions/xmppfeatureorders.h>
#include <definitions/xmppfeaturepluginorders.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ixmppstreams.h>
#include <utils/errorhandler.h>
#include "compression.h"

#define COMPRESS_UUID "{061D0687-B954-416d-B690-D1BA7D845D83}"

class CompressPlugin :
			public QObject,
			public IPlugin,
			public IXmppFeaturesPlugin
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IXmppFeaturesPlugin);
public:
	CompressPlugin();
	~CompressPlugin();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return COMPRESS_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings() { return true; }
	virtual bool startPlugin() { return true; }
	//IXmppFeaturesPlugin
	virtual QList<QString> xmppFeatures() const { return QList<QString>() << NS_FEATURE_COMPRESS; }
	virtual IXmppFeature *newXmppFeature(const QString &AFeatureNS, IXmppStream *AXmppStream);
signals:
	void featureCreated(IXmppFeature *AFeature);
	void featureDestroyed(IXmppFeature *AFeature);
protected slots:
	void onFeatureDestroyed();
private:
	IXmppStreams *FXmppStreams;
};

#endif // COMPRESSPLUGIN_H
