#include "xmppuriqueries.h"

#include <QPair>
#include <QUrlQuery>
#include <definitions/messageviewurlhandlerorders.h>
#include <utils/logger.h>

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
		FMessageWidgets->insertViewUrlHandler(MVUHO_XMPPURIQUERIES, this);
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
	Jid contactJid;
	QString action;
	QMultiMap<QString, QString> params;
	if (parseXmppUri(AUrl,contactJid,action,params))
	{
		LOG_STRM_INFO(AStreamJid,QString("Opening XMPP URI, url=%1").arg(AUrl.toString()));
		foreach (IXmppUriHandler *handler, FHandlers)
		{
			if (handler->xmppUriOpen(AStreamJid, contactJid, action, params))
				return true;
		}
		LOG_STRM_WARNING(AStreamJid,QString("Failed to open XMPP URI, url=%1").arg(AUrl.toString()));
	}
	return false;
}

bool XmppUriQueries::parseXmppUri(const QUrl &AUrl, Jid &AContactJid, QString &AAction, QMultiMap<QString, QString> &AParams) const
{
	if (AUrl.isValid() && AUrl.scheme()==XMPP_URI_SCHEME)
	{
		QUrl url =  QUrl::fromEncoded(AUrl.toEncoded().replace(';','&'), QUrl::StrictMode);

		QList< QPair<QString, QString> > keyValues = QUrlQuery(url).queryItems();
		if (!keyValues.isEmpty())
		{
			AContactJid = url.path();
			AAction = keyValues.takeAt(0).first;
			if (AContactJid.isValid() && !AAction.isEmpty())
			{
				for (int i=0; i<keyValues.count(); i++)
					AParams.insertMulti(keyValues.at(i).first, keyValues.at(i).second);
				return true;
			}
		}
	}
	return false;
}

QString XmppUriQueries::makeXmppUri(const Jid &AContactJid, const QString &AAction, const QMultiMap<QString, QString> &AParams) const
{
	if (AContactJid.isValid() && !AAction.isEmpty())
	{
		QUrl url;
		QUrlQuery query;
		query.setQueryDelimiters('=',';');

		url.setScheme(XMPP_URI_SCHEME);
		url.setPath(AContactJid.full());

		QList< QPair<QString, QString> > queryItems;
		queryItems.append(qMakePair<QString,QString>(AAction,QString::null));
		for(QMultiMap<QString, QString>::const_iterator it=AParams.constBegin(); it!=AParams.end(); ++it)
			queryItems.append(qMakePair<QString,QString>(it.key(),it.value()));
		query.setQueryItems(queryItems);
		url.setQuery(query);

		return url.toString().replace(QString("?%1=;").arg(AAction),QString("?%1;").arg(AAction));
	}
	return QString::null;
}

void XmppUriQueries::insertUriHandler(int AOrder, IXmppUriHandler *AHandler)
{
	if (!FHandlers.contains(AOrder, AHandler))
	{
		LOG_DEBUG(QString("URI handler inserted, order=%1, address=%2").arg(AOrder).arg((quint64)AHandler));
		FHandlers.insertMulti(AOrder, AHandler);
		emit uriHandlerInserted(AOrder, AHandler);
	}
}

void XmppUriQueries::removeUriHandler(int AOrder, IXmppUriHandler *AHandler)
{
	if (FHandlers.contains(AOrder, AHandler))
	{
		LOG_DEBUG(QString("URI handler removed, order=%1, address=%2").arg(AOrder).arg((quint64)AHandler));
		FHandlers.remove(AOrder, AHandler);
		emit uriHandlerRemoved(AOrder, AHandler);
	}
}
