#ifndef COMPRESSPLUGIN_H
#define COMPRESSPLUGIN_H

#include <QObject>
#include <definitions/namespaces.h>
#include <definitions/optionnodes.h>
#include <definitions/optionvalues.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/xmppfeatureorders.h>
#include <definitions/xmppfeaturepluginorders.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/iaccountmanager.h>
#include <utils/options.h>
#include <utils/xmpperror.h>
#include "compression.h"

#define COMPRESS_UUID "{061D0687-B954-416d-B690-D1BA7D845D83}"

class CompressPlugin :
			public QObject,
			public IPlugin,
			public IOptionsHolder,
			public IXmppFeaturesPlugin
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IOptionsHolder IXmppFeaturesPlugin);
public:
	CompressPlugin();
	~CompressPlugin();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return COMPRESS_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin() { return true; }
	//IOptionsHolder
	virtual QMultiMap<int, IOptionsWidget *> optionsWidgets(const QString &ANodeId, QWidget *AParent);
	//IXmppFeaturesPlugin
	virtual QList<QString> xmppFeatures() const;
	virtual IXmppFeature *newXmppFeature(const QString &AFeatureNS, IXmppStream *AXmppStream);
signals:
	void featureCreated(IXmppFeature *AFeature);
	void featureDestroyed(IXmppFeature *AFeature);
protected slots:
	void onFeatureDestroyed();
private:
	IXmppStreams *FXmppStreams;
	IOptionsManager *FOptionsManager;
	IAccountManager *FAccountManager;
};

#endif // COMPRESSPLUGIN_H
