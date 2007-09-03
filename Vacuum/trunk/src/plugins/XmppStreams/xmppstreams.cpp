#include <QtDebug>
#include "xmppstreams.h"

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
  ErrorHandler::addErrorItem(NS_XMPP_STREAMS, "bad-format",
    ErrorHandler::MODIFY, ErrorHandler::BAD_REQUEST, tr("Bad Request Format"));

  ErrorHandler::addErrorItem(NS_XMPP_STREAMS, "bad-namespace-prefix",
    ErrorHandler::MODIFY, ErrorHandler::BAD_REQUEST, tr("Bad Namespace Prefix"));

  ErrorHandler::addErrorItem(NS_XMPP_STREAMS, "conflict",
    ErrorHandler::MODIFY, ErrorHandler::CONFLICT, tr("Conflict"));

  ErrorHandler::addErrorItem(NS_XMPP_STREAMS, "connection-timeout",
    ErrorHandler::CANCEL, ErrorHandler::DISCONNECTED, tr("Connection timeout"));

  ErrorHandler::addErrorItem(NS_XMPP_STREAMS, "host-gone",
    ErrorHandler::WAIT, ErrorHandler::GONE, tr("Host Gone"));

  ErrorHandler::addErrorItem(NS_XMPP_STREAMS, "host-unknown",
    ErrorHandler::CANCEL, ErrorHandler::REMOTE_SERVER_NOT_FOUND, tr("Host Unknown"));

  ErrorHandler::addErrorItem(NS_XMPP_STREAMS, "improper-addressing",
    ErrorHandler::MODIFY, ErrorHandler::JID_MALFORMED, tr("Improper Addressing"));

  ErrorHandler::addErrorItem(NS_XMPP_STREAMS, "internal-server-error",
    ErrorHandler::CANCEL, ErrorHandler::INTERNAL_SERVER_ERROR, tr("Internal Server Error"));

  ErrorHandler::addErrorItem(NS_XMPP_STREAMS, "invalid-from",
    ErrorHandler::MODIFY, ErrorHandler::JID_MALFORMED, tr("Invalid From"));

  ErrorHandler::addErrorItem(NS_XMPP_STREAMS, "invalid-id",
    ErrorHandler::MODIFY, ErrorHandler::NOT_ACCEPTABLE, tr("Invalid Id"));

  ErrorHandler::addErrorItem(NS_XMPP_STREAMS, "invalid-namespace",
    ErrorHandler::MODIFY, ErrorHandler::NOT_ACCEPTABLE, tr("Invalid Namespace"));

  ErrorHandler::addErrorItem(NS_XMPP_STREAMS, "invalid-xml",
    ErrorHandler::MODIFY, ErrorHandler::NOT_ACCEPTABLE, tr("Invalid XML"));

  ErrorHandler::addErrorItem(NS_XMPP_STREAMS, "not-authorized",
    ErrorHandler::AUTH, ErrorHandler::NOT_AUTHORIZED, tr("Not Authorized"));

  ErrorHandler::addErrorItem(NS_XMPP_STREAMS, "policy-violation",
    ErrorHandler::AUTH, ErrorHandler::FORBIDDEN, tr("Policy Violation"));

  ErrorHandler::addErrorItem(NS_XMPP_STREAMS, "remote-connection-failed",
    ErrorHandler::CANCEL, ErrorHandler::REMOTE_SERVER_NOT_FOUND, tr("Remote Connection Failed"));

  ErrorHandler::addErrorItem(NS_XMPP_STREAMS, "resource-constraint",
    ErrorHandler::CANCEL, ErrorHandler::RESOURCE_CONSTRAINT, tr("Resource Constraint"));

  ErrorHandler::addErrorItem(NS_XMPP_STREAMS, "restricted-xml",
    ErrorHandler::MODIFY, ErrorHandler::NOT_ACCEPTABLE, tr("Restricted XML"));

  ErrorHandler::addErrorItem(NS_XMPP_STREAMS, "see-other-host",
    ErrorHandler::MODIFY, ErrorHandler::NOT_ACCEPTABLE, tr("See Other Host"));

  ErrorHandler::addErrorItem(NS_XMPP_STREAMS, "system-shutdown",
    ErrorHandler::CANCEL, ErrorHandler::DISCONNECTED, tr("System Shutdown"));

  ErrorHandler::addErrorItem(NS_XMPP_STREAMS, "undefined-condition",
    ErrorHandler::CANCEL, ErrorHandler::UNDEFINED_CONDITION, tr("Undefined Condition"));

  ErrorHandler::addErrorItem(NS_XMPP_STREAMS, "unsupported-encoding",
    ErrorHandler::CANCEL, ErrorHandler::NOT_ACCEPTABLE, tr("Unsupported Encoding"));

  ErrorHandler::addErrorItem(NS_XMPP_STREAMS, "unsupported-stanza-type",
    ErrorHandler::CANCEL, ErrorHandler::FEATURE_NOT_IMPLEMENTED, tr("Unsupported Stanza Type"));

  ErrorHandler::addErrorItem(NS_XMPP_STREAMS, "unsupported-version",
    ErrorHandler::CANCEL, ErrorHandler::FEATURE_NOT_IMPLEMENTED, tr("Unsupported Version"));

  ErrorHandler::addErrorItem(NS_XMPP_STREAMS, "xml-not-well-formed",
    ErrorHandler::CANCEL, ErrorHandler::NOT_ACCEPTABLE, tr("XML Not Well Formed"));

  return true;
}

