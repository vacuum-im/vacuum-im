#ifndef IQAUTH_H
#define IQAUTH_H

#include <QDomDocument>
#include <definitions/namespaces.h>
#include <definitions/xmppfeatureorders.h>
#include <definitions/xmppstanzahandlerorders.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/iconnectionmanager.h>
#include <utils/errorhandler.h>
#include <utils/stanza.h>
#include <utils/jid.h>

#define IQAUTH_UUID "{1E3645BC-313F-49e9-BD00-4CC062EE76A7}"

class IqAuth :
			public QObject,
			public IXmppFeature,
			public IXmppStanzaHadler
{
	Q_OBJECT;
	Q_INTERFACES(IXmppFeature IXmppStanzaHadler);
public:
	IqAuth(IXmppStream *AXmppStream);
	~IqAuth();
	virtual QObject *instance() { return this; }
	//IXmppStanzaHadler
	virtual bool xmppStanzaIn(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder);
	virtual bool xmppStanzaOut(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder);
	//IXmppFeature
	virtual QString featureNS() const;
	virtual IXmppStream *xmppStream() const;
	virtual bool start(const QDomElement &AElem);
signals:
	void finished(bool ARestart);
	void error(const QString &AError);
	void featureDestroyed();
private:
	IXmppStream *FXmppStream;
};

class IqAuthPlugin :
			public QObject,
			public IPlugin,
			public IXmppFeaturesPlugin
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IXmppFeaturesPlugin);
public:
	IqAuthPlugin();
	~IqAuthPlugin();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return IQAUTH_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings() { return true; }
	virtual bool startPlugin() { return true; }
	//IXmppFeaturesPlugin
	virtual QList<QString> xmppFeatures() const;
	virtual IXmppFeature *newXmppFeature(const QString &AFeatureNS, IXmppStream *AXmppStream);
signals:
	void featureCreated(IXmppFeature *AStreamFeature);
	void featureDestroyed(IXmppFeature *AStreamFeature);
protected slots:
	void onFeatureDestroyed();
private:
	IXmppStreams *FXmppStreams;
};

#endif // IQAUTH_H
