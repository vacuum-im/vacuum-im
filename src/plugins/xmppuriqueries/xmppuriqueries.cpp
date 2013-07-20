#include "xmppuriqueries.h"

#include <QPair>
#include <QUrlQuery>

XmppUriQueries::XmppUriQueries()
{
	FMessageWidgets = NULL;
}

XmppUriQueries::~XmppUriQueries()
{

}

void XmppUriQueries::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("XMPP URI Queries");
	APluginInfo->description = tr("Allows other plugins to handle XMPP URI queries");
	APluginInfo ->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
}

bool XmppUriQueries::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
	IPlugin *plugin = APluginManager->pluginInterface("IMessageWidgets").value(0,NULL);
	if (plugin)
	{
		FMessageWidgets = qobject_cast<IMessageWidgets *>(plugin->instance());
	}
	return true;
}

bool XmppUriQueries::initObjects()
{
	if (FMessageWidgets)
	{
		FMessageWidgets->insertViewUrlHandler(MVUHO_XMPPURIQUERIES, this);
	}
	return true;
}

bool XmppUriQueries::messageViewUrlOpen(int AOrder, IMessageViewWidget *AWidget, const QUrl &AUrl)
{
	if (AOrder == MVUHO_XMPPURIQUERIES)
		return openXmppUri(AWidget->messageWindow()->streamJid(), AUrl);
	return false;
}

bool XmppUriQueries::openXmppUri(const Jid &AStreamJid, const QUrl &AUrl) const
{
	if (AUrl.isValid() && AUrl.scheme()=="xmpp")
	{
		QUrl url =  QUrl::fromEncoded(AUrl.toEncoded().replace(';','&'), QUrl::StrictMode);
		Jid contactJid = url.path();

		QList< QPair<QString, QString> > keyValues = QUrlQuery(url).queryItems();
		if (keyValues.count() > 0)
		{
			QString action = keyValues.takeAt(0).first;
			if (contactJid.isValid() && !action.isEmpty())
			{
				QMultiMap<QString, QString> params;
				for (int i=0; i<keyValues.count(); i++)
					params.insertMulti(keyValues.at(i).first, keyValues.at(i).second);

				foreach (IXmppUriHandler *handler, FHandlers)
					if (handler->xmppUriOpen(AStreamJid, contactJid, action, params))
						return true;
			}
		}
	}
	return false;
}

void XmppUriQueries::insertUriHandler(IXmppUriHandler *AHandler, int AOrder)
{
	if (!FHandlers.contains(AOrder, AHandler))
	{
		FHandlers.insertMulti(AOrder, AHandler);
		emit uriHandlerInserted(AHandler, AOrder);
	}
}

void XmppUriQueries::removeUriHandler(IXmppUriHandler *AHandler, int AOrder)
{
	if (FHandlers.contains(AOrder, AHandler))
	{
		FHandlers.remove(AOrder, AHandler);
		emit uriHandlerRemoved(AHandler, AOrder);
	}
}