void XmppStreams::pluginInfo(PluginInfo *APluginInfo)
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
    stream = new XmppStream(AStreamJid, this);
    FStreams.append(stream);
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
    qDebug() << "Stream added" << AXmppStream->jid().full(); 
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
    FActiveStreams.append(AXmppStream);
    emit added(AXmppStream);
  }
}

void XmppStreams::removeStream(IXmppStream *AXmppStream)
{
  if (FActiveStreams.contains(AXmppStream))
  {
    disconnect(AXmppStream->instance(), SIGNAL(opened(IXmppStream *)), 
      this, SLOT(onStreamOpened(IXmppStream *)));
    disconnect(AXmppStream->instance(), SIGNAL(element(IXmppStream *, const QDomElement &)), 
      this, SLOT(onStreamElement(IXmppStream *, const QDomElement &))); 
    disconnect(AXmppStream->instance(), SIGNAL(aboutToClose(IXmppStream *)), 
      this, SLOT(onStreamAboutToClose(IXmppStream *))); 
    disconnect(AXmppStream->instance(), SIGNAL(closed(IXmppStream *)), 
      this, SLOT(onStreamClosed(IXmppStream *))); 
    disconnect(AXmppStream->instance(), SIGNAL(error(IXmppStream *, const QString &)), 
      this, SLOT(onStreamError(IXmppStream *, const QString &)));
    disconnect(AXmppStream->instance(), SIGNAL(jidAboutToBeChanged(IXmppStream *, const Jid &)), 
      this, SLOT(onStreamJidAboutToBeChanged(IXmppStream *, const Jid &))); 
    disconnect(AXmppStream->instance(), SIGNAL(jidChanged(IXmppStream *, const Jid &)), 
      this, SLOT(onStreamJidChanged(IXmppStream *, const Jid &))); 
    disconnect(AXmppStream->instance(), SIGNAL(connectionAdded(IXmppStream *, IConnection *)), 
      this, SLOT(onStreamConnectionAdded(IXmppStream *, IConnection *))); 
    disconnect(AXmppStream->instance(), SIGNAL(connectionRemoved(IXmppStream *, IConnection *)), 
      this, SLOT(onStreamConnectionRemoved(IXmppStream *, IConnection *))); 
    FActiveStreams.removeAt(FActiveStreams.indexOf(AXmppStream));
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
  qDebug() << "Stream error" << AXmppStream->jid().full() << AErrStr; 
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

Q_EXPORT_PLUGIN2(XmppStreamsPlugin, XmppStreams)
