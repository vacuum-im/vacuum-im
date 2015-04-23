#ifndef XMPPSTREAMS_H
#define XMPPSTREAMS_H

#include <QMultiMap>
#include <definitions/namespaces.h>
#include <definitions/optionvalues.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ixmppstreammanager.h>
#include "xmppstream.h"

class XmppStreamManager :
	public QObject,
	public IPlugin,
	public IXmppStreamManager
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IXmppStreamManager);
	Q_PLUGIN_METADATA(IID "org.jrudevels.vacuum.IXmppStreamManager");
public:
	XmppStreamManager();
	~XmppStreamManager();
	virtual QObject *instance() {return this;}
	//IPlugin
	virtual QUuid pluginUuid() const { return XMPPSTREAMS_UUID;}
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin() { return true; }
	// XmppStreams
	virtual QList<IXmppStream *> xmppStreams() const;
	virtual IXmppStream *findXmppStream(const Jid &AStreamJid) const;
	virtual IXmppStream *createXmppStream(const Jid &AStreamJid);
	virtual void destroyXmppStream(IXmppStream *AXmppStream);
	virtual bool isXmppStreamActive(IXmppStream *AXmppStream) const;
	virtual void setXmppStreamActive(IXmppStream *AXmppStream, bool AActive);
	// XmppFeatures
	virtual QList<QString> xmppFeatures() const;
	virtual void registerXmppFeature(int AOrder, const QString &AFeatureNS);
	virtual QList<IXmppFeatureFactory *> xmppFeatureFactories(const QString &AFeatureNS) const;
	virtual void registerXmppFeatureFactory(int AOrder, const QString &AFeatureNS, IXmppFeatureFactory *AFactory);
signals:
	// XmppStreamManager
	void streamCreated(IXmppStream *AXmppStream);
	void streamOpened(IXmppStream *AXmppStream);
	void streamClosed(IXmppStream *AXmppStream);
	void streamAboutToClose(IXmppStream *AXmppStream);
	void streamError(IXmppStream *AXmppStream, const XmppError &AError);
	void streamJidAboutToBeChanged(IXmppStream *AXmppStream, const Jid &AAfter);
	void streamJidChanged(IXmppStream *AXmppStream, const Jid &ABefore);
	void streamConnectionChanged(IXmppStream *AXmppStream, IConnection *AConnection);
	void streamActiveChanged(IXmppStream *AXmppStream, bool AActive);
	void streamDestroyed(IXmppStream *AXmppStream);
	// XmppFeatures
	void xmppFeatureRegistered(int AOrder, const QString &AFeatureNS);
	void xmppFeatureFactoryRegistered(int AOrder, const QString &AFeatureNS, IXmppFeatureFactory *AFactory);
protected slots:
	void onXmppStreamOpened();
	void onXmppStreamAboutToClose();
	void onXmppStreamClosed();
	void onXmppStreamError(const XmppError &AError);
	void onXmppStreamJidAboutToBeChanged(const Jid &AAfter);
	void onXmppStreamJidChanged(const Jid &ABefore);
	void onXmppStreamConnectionChanged(IConnection *AConnection);
	void onXmppStreamDestroyed();
private:
	QList<IXmppStream *> FStreams;
	QList<IXmppStream *> FActiveStreams;
	QMultiMap<int, QString> FFeatureOrders;
	QMap<QString, QMultiMap<int, IXmppFeatureFactory *> > FFeatureFactories;
};

#endif // XMPPSTREAMS_H
