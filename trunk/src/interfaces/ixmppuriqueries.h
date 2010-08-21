#ifndef IXMPPURIQUERIES_H
#define IXMPPURIQUERIES_H

#include <QUrl>
#include <QMultiMap>
#include <utils/jid.h>

#define XMPPURIQUERIES_UUID "{fdd3ab79-25da-4f96-a3d5-add3188047aa}"

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
	virtual void insertUriHandler(IXmppUriHandler *AHandler, int AOrder) =0;
	virtual void removeUriHandler(IXmppUriHandler *AHandler, int AOrder) =0;
protected:
	virtual void uriHandlerInserted(IXmppUriHandler *AHandler, int AOrder) =0;
	virtual void uriHandlerRemoved(IXmppUriHandler *AHandler, int AOrder) =0;
};

Q_DECLARE_INTERFACE(IXmppUriHandler,"Vacuum.Plugin.IXmppUriHandler/1.0")
Q_DECLARE_INTERFACE(IXmppUriQueries,"Vacuum.Plugin.IXmppUriQueries/1.0")

#endif
