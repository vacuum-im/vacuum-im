#ifndef XMPPSTREAMS_H
#define XMPPSTREAMS_H

#include <QMultiMap>
#include <definitions/namespaces.h>
#include <definitions/optionvalues.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ixmppstreams.h>
#include "xmppstream.h"

class XmppStreams :
	public QObject,
	public IPlugin,
	public IXmppStreams
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IXmppStreams);
	Q_PLUGIN_METADATA(IID "org.jrudevels.vacuum.IXmppStreams");
public:
	XmppStreams();
	~XmppStreams();
	virtual QObject *instance() {return this;}
	//IPlugin
	virtual QUuid pluginUuid() const { return XMPPSTREAMS_UUID;}
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin() { return true; }
	//IXmppStreams
	virtual QList<IXmppStream *> xmppStreams() const;
	virtual IXmppStream *xmppStream(const Jid &AStreamJid) const;
	virtual IXmppStream *newXmppStream(const Jid &AStreamJid);
	virtual bool isActive(IXmppStream *AXmppStream) const;
	virtual void addXmppStream(IXmppStream *AXmppStream);
	virtual void removeXmppStream(IXmppStream *AXmppStream);
	virtual void destroyXmppStream(const Jid &AJid);
	virtual QList<QString> xmppFeatures() const;
	virtual void registerXmppFeature(int AOrder, const QString &AFeatureNS);
	virtual QList<IXmppFeaturesPlugin *> xmppFeaturePlugins(const QString &AFeatureNS) const;
	virtual void registerXmppFeaturePlugin(int AOrder, const QString &AFeatureNS, IXmppFeaturesPlugin *AFeaturePlugin);
signals:
	void created(IXmppStream *AXmppStream);
	void added(IXmppStream *AXmppStream);
	void opened(IXmppStream *AXmppStream);
	void aboutToClose(IXmppStream *AXmppStream);
	void closed(IXmppStream *AXmppStream);
	void error(IXmppStream *AXmppStream, const XmppError &AError);
	void jidAboutToBeChanged(IXmppStream *AXmppStream, const Jid &AAfter);
	void jidChanged(IXmppStream *AXmppStream, const Jid &ABefore);
	void connectionChanged(IXmppStream *AXmppStream, IConnection *AConnection);
	void removed(IXmppStream *AXmppStream);
	void streamDestroyed(IXmppStream *AXmppStream);
	void xmppFeatureRegistered(int AOrder, const QString &AFeatureNS);
	void xmppFeaturePluginRegistered(int AOrder, const QString &AFeatureNS, IXmppFeaturesPlugin *AFeaturePlugin);
protected slots:
	void onStreamOpened();
	void onStreamAboutToClose();
	void onStreamClosed();
	void onStreamError(const XmppError &AError);
	void onStreamJidAboutToBeChanged(const Jid &AAfter);
	void onStreamJidChanged(const Jid &ABefore);
	void onStreamConnectionChanged(IConnection *AConnection);
	void onStreamDestroyed();
private:
	QList<IXmppStream *> FStreams;
	QList<IXmppStream *> FActiveStreams;
	QMultiMap<int, QString> FFeatureOrders;
	QMap<QString, QMultiMap<int, IXmppFeaturesPlugin *> > FFeaturePlugins;
};

#endif // XMPPSTREAMS_H
