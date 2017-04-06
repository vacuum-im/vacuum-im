#ifndef XMPPURIQUERIES_H
#define XMPPURIQUERIES_H

#include <interfaces/ipluginmanager.h>
#include <interfaces/ixmppuriqueries.h>
#include <interfaces/imessagewidgets.h>

class XmppUriQueries :
	public QObject,
	public IPlugin,
	public IXmppUriQueries,
	public IMessageViewUrlHandler
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IXmppUriQueries IMessageViewUrlHandler);
	Q_PLUGIN_METADATA(IID "org.vacuum-im.plugins.XmppUriQueries");
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
	//IMessageViewUrlHandler
	virtual bool messageViewUrlOpen(int AOrder, IMessageViewWidget *AWidget, const QUrl &AUrl);
	//IXmppUriQueries
	virtual bool openXmppUri(const Jid &AStreamJid, const QUrl &AUrl) const;
	virtual bool parseXmppUri(const QUrl &AUrl, Jid &AContactJid, QString &AAction, QMultiMap<QString, QString> &AParams) const;
	virtual QString makeXmppUri(const Jid &AContactJid, const QString &AAction, const QMultiMap<QString, QString> &AParams) const;
	virtual void insertUriHandler(int AOrder, IXmppUriHandler *AHandler);
	virtual void removeUriHandler(int AOrder, IXmppUriHandler *AHandler);
signals:
	void uriHandlerInserted(int AOrder, IXmppUriHandler *AHandler);
	void uriHandlerRemoved(int AOrder, IXmppUriHandler *AHandler);
private:
	IMessageWidgets *FMessageWidgets;
private:
	QMultiMap<int, IXmppUriHandler *> FHandlers;
};

#endif // XMPPURIQUERIES_H
