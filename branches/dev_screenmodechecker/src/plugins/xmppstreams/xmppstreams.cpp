#include "xmppstreams.h"

XmppStreams::XmppStreams()
{

}

XmppStreams::~XmppStreams()
{

}

void XmppStreams::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("XMPP Streams Manager");
	APluginInfo->description = tr("Allows other modules to create XMPP streams and get access to them");
	APluginInfo ->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
}

bool XmppStreams::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(APluginManager); Q_UNUSED(AInitOrder);
	return true;
}

QList<IXmppStream *> XmppStreams::xmppStreams() const
{
	return FStreams;
}

IXmppStream *XmppStreams::xmppStream(const Jid &AStreamJid) const
{
	foreach(IXmppStream *stream,FStreams)
		if (stream->streamJid() == AStreamJid)
			return stream;
	return NULL;
}

IXmppStream *XmppStreams::newXmppStream(const Jid &AStreamJid)
{
	IXmppStream *stream = xmppStream(AStreamJid);
	if (!stream)
	{
		stream = new XmppStream(this, AStreamJid);
		connect(stream->instance(), SIGNAL(streamDestroyed()), SLOT(onStreamDestroyed()));
		FStreams.append(stream);
		emit created(stream);
	}
	return stream;
}

bool XmppStreams::isActive( IXmppStream *AXmppStream ) const
{
	return FActiveStreams.contains(AXmppStream);
}

void XmppStreams::addXmppStream(IXmppStream *AXmppStream)
{
	if (AXmppStream && !FActiveStreams.contains(AXmppStream))
	{
		connect(AXmppStream->instance(), SIGNAL(opened()), SLOT(onStreamOpened()));
		connect(AXmppStream->instance(), SIGNAL(aboutToClose()), SLOT(onStreamAboutToClose()));
		connect(AXmppStream->instance(), SIGNAL(closed()), SLOT(onStreamClosed()));
		connect(AXmppStream->instance(), SIGNAL(error(const QString &)), SLOT(onStreamError(const QString &)));
		connect(AXmppStream->instance(), SIGNAL(jidAboutToBeChanged(const Jid &)), SLOT(onStreamJidAboutToBeChanged(const Jid &)));
		connect(AXmppStream->instance(), SIGNAL(jidChanged(const Jid &)), SLOT(onStreamJidChanged(const Jid &)));
		connect(AXmppStream->instance(), SIGNAL(connectionChanged(IConnection *)), SLOT(onStreamConnectionChanged(IConnection *)));
		FActiveStreams.append(AXmppStream);
		emit added(AXmppStream);
	}
}

void XmppStreams::removeXmppStream(IXmppStream *AXmppStream)
{
	if (FActiveStreams.contains(AXmppStream))
	{
		if (AXmppStream->isConnected())
		{
			AXmppStream->close();
			AXmppStream->connection()->disconnectFromHost();
		}
		AXmppStream->instance()->disconnect(this);
		connect(AXmppStream->instance(), SIGNAL(streamDestroyed()),SLOT(onStreamDestroyed()));
		FActiveStreams.removeAt(FActiveStreams.indexOf(AXmppStream));
		emit removed(AXmppStream);
	}
}

void XmppStreams::destroyXmppStream(const Jid &AStreamJid)
{
	IXmppStream *stream = xmppStream(AStreamJid);
	if (stream)
		delete stream->instance();
}

QList<QString> XmppStreams::xmppFeatures() const
{
	return FFeatureOrders.values();
}

void XmppStreams::registerXmppFeature(int AOrder, const QString &AFeatureNS)
{
	if (!AFeatureNS.isEmpty() && !FFeatureOrders.values().contains(AFeatureNS))
	{
		FFeatureOrders.insertMulti(AOrder,AFeatureNS);
		emit xmppFeatureRegistered(AOrder,AFeatureNS);
	}
}

QList<IXmppFeaturesPlugin *> XmppStreams::xmppFeaturePlugins(const QString &AFeatureNS) const
{
	return FFeaturePlugins.value(AFeatureNS).values();
}

void XmppStreams::registerXmppFeaturePlugin(int AOrder, const QString &AFeatureNS, IXmppFeaturesPlugin *AFeaturePlugin)
{
	if (AFeaturePlugin && !AFeatureNS.isEmpty())
	{
		FFeaturePlugins[AFeatureNS].insertMulti(AOrder,AFeaturePlugin);
		emit xmppFeaturePluginRegistered(AOrder,AFeatureNS,AFeaturePlugin);
	}
}

void XmppStreams::onStreamOpened()
{
	IXmppStream *stream = qobject_cast<IXmppStream *>(sender());
	if (stream)
		emit opened(stream);
}

void XmppStreams::onStreamAboutToClose()
{
	IXmppStream *stream = qobject_cast<IXmppStream *>(sender());
	if (stream)
		emit aboutToClose(stream);
}
void XmppStreams::onStreamClosed()
{
	IXmppStream *stream = qobject_cast<IXmppStream *>(sender());
	if (stream)
		emit closed(stream);
}

void XmppStreams::onStreamError(const QString &AError)
{
	IXmppStream *stream = qobject_cast<IXmppStream *>(sender());
	if (stream)
		emit error(stream,AError);
}

void XmppStreams::onStreamJidAboutToBeChanged(const Jid &AAfter)
{
	IXmppStream *stream = qobject_cast<IXmppStream *>(sender());
	if (stream)
		emit jidAboutToBeChanged(stream,AAfter);
}

void XmppStreams::onStreamJidChanged(const Jid &ABefore)
{
	IXmppStream *stream = qobject_cast<IXmppStream *>(sender());
	if (stream)
		emit jidChanged(stream,ABefore);
}

void XmppStreams::onStreamConnectionChanged(IConnection *AConnection)
{
	IXmppStream *stream = qobject_cast<IXmppStream *>(sender());
	if (stream)
		emit connectionChanged(stream,AConnection);
}

void XmppStreams::onStreamDestroyed()
{
	IXmppStream *stream = qobject_cast<IXmppStream *>(sender());
	if (stream)
	{
		removeXmppStream(stream);
		FStreams.removeAt(FStreams.indexOf(stream));
		emit streamDestroyed(stream);
	}
}

Q_EXPORT_PLUGIN2(plg_xmppstreams, XmppStreams)
