#ifndef IXMPPURIQUERIES_H
#define IXMPPURIQUERIES_H

#include <QUrl>
#include <QMultiMap>
#include <utils/jid.h>

#define XMPPURIQUERIES_UUID "{fdd3ab79-25da-4f96-a3d5-add3188047aa}"

#define XMPP_URI_SCHEME     "xmpp"

class IXmppUriHandler
{
public:
	virtual bool xmppUriOpen(const Jid &AStreamJid, const Jid &AContactJid, const QString &AAction, const QMultiMap<QString, QString> &AParams) =0;
};

class IXmppUriQueries
{
public:
	virtual QObject *instance() =0;
	virtual bool openXmppUri(const Jid &AStreamJid, const QUrl &AUrl) const =0;
	virtual bool parseXmppUri(const QUrl &AUrl, Jid &AContactJid, QString &AAction, QMultiMap<QString, QString> &AParams) const =0;
	virtual QString makeXmppUri(const Jid &AContactJid, const QString &AAction, const QMultiMap<QString, QString> &AParams) const =0;
	virtual void insertUriHandler(int AOrder, IXmppUriHandler *AHandler) =0;
	virtual void removeUriHandler(int AOrder, IXmppUriHandler *AHandler) =0;
protected:
	virtual void uriHandlerInserted(int AOrder, IXmppUriHandler *AHandler) =0;
	virtual void uriHandlerRemoved(int AOrder, IXmppUriHandler *AHandler) =0;
};

Q_DECLARE_INTERFACE(IXmppUriHandler,"Vacuum.Plugin.IXmppUriHandler/1.0")
Q_DECLARE_INTERFACE(IXmppUriQueries,"Vacuum.Plugin.IXmppUriQueries/1.1")

#endif
