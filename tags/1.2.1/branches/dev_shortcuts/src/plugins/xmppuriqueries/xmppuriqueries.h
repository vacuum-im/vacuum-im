#ifndef XMPPURIQUERIES_H
#define XMPPURIQUERIES_H

#include <interfaces/ipluginmanager.h>
#include <interfaces/ixmppuriqueries.h>
#include <interfaces/imessagewidgets.h>
#include <definitions/viewurlhandlerorders.h>

class XmppUriQueries :
			public QObject,
			public IPlugin,
			public IXmppUriQueries,
			public IViewUrlHandler
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IXmppUriQueries IViewUrlHandler);
public:
	XmppUriQueries();
	~XmppUriQueries();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return XMPPURIQUERIES_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings() { return true; }
	virtual bool startPlugin() { return true; }
	//IViewUrlHandler
	virtual bool viewUrlOpen(IViewWidget *AWidget, const QUrl &AUrl, int AOrder);
	//IXmppUriQueries
	virtual bool openXmppUri(const Jid &AStreamJid, const QUrl &AUrl) const;
	virtual void insertUriHandler(IXmppUriHandler *AHandler, int AOrder);
	virtual void removeUriHandler(IXmppUriHandler *AHandler, int AOrder);
signals:
	void uriHandlerInserted(IXmppUriHandler *AHandler, int AOrder);
	void uriHandlerRemoved(IXmppUriHandler *AHandler, int AOrder);
private:
	IMessageWidgets *FMessageWidgets;
private:
	QMultiMap<int, IXmppUriHandler *> FHandlers;
};

#endif // XMPPURIQUERIES_H
