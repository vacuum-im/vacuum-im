#include "xmppstreammanager.h"

#include <definitions/internalerrors.h>
#include <utils/options.h>
#include <utils/logger.h>

XmppStreamManager::XmppStreamManager()
{

}

XmppStreamManager::~XmppStreamManager()
{

}

void XmppStreamManager::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("XMPP Streams Manager");
	APluginInfo->description = tr("Allows other modules to create XMPP streams and get access to them");
	APluginInfo ->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
}

bool XmppStreamManager::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(APluginManager); Q_UNUSED(AInitOrder);
	return true;
}

bool XmppStreamManager::initObjects()
{
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_XMPPSTREAM_DESTROYED,tr("XMPP stream destroyed"));
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_XMPPSTREAM_NOT_SECURE,tr("Secure connection is not established"));
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_XMPPSTREAM_CLOSED_UNEXPECTEDLY,tr("Connection closed unexpectedly"));
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_XMPPSTREAM_FAILED_START_CONNECTION,tr("Failed to start connection"));
	return true;
}

bool XmppStreamManager::initSettings()
{
	Options::setDefaultValue(OPV_XMPPSTREAMS_TIMEOUT_HANDSHAKE,60000);
	Options::setDefaultValue(OPV_XMPPSTREAMS_TIMEOUT_KEEPALIVE,30000);
	Options::setDefaultValue(OPV_XMPPSTREAMS_TIMEOUT_DISCONNECT,5000);
	return true;
}

QList<IXmppStream *> XmppStreamManager::xmppStreams() const
{
	return FStreams;
}

IXmppStream *XmppStreamManager::findXmppStream(const Jid &AStreamJid) const
{
	foreach(IXmppStream *stream, FStreams)
		if (stream->streamJid() == AStreamJid)
			return stream;
	return NULL;
}

IXmppStream *XmppStreamManager::createXmppStream(const Jid &AStreamJid)
{
	IXmppStream *stream = findXmppStream(AStreamJid);
	if (!stream)
	{
		LOG_STRM_INFO(AStreamJid,"XMPP stream created");
		stream = new XmppStream(this, AStreamJid);
		connect(stream->instance(), SIGNAL(streamDestroyed()), SLOT(onXmppStreamDestroyed()));
		FStreams.append(stream);
		emit streamCreated(stream);
	}
	return stream;
}

void XmppStreamManager::destroyXmppStream(IXmppStream *AXmppStream)
{
	if (AXmppStream)
	{
		LOG_STRM_INFO(AXmppStream->streamJid(),QString("Destroying XMPP stream"));
		delete AXmppStream->instance();
	}
}

bool XmppStreamManager::isXmppStreamActive(IXmppStream *AXmppStream) const
{
	return FActiveStreams.contains(AXmppStream);
}

void XmppStreamManager::setXmppStreamActive(IXmppStream *AXmppStream, bool AActive)
{
	if (AActive && !FActiveStreams.contains(AXmppStream))
	{
		LOG_STRM_INFO(AXmppStream->streamJid(),"Activating XMPP stream");
		connect(AXmppStream->instance(), SIGNAL(opened()), SLOT(onXmppStreamOpened()));
		connect(AXmppStream->instance(), SIGNAL(closed()), SLOT(onXmppStreamClosed()));
		connect(AXmppStream->instance(), SIGNAL(aboutToClose()), SLOT(onXmppStreamAboutToClose()));
		connect(AXmppStream->instance(), SIGNAL(error(const XmppError &)), SLOT(onXmppStreamError(const XmppError &)));
		connect(AXmppStream->instance(), SIGNAL(jidAboutToBeChanged(const Jid &)), SLOT(onXmppStreamJidAboutToBeChanged(const Jid &)));
		connect(AXmppStream->instance(), SIGNAL(jidChanged(const Jid &)), SLOT(onXmppStreamJidChanged(const Jid &)));
		connect(AXmppStream->instance(), SIGNAL(connectionChanged(IConnection *)), SLOT(onXmppStreamConnectionChanged(IConnection *)));
		FActiveStreams.append(AXmppStream);
		emit streamActiveChanged(AXmppStream,AActive);
	}
	else if (!AActive && FActiveStreams.contains(AXmppStream))
	{
		LOG_STRM_INFO(AXmppStream->streamJid(),"Deactivating XMPP stream");
		disconnect(AXmppStream->instance(), SIGNAL(opened()), this, SLOT(onXmppStreamOpened()));
		disconnect(AXmppStream->instance(), SIGNAL(closed()), this, SLOT(onXmppStreamClosed()));
		disconnect(AXmppStream->instance(), SIGNAL(aboutToClose()), this, SLOT(onXmppStreamAboutToClose()));
		disconnect(AXmppStream->instance(), SIGNAL(error(const XmppError &)), this, SLOT(onXmppStreamError(const XmppError &)));
		disconnect(AXmppStream->instance(), SIGNAL(jidAboutToBeChanged(const Jid &)), this, SLOT(onXmppStreamJidAboutToBeChanged(const Jid &)));
		disconnect(AXmppStream->instance(), SIGNAL(jidChanged(const Jid &)), this, SLOT(onXmppStreamJidChanged(const Jid &)));
		disconnect(AXmppStream->instance(), SIGNAL(connectionChanged(IConnection *)), this, SLOT(onXmppStreamConnectionChanged(IConnection *)));
		FActiveStreams.removeAll(AXmppStream);
		emit streamActiveChanged(AXmppStream,AActive);
	}
}

