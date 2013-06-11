#ifndef SASLPLUGIN_H
#define SASLPLUGIN_H

#include <definitions/namespaces.h>
#include <definitions/xmpperrors.h>
#include <definitions/internalerrors.h>
#include <definitions/xmppfeatureorders.h>
#include <definitions/xmppfeaturepluginorders.h>
#include <definitions/xmppstanzahandlerorders.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/iconnectionmanager.h>
#include <utils/xmpperror.h>
#include "saslauth.h"
#include "saslbind.h"
#include "saslsession.h"

#define SASLAUTH_UUID "{E583F155-BE87-4919-8769-5C87088F0F57}"

class SASLPlugin :
	public QObject,
	public IPlugin,
	public IXmppFeaturesPlugin,
	public IXmppStanzaHadler
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IXmppFeaturesPlugin IXmppStanzaHadler);
public:
	SASLPlugin();
	~SASLPlugin();
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
	//IXmppFeaturesPlugin
	virtual QList<QString> xmppFeatures() const;
	virtual IXmppFeature *newXmppFeature(const QString &AFeatureNS, IXmppStream *AXmppStream);
signals:
	void featureCreated(IXmppFeature *AFeature);
	void featureDestroyed(IXmppFeature *AFeature);
protected slots:
	void onXmppStreamCreated(IXmppStream *AXmppStream);
	void onFeatureDestroyed();
private:
	IXmppStreams *FXmppStreams;
};

#endif // SASLPLUGIN_H
