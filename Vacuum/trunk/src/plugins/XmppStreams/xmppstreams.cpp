#include "xmppstreams.h"
#include <QtDebug>
#include "utils/errorhandler.h"
#include "xmppstream.h"


XmppStreams::XmppStreams()
{
  FPluginManager = 0;
  FSettings = 0;
}

XmppStreams::~XmppStreams()
{
  qDebug() << "~XmppStreams";
}

bool XmppStreams::initPlugin(IPluginManager *APluginManager) 
{
  FPluginManager = APluginManager;
  QList<IPlugin *> plugins = FPluginManager->getPlugins("ISettingsPlugin");
  if (plugins.count() > 0)
  {
    ISettingsPlugin *settingsPlugin = qobject_cast<ISettingsPlugin *>(plugins[0]->instance());
    if (settingsPlugin)
      FSettings = settingsPlugin->newSettings(XMPPSTREAMS_UUID,this);
  }

  if (FSettings)
  {
    connect(FSettings->instance(),SIGNAL(opened()),SLOT(onConfigOpened()));
    connect(FSettings->instance(),SIGNAL(closed()),SLOT(onConfigClosed()));
  }
  else
  {
    qDebug() << "SETTINGS FOR XMPPSTREAMS NOT FOUND";
    return false;
  }

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

bool XmppStreams::startPlugin()
{
  return true;
}

void XmppStreams::getPluginInfo(PluginInfo *info)
{
  info->author = "Potapov S.A. aka Lion";
  info->description = tr("Managing XMPP streams, implementation of XMPP-Core");
  info->homePage = "http://jrudevels.org";
  info->name = tr("XMPP Streams Manager"); 
  info->uid = XMPPSTREAMS_UUID;
  info ->version = "0.1";
  info->dependences.append("{6030FCB2-9F1E-4ea2-BE2B-B66EBE0C4367}"); //ISettings  
}

//IXmppStreams
IXmppStream *XmppStreams::newStream(const Jid &AJid)
{
  IXmppStream *stream = getStream(AJid);
  if (!stream)
  {
    stream = new XmppStream(AJid, this);
    FStreams.append(stream);
  }
  return stream;
}

IXmppStream *XmppStreams::getStream(const Jid &AJid) const
{
  foreach(IXmppStream *stream,FStreams)
    if (stream->jid() == AJid) 
      return stream;
  return 0;
}

bool XmppStreams::addStream(IXmppStream *AStream)
{
  QObject *obj = AStream->instance();
  bool connected =
    connect(obj, SIGNAL(opened(IXmppStream *)), 
      SLOT(onStreamOpened(IXmppStream *))) &&
     connect(obj, SIGNAL(element(IXmppStream *, const QDomElement &)), 
      SLOT(onStreamElement(IXmppStream *, const QDomElement &))) && 
    connect(obj, SIGNAL(aboutToClose(IXmppStream *)), 
      SLOT(onStreamAboutToClose(IXmppStream *))) && 
    connect(obj, SIGNAL(closed(IXmppStream *)), 
      SLOT(onStreamClosed(IXmppStream *))) && 
    connect(obj, SIGNAL(error(IXmppStream *, const QString &)), 
      SLOT(onStreamError(IXmppStream *, const QString &)));
  if (connected)
  {
    FAddedStreams.append(AStream);
    qDebug() << "Stream added" << AStream->jid().full(); 
    emit added(AStream);
  } 
  else
    disconnect(obj,0,this,0);
  return connected;
}

void XmppStreams::removeStream(IXmppStream *AStream)
{
  if (!AStream)
    return;

  qDebug() << "Removing stream" << AStream->jid().full();

  AStream->close(); 
  
  if (FAddedStreams.contains(AStream))
  {
    emit removed(AStream);
    FAddedStreams.removeAt(FAddedStreams.indexOf(AStream));
  }
  
  if (FStreams.contains(AStream))
    FStreams.removeAt(FStreams.indexOf(AStream));
  
  delete (XmppStream *)AStream;
}

void XmppStreams::onStreamOpened(IXmppStream *AStream)
{
  qDebug() << "Stream opened" << AStream->jid().full(); 
  emit opened(AStream);
}

void XmppStreams::onStreamElement(IXmppStream *AStream, const QDomElement &elem)
{
  //qDebug() << "\nStream element" << AStream->jid().full() << ":";
  //qDebug() << elem.ownerDocument().toString(); 
  emit element(AStream, elem);
}

void XmppStreams::onStreamAboutToClose(IXmppStream *AStream)
{
  qDebug() << "Stream aboutToClose" << AStream->jid().full(); 
  emit aboutToClose(AStream);
}
void XmppStreams::onStreamClosed(IXmppStream *AStream)
{
  qDebug() << "Stream closed" << AStream->jid().full(); 
  emit closed(AStream);
}

void XmppStreams::onStreamError(IXmppStream *AStream, const QString &AErrStr)
{
  qDebug() << "Stream error" << AStream->jid().full() << AErrStr; 
  emit error(AStream,AErrStr);
}

void XmppStreams::onConfigOpened()
{
  IXmppStream *stream = newStream("Test2@potapov/Vacuum");
  stream->setDefaultLang("ru"); 
  stream->setXmppVersion("1.0");
  stream->setPassword("1");
  stream->connection()->setProxyType(0);
  stream->connection()->setProxyUsername("wrong");  
  stream->connection()->setProxyHost("220.194.57.126");
  stream->connection()->setProxyPort(1080);
  addStream(stream);
  stream->open(); 

  //deleteStream(stream);

  //QHash<QString,QVariant> streams = FSettings->values("stream"); 
  //QList<QString> NS = streams.keys(); 
  //for(int i=0;i<NS.count();i++)
  //{
  //  IXmppStream *stream = newStream(streams[NS[i]].toString());
  //  stream->setDefaultLang(FSettings->valueNS("stream[]:lang",NS[i],"ru").toString()); 
  //  stream->setXmppVersion(FSettings->valueNS("stream[]:version",NS[i],"1.0").toString());
  //  stream->setPassword(FSettings->valueNS("stream[]:password",NS[i],"").toString());
  //  stream->connection()->setProxyType(FSettings->valueNS("stream[]:connection:proxyType",NS[i],0).toInt());
  //  stream->connection()->setProxyHost(FSettings->valueNS("stream[]:connection:proxyHost",NS[i],"").toString());
  //  stream->connection()->setProxyPort(FSettings->valueNS("stream[]:connection:proxyPort",NS[i],1080).toInt());
  //  stream->connection()->setProxyPassword(FSettings->valueNS("stream[]:connection:proxyPassword",NS[i],"").toString());
  //  stream->connection()->setProxyUsername(FSettings->valueNS("stream[]:connection:proxyUsername",NS[i],"").toString());
  //  stream->connection()->setProxyPassword(FSettings->valueNS("stream[]:connection:proxyPassword",NS[i],"").toString());
  //  stream->connection()->setPollServer(FSettings->valueNS("stream[]:connection:pollServer",NS[i],"").toString());
  //  addStream(stream);
  //  if (FSettings->valueNS("stream[]:autoConnect",NS[i],true).toBool())
  //    stream->open(); 
  //}
}

void XmppStreams::onConfigClosed()
{
  while (FAddedStreams.count()>0) 
  {
    IXmppStream *stream = FAddedStreams[0];
    //QString NS = stream->jid().prep().full();
    //FSettings->setValueNS("stream",NS,stream->jid().full());     
    //FSettings->setValueNS("stream[]:version",NS,stream->xmppVersion());     
    //FSettings->setValueNS("stream[]:lang",NS,stream->defaultLang());     
    //FSettings->setValueNS("stream[]:password",NS,stream->password());
    //FSettings->setValueNS("stream[]:host",NS,stream->host());
    //FSettings->setValueNS("stream[]:port",NS,stream->port()); 
    //FSettings->setValueNS("stream[]:connection:proxyType",NS,stream->connection()->proxyType());  
    //FSettings->setValueNS("stream[]:connection:proxyHost",NS,stream->connection()->proxyHost());  
    //FSettings->setValueNS("stream[]:connection:proxyPort",NS,stream->connection()->proxyPort());  
    //FSettings->setValueNS("stream[]:connection:proxyUsername",NS,stream->connection()->proxyUsername()); 
    //FSettings->setValueNS("stream[]:connection:proxyPassword",NS,stream->connection()->proxyPassword());  
    //FSettings->setValueNS("stream[]:connection:pollServer",NS,stream->connection()->pollServer());  
    removeStream(stream);
  }
}

Q_EXPORT_PLUGIN2(XmppStreamsPlugin, XmppStreams)
