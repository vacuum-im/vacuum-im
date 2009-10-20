#include "xmppstreams.h"

#include "../../definations/namespaces.h"
#include "../../utils/errorhandler.h"
#include "xmppstream.h"

XmppStreams::XmppStreams()
{

}

XmppStreams::~XmppStreams()
{

}

void XmppStreams::pluginInfo(IPluginInfo *APluginInfo)
{
  APluginInfo->name = tr("XMPP Streams Manager"); 
  APluginInfo->description = tr("Managing XMPP streams, implementation of XMPP-Core");
  APluginInfo ->version = "1.0";
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->homePage = "http://jrudevels.org";
}

bool XmppStreams::initObjects()
{
  ErrorHandler::addErrorItem("bad-format", ErrorHandler::MODIFY, 
    ErrorHandler::BAD_REQUEST, tr("Bad Request Format"), NS_XMPP_STREAMS);

  ErrorHandler::addErrorItem("bad-namespace-prefix", ErrorHandler::MODIFY,
    ErrorHandler::BAD_REQUEST, tr("Bad Namespace Prefix"), NS_XMPP_STREAMS);

  ErrorHandler::addErrorItem("conflict", ErrorHandler::MODIFY, 
    ErrorHandler::CONFLICT, tr("Conflict"), NS_XMPP_STREAMS);

  ErrorHandler::addErrorItem("connection-timeout", ErrorHandler::CANCEL, 
    ErrorHandler::DISCONNECTED, tr("Connection timeout"), NS_XMPP_STREAMS);

  ErrorHandler::addErrorItem("host-gone", ErrorHandler::WAIT, 
    ErrorHandler::GONE, tr("Host Gone"), NS_XMPP_STREAMS);

  ErrorHandler::addErrorItem("host-unknown", ErrorHandler::CANCEL, 
    ErrorHandler::REMOTE_SERVER_NOT_FOUND, tr("Host Unknown"), NS_XMPP_STREAMS);

  ErrorHandler::addErrorItem("improper-addressing", ErrorHandler::MODIFY, 
    ErrorHandler::JID_MALFORMED, tr("Improper Addressing"), NS_XMPP_STREAMS);

  ErrorHandler::addErrorItem("internal-server-error", ErrorHandler::CANCEL, 
    ErrorHandler::INTERNAL_SERVER_ERROR, tr("Internal Server Error"), NS_XMPP_STREAMS);

  ErrorHandler::addErrorItem("invalid-from", ErrorHandler::MODIFY, 
    ErrorHandler::JID_MALFORMED, tr("Invalid From"), NS_XMPP_STREAMS);

  ErrorHandler::addErrorItem("invalid-id", ErrorHandler::MODIFY,
    ErrorHandler::NOT_ACCEPTABLE, tr("Invalid Id"), NS_XMPP_STREAMS);

  ErrorHandler::addErrorItem("invalid-namespace", ErrorHandler::MODIFY, 
    ErrorHandler::NOT_ACCEPTABLE, tr("Invalid Namespace"), NS_XMPP_STREAMS);

  ErrorHandler::addErrorItem("invalid-xml", ErrorHandler::MODIFY, 
    ErrorHandler::NOT_ACCEPTABLE, tr("Invalid XML"), NS_XMPP_STREAMS);

  ErrorHandler::addErrorItem("not-authorized", ErrorHandler::AUTH, 
    ErrorHandler::NOT_AUTHORIZED, tr("Not Authorized"), NS_XMPP_STREAMS);

  ErrorHandler::addErrorItem("policy-violation", ErrorHandler::AUTH, 
    ErrorHandler::FORBIDDEN, tr("Policy Violation"), NS_XMPP_STREAMS);

  ErrorHandler::addErrorItem("remote-connection-failed", ErrorHandler::CANCEL,
    ErrorHandler::REMOTE_SERVER_NOT_FOUND, tr("Remote Connection Failed"), NS_XMPP_STREAMS);

  ErrorHandler::addErrorItem("resource-constraint", ErrorHandler::CANCEL,
    ErrorHandler::RESOURCE_CONSTRAINT, tr("Resource Constraint"), NS_XMPP_STREAMS);

  ErrorHandler::addErrorItem("restricted-xml",
    ErrorHandler::MODIFY, ErrorHandler::NOT_ACCEPTABLE, tr("Restricted XML"), NS_XMPP_STREAMS);

  ErrorHandler::addErrorItem("see-other-host", ErrorHandler::MODIFY,
    ErrorHandler::NOT_ACCEPTABLE, tr("See Other Host"), NS_XMPP_STREAMS);

  ErrorHandler::addErrorItem("system-shutdown", ErrorHandler::CANCEL, 
    ErrorHandler::DISCONNECTED, tr("System Shutdown"), NS_XMPP_STREAMS);

  ErrorHandler::addErrorItem("undefined-condition", ErrorHandler::CANCEL, 
    ErrorHandler::UNDEFINED_CONDITION, tr("Undefined Condition"), NS_XMPP_STREAMS);

  ErrorHandler::addErrorItem("unsupported-encoding", ErrorHandler::CANCEL,
    ErrorHandler::NOT_ACCEPTABLE, tr("Unsupported Encoding"), NS_XMPP_STREAMS);

  ErrorHandler::addErrorItem("unsupported-stanza-type", ErrorHandler::CANCEL,
    ErrorHandler::FEATURE_NOT_IMPLEMENTED, tr("Unsupported Stanza Type"), NS_XMPP_STREAMS);

  ErrorHandler::addErrorItem("unsupported-version", ErrorHandler::CANCEL,
    ErrorHandler::FEATURE_NOT_IMPLEMENTED, tr("Unsupported Version"), NS_XMPP_STREAMS);

  ErrorHandler::addErrorItem("xml-not-well-formed", ErrorHandler::CANCEL,
    ErrorHandler::NOT_ACCEPTABLE, tr("XML Not Well Formed"), NS_XMPP_STREAMS);

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
    connect(stream->instance(), SIGNAL(destroyed()), SLOT(onStreamDestroyed())); 
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
    connect(AXmppStream->instance(), SIGNAL(element(const QDomElement &)), SLOT(onStreamElement(const QDomElement &))); 
    connect(AXmppStream->instance(), SIGNAL(consoleElement(const QDomElement &, bool)), SLOT(onStreamConsoleElement(const QDomElement &, bool))); 
    connect(AXmppStream->instance(), SIGNAL(aboutToClose()), SLOT(onStreamAboutToClose())); 
    connect(AXmppStream->instance(), SIGNAL(closed()), SLOT(onStreamClosed())); 
    connect(AXmppStream->instance(), SIGNAL(error(const QString &)), SLOT(onStreamError(const QString &)));
    connect(AXmppStream->instance(), SIGNAL(jidAboutToBeChanged(const Jid &)), SLOT(onStreamJidAboutToBeChanged(const Jid &))); 
    connect(AXmppStream->instance(), SIGNAL(jidChanged(const Jid &)), SLOT(onStreamJidChanged(const Jid &))); 
    connect(AXmppStream->instance(), SIGNAL(connectionAdded(IConnection *)), SLOT(onStreamConnectionAdded(IConnection *))); 
    connect(AXmppStream->instance(), SIGNAL(connectionRemoved(IConnection *)), SLOT(onStreamConnectionRemoved(IConnection *))); 
    connect(AXmppStream->instance(), SIGNAL(featureAdded(IStreamFeature *)), SLOT(onStreamFeatureAdded(IStreamFeature *))); 
    connect(AXmppStream->instance(), SIGNAL(featureRemoved(IStreamFeature *)), SLOT(onStreamFeatureRemoved(IStreamFeature *)));
    FActiveStreams.append(AXmppStream);
    emit added(AXmppStream);
  }
}