QList<QString> XmppStreamManager::xmppFeatures() const
{
	return FFeatureOrders.values();
}

void XmppStreamManager::registerXmppFeature(int AOrder, const QString &AFeatureNS)
{
	if (!AFeatureNS.isEmpty() && !FFeatureOrders.values().contains(AFeatureNS))
	{
		LOG_DEBUG(QString("XMPP feature registered, order=%1, feature=%2").arg(AOrder).arg(AFeatureNS));
		FFeatureOrders.insertMulti(AOrder,AFeatureNS);
		emit xmppFeatureRegistered(AOrder,AFeatureNS);
	}
}

QList<IXmppFeatureFactory *> XmppStreamManager::xmppFeatureFactories(const QString &AFeatureNS) const
{
	return FFeatureFactories.value(AFeatureNS).values();
}

void XmppStreamManager::registerXmppFeatureFactory(int AOrder, const QString &AFeatureNS, IXmppFeatureFactory *AFactory)
{
	if (AFactory && !AFeatureNS.isEmpty())
	{
		LOG_DEBUG(QString("XMPP feature factory registered, order=%1, feature=%2, factory=%3").arg(AOrder).arg(AFeatureNS,AFactory->instance()->metaObject()->className()));
		FFeatureFactories[AFeatureNS].insertMulti(AOrder,AFactory);
		emit xmppFeatureFactoryRegistered(AOrder,AFeatureNS,AFactory);
	}
}

void XmppStreamManager::onXmppStreamOpened()
{
	IXmppStream *stream = qobject_cast<IXmppStream *>(sender());
	if (stream)
		emit streamOpened(stream);
}

void XmppStreamManager::onXmppStreamAboutToClose()
{
	IXmppStream *stream = qobject_cast<IXmppStream *>(sender());
	if (stream)
		emit streamAboutToClose(stream);
}
void XmppStreamManager::onXmppStreamClosed()
{
	IXmppStream *stream = qobject_cast<IXmppStream *>(sender());
	if (stream)
		emit streamClosed(stream);
}

void XmppStreamManager::onXmppStreamError(const XmppError &AError)
{
	IXmppStream *stream = qobject_cast<IXmppStream *>(sender());
	if (stream)
		emit streamError(stream,AError);
}

void XmppStreamManager::onXmppStreamJidAboutToBeChanged(const Jid &AAfter)
{
	IXmppStream *stream = qobject_cast<IXmppStream *>(sender());
	if (stream)
		emit streamJidAboutToBeChanged(stream,AAfter);
}

void XmppStreamManager::onXmppStreamJidChanged(const Jid &ABefore)
{
	IXmppStream *stream = qobject_cast<IXmppStream *>(sender());
	if (stream)
		emit streamJidChanged(stream,ABefore);
}

void XmppStreamManager::onXmppStreamConnectionChanged(IConnection *AConnection)
{
	IXmppStream *stream = qobject_cast<IXmppStream *>(sender());
	if (stream)
		emit streamConnectionChanged(stream,AConnection);
}

void XmppStreamManager::onXmppStreamDestroyed()
{
	IXmppStream *stream = qobject_cast<IXmppStream *>(sender());
	if (stream)
	{
		setXmppStreamActive(stream,false);
		FStreams.removeAll(stream);
		emit streamDestroyed(stream);
		LOG_STRM_INFO(stream->streamJid(),"XMPP stream destroyed");
	}
}
