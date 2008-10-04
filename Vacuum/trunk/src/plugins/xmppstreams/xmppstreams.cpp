#include "xmppstreams.h"

#include <QtDebug>
#include <QIcon>
#include "../../definations/namespaces.h"
#include "../../utils/errorhandler.h"
#include "xmppstream.h"

XmppStreams::XmppStreams()
{

}

XmppStreams::~XmppStreams()
{

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

void XmppStreams::pluginInfo(IPluginInfo *APluginInfo)
{
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->description = tr("Managing XMPP streams, implementation of XMPP-Core");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = tr("XMPP Streams Manager"); 
  APluginInfo->uid = XMPPSTREAMS_UUID;
  APluginInfo ->version = "0.1";
}

//IXmppStreams
IXmppStream *XmppStreams::newStream(const Jid &AStreamJid)
{
  IXmppStream *stream = getStream(AStreamJid);
  if (!stream)
  {
    stream = new XmppStream(this, AStreamJid);
    FStreams.append(stream);
    emit created(stream);
  }
  return stream;
}

IXmppStream *XmppStreams::getStream(const Jid &AStreamJid) const
{
  foreach(IXmppStream *stream,FStreams)
    if (stream->jid() == AStreamJid) 
      return stream;
  return NULL;
}

void XmppStreams::addStream(IXmppStream *AXmppStream)
{
  if (AXmppStream && !FActiveStreams.contains(AXmppStream))
  {
    connect(AXmppStream->instance(), SIGNAL(opened(IXmppStream *)), 
      SLOT(onStreamOpened(IXmppStream *)));
    connect(AXmppStream->instance(), SIGNAL(element(IXmppStream *, const QDomElement &)), 
      SLOT(onStreamElement(IXmppStream *, const QDomElement &))); 
    connect(AXmppStream->instance(), SIGNAL(aboutToClose(IXmppStream *)), 
      SLOT(onStreamAboutToClose(IXmppStream *))); 
    connect(AXmppStream->instance(), SIGNAL(closed(IXmppStream *)), 
      SLOT(onStreamClosed(IXmppStream *))); 
    connect(AXmppStream->instance(), SIGNAL(error(IXmppStream *, const QString &)), 
      SLOT(onStreamError(IXmppStream *, const QString &)));
    connect(AXmppStream->instance(), SIGNAL(jidAboutToBeChanged(IXmppStream *, const Jid &)), 
      SLOT(onStreamJidAboutToBeChanged(IXmppStream *, const Jid &))); 
    connect(AXmppStream->instance(), SIGNAL(jidChanged(IXmppStream *, const Jid &)), 
      SLOT(onStreamJidChanged(IXmppStream *, const Jid &))); 
    connect(AXmppStream->instance(), SIGNAL(connectionAdded(IXmppStream *, IConnection *)), 
      SLOT(onStreamConnectionAdded(IXmppStream *, IConnection *))); 
    connect(AXmppStream->instance(), SIGNAL(connectionRemoved(IXmppStream *, IConnection *)), 
      SLOT(onStreamConnectionRemoved(IXmppStream *, IConnection *))); 
    connect(AXmppStream->instance(), SIGNAL(featureAdded(IXmppStream *, IStreamFeature *)), 
      SLOT(onStreamFeatureAdded(IXmppStream *, IStreamFeature *))); 
    connect(AXmppStream->instance(), SIGNAL(featureRemoved(IXmppStream *, IStreamFeature *)), 
      SLOT(onStreamFeatureRemoved(IXmppStream *, IStreamFeature *)));
    connect(AXmppStream->instance(), SIGNAL(destroyed(IXmppStream *)), 
      SLOT(onStreamDestroyed(IXmppStream *))); 
    FActiveStreams.append(AXmppStream);
    qDebug() << "Stream added" << AXmppStream->jid().full(); 
    emit added(AXmppStream);
  }
}

void XmppStreams::removeStream(IXmppStream *AXmppStream)
{
  if (FActiveStreams.contains(AXmppStream))
  {
    AXmppStream->close();
    AXmppStream->instance()->disconnect(this);
    FActiveStreams.removeAt(FActiveStreams.indexOf(AXmppStream));
    qDebug() << "Stream removed" << AXmppStream->jid().full(); 
    emit removed(AXmppStream);
  }
}

void XmppStreams::destroyStream(const Jid &AStreamJid)
{
  XmppStream *stream = (XmppStream *)getStream(AStreamJid);
  if (stream)
  {
    removeStream(stream);
    FStreams.removeAt(FStreams.indexOf(stream));
    delete stream;
  }
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

void XmppStreams::onStreamOpened(IXmppStream *AXmppStream)
{
  emit opened(AXmppStream);
}

void XmppStreams::onStreamElement(IXmppStream *AXmppStream, const QDomElement &elem)
{
  emit element(AXmppStream, elem);
}

void XmppStreams::onStreamAboutToClose(IXmppStream *AXmppStream)
{
  emit aboutToClose(AXmppStream);
}
void XmppStreams::onStreamClosed(IXmppStream *AXmppStream)
{
  emit closed(AXmppStream);
}

void XmppStreams::onStreamError(IXmppStream *AXmppStream, const QString &AErrStr)
{
  emit error(AXmppStream,AErrStr);
}

void XmppStreams::onStreamJidAboutToBeChanged(IXmppStream *AXmppStream, const Jid &AAfter)
{
  emit jidAboutToBeChanged(AXmppStream,AAfter);
}

void XmppStreams::onStreamJidChanged(IXmppStream *AXmppStream, const Jid &ABefour)
{
  emit jidChanged(AXmppStream,ABefour);
}

void XmppStreams::onStreamConnectionAdded(IXmppStream *AXmppStream, IConnection *AConnection)
{
  emit connectionAdded(AXmppStream,AConnection);
}

void XmppStreams::onStreamConnectionRemoved(IXmppStream *AXmppStream, IConnection *AConnection)
{
  emit connectionRemoved(AXmppStream,AConnection);
}

void XmppStreams::onStreamFeatureAdded(IXmppStream *AXmppStream, IStreamFeature *AFeature)
{
  emit featureAdded(AXmppStream,AFeature);
}

void XmppStreams::onStreamFeatureRemoved(IXmppStream *AXmppStream, IStreamFeature *AFeature)
{
  emit featureRemoved(AXmppStream,AFeature);
}

void XmppStreams::onStreamDestroyed(IXmppStream *AXmppStream)
{
  emit destroyed(AXmppStream);
}

Q_EXPORT_PLUGIN2(XmppStreamsPlugin, XmppStreams)
