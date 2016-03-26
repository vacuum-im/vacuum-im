#ifndef IDATASTREAMSPUBLISHER_H
#define IDATASTREAMSPUBLISHER_H

#include <QMap>
#include <QVariant>
#include <QDateTime>
#include <QDomElement>
#include <utils/xmpperror.h>
#include <utils/jid.h>

#define DATASTREAMSPUBLISHER_UUID "{8F79D9E3-380D-4026-9869-FF86A93B6A87}"

struct IPublicDataStream {
	QString id;
	Jid ownerJid;
	QString profile;
	QString mimeType;
	QVariantMap params;
	inline bool isNull() const {
		return id.isEmpty();
	}
	inline bool isValid() const {
		return !id.isEmpty() && ownerJid.isValid() && !profile.isEmpty();
	}
};

class IPublicDataStreamHandler
{
public:
	virtual bool publicDataStreamCanStart(const IPublicDataStream &AStream) const =0;
	virtual bool publicDataStreamStart(const Jid &AStreamJid, const Jid AContactJid, const QString &ASessionId, const IPublicDataStream &AStream) =0;
	virtual bool publicDataStreamRead(IPublicDataStream &AStream, const QDomElement &ASiPub) const =0;
	virtual bool publicDataStreamWrite(const IPublicDataStream &AStream, QDomElement &ASiPub) const =0;
};

class IDataStreamsPublisher
{
public:
	virtual QObject *instance() =0;
	// Register Streams
	virtual QList<QString> streams() const =0;
	virtual IPublicDataStream findStream(const QString &AStreamId) const =0;
	virtual bool publishStream(const IPublicDataStream &AStream) =0;
	virtual void removeStream(const QString &AStreamId) =0;
	// Publish Streams
	virtual QList<IPublicDataStream> readStreams(const QDomElement &AParent) const =0;
	virtual bool writeStream(const QString &AStreamId, QDomElement &AParent) const =0;
	// Process Streams
	virtual bool isSupported(const Jid &AStreamJid, const Jid &AContactJid) const =0;
	virtual QString startStream(const Jid &AStreamJid, const Jid &AContactJid, const QString &AStreamId) =0;
	// Stream Handlers
	virtual QList<IPublicDataStreamHandler *> streamHandlers() const =0;
	virtual void insertStreamHandler(int AOrder, IPublicDataStreamHandler *AHandler) =0;
	virtual void removeStreamHandler(int AOrder, IPublicDataStreamHandler *AHandler) =0;
protected:
	virtual void streamPublished(const IPublicDataStream &AStream) =0;
	virtual void streamRemoved(const IPublicDataStream &AStream) =0;
	virtual void streamStartAccepted(const QString &ARequestId, const QString &ASessionId) =0;
	virtual void streamStartRejected(const QString &ARequestId, const XmppError &AError) =0;
	virtual void streamHandlerInserted(int AOrder, IPublicDataStreamHandler *AHandler) =0;
	virtual void streamHandlerRemoved(int AOrder, IPublicDataStreamHandler *AHandler) =0;
};

Q_DECLARE_INTERFACE(IPublicDataStreamHandler,"Vacuum.Plugin.IPublicDataStreamHandler/1.0")
Q_DECLARE_INTERFACE(IDataStreamsPublisher,"Vacuum.Plugin.IDataStreamsPublisher/1.0")

#endif // IDATASTREAMSPUBLISHER_H