void XmppStreams::removeXmppStream(IXmppStream *AXmppStream)
{
  if (FActiveStreams.contains(AXmppStream))
  {
    AXmppStream->close();
    AXmppStream->instance()->disconnect(this);
    connect(AXmppStream->instance(), SIGNAL(destroyed()),SLOT(onStreamDestroyed())); 
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

IStreamFeaturePlugin *XmppStreams::featurePlugin(const QString &AFeatureNS) const
{
  return FFeatures.value(AFeatureNS,NULL);
}

void XmppStreams::registerFeature(const QString &AFeatureNS, IStreamFeaturePlugin *AFeaturePlugin)
{
  if (AFeaturePlugin && !FFeatures.contains(AFeatureNS))
  {
    FFeatures.insert(AFeatureNS,AFeaturePlugin);
    emit featureRegistered(AFeatureNS,AFeaturePlugin);
  }
  else if (!AFeaturePlugin && FFeatures.contains(AFeatureNS))
  {
    FFeatures.remove(AFeatureNS);
    emit featureRegistered(AFeatureNS,AFeaturePlugin);
  }
}

void XmppStreams::onStreamOpened()
{
  IXmppStream *stream = qobject_cast<IXmppStream *>(sender());
  if (stream)
    emit opened(stream);
}

void XmppStreams::onStreamElement(const QDomElement &elem)
{
  IXmppStream *stream = qobject_cast<IXmppStream *>(sender());
  if (stream)
    emit element(stream, elem);
}

void XmppStreams::onStreamConsoleElement(const QDomElement &AElem, bool ASended)
{
  IXmppStream *stream = qobject_cast<IXmppStream *>(sender());
  if (stream)
    emit consoleElement(stream,AElem,ASended);
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

void XmppStreams::onStreamJidChanged(const Jid &ABefour)
{
  IXmppStream *stream = qobject_cast<IXmppStream *>(sender());
  if (stream)
    emit jidChanged(stream,ABefour);
}

void XmppStreams::onStreamConnectionAdded(IConnection *AConnection)
{
  IXmppStream *stream = qobject_cast<IXmppStream *>(sender());
  if (stream)
    emit connectionAdded(stream,AConnection);
}

void XmppStreams::onStreamConnectionRemoved(IConnection *AConnection)
{
  IXmppStream *stream = qobject_cast<IXmppStream *>(sender());
  if (stream)
    emit connectionRemoved(stream,AConnection);
}

void XmppStreams::onStreamFeatureAdded(IStreamFeature *AFeature)
{
  IXmppStream *stream = qobject_cast<IXmppStream *>(sender());
  if (stream)
    emit featureAdded(stream,AFeature);
}

void XmppStreams::onStreamFeatureRemoved(IStreamFeature *AFeature)
{
  IXmppStream *stream = qobject_cast<IXmppStream *>(sender());
  if (stream)
    emit featureRemoved(stream,AFeature);
}

void XmppStreams::onStreamDestroyed()
{
  IXmppStream *stream = qobject_cast<IXmppStream *>(sender());
  if (stream)
  {
    removeXmppStream(stream);
    FStreams.removeAt(FStreams.indexOf(stream));
    emit destroyed(stream);
  }
}

Q_EXPORT_PLUGIN2(XmppStreamsPlugin, XmppStreams)
