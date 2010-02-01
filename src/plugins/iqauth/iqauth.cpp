#include "iqauth.h"

#include <QCryptographicHash>

IqAuth::IqAuth(IXmppStream *AXmppStream) : QObject(AXmppStream->instance())
{
  FXmppStream = AXmppStream;
}

IqAuth::~IqAuth()
{
  FXmppStream->removeXmppElementHandler(this,XEHO_XMPP_FEATURE);
  emit featureDestroyed();
}

bool IqAuth::xmppElementIn(IXmppStream *AXmppStream, QDomElement &AElem, int AOrder)
{
  if (AXmppStream==FXmppStream && AOrder==XEHO_XMPP_FEATURE)
  {
    FXmppStream->removeXmppElementHandler(this,XEHO_XMPP_FEATURE);
    if (AElem.attribute("id") == "auth")
    {
      if (AElem.attribute("type") == "result")
      {
        deleteLater();
        emit finished(false);
      }
      else if (AElem.attribute("type") == "error")
      {
        ErrorHandler err(AElem);
        emit error(err.message());
      }
    }
    else
    {
      emit error(tr("Wrong iq-auth response"));
    }
    return true;
  }
  return false;
}

bool IqAuth::xmppElementOut(IXmppStream *AXmppStream, QDomElement &AElem, int AOrder)
{
  Q_UNUSED(AXmppStream);
  Q_UNUSED(AElem);
  Q_UNUSED(AOrder);
  return false;
}

bool IqAuth::start(const QDomElement &AElem)
{
  if (AElem.tagName()=="auth")
  {
    Stanza auth("iq");
    auth.setType("set").setTo(FXmppStream->streamJid().domain()).setId("auth"); 
    QDomElement query = auth.addElement("query",NS_JABBER_IQ_AUTH);
    query.appendChild(auth.createElement("username")).appendChild(auth.createTextNode(FXmppStream->streamJid().eNode()));
    QByteArray shaData = FXmppStream->streamId().toUtf8()+FXmppStream->password().toUtf8(); 
    QByteArray shaDigest = QCryptographicHash::hash(shaData,QCryptographicHash::Sha1).toHex();
    query.appendChild(auth.createElement("digest")).appendChild(auth.createTextNode(shaDigest.toLower().trimmed()));
    query.appendChild(auth.createElement("resource")).appendChild(auth.createTextNode(FXmppStream->streamJid().resource()));
    FXmppStream->insertXmppElementHandler(this, XEHO_XMPP_FEATURE);
    FXmppStream->sendStanza(auth);
    return true;
  }
  deleteLater();
  return false;
}


//IqAuthPlugin
IqAuthPlugin::IqAuthPlugin()
{
  FXmppStreams = NULL;
}

IqAuthPlugin::~IqAuthPlugin()
{

}

void IqAuthPlugin::pluginInfo(IPluginInfo *APluginInfo)
{
  APluginInfo->name = tr("Query Authentication");
  APluginInfo->description = tr("Allow you to log on the Jabber server without support SASL authentication");
  APluginInfo->version = "1.0";
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->dependences.append(XMPPSTREAMS_UUID);  
}

bool IqAuthPlugin::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
  if (plugin)
    FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());

  return FXmppStreams!=NULL;
}

bool IqAuthPlugin::initObjects()
{
  if (FXmppStreams)
  {
    FXmppStreams->registerXmppFeature(this, NS_FEATURE_IQAUTH, XFO_IQAUTH);
  }
  return true;
}

IXmppFeature *IqAuthPlugin::newXmppFeature(const QString &AFeatureNS, IXmppStream *AXmppStream)
{
  if (AFeatureNS == NS_FEATURE_IQAUTH)
  {
    IXmppFeature *feature = new IqAuth(AXmppStream);
    connect(feature->instance(),SIGNAL(featureDestroyed()),SLOT(onFeatureDestroyed()));
    emit featureCreated(feature);
    return feature;
  }
  return NULL;
}

void IqAuthPlugin::onFeatureDestroyed()
{
  IXmppFeature *feature = qobject_cast<IXmppFeature *>(sender());
  if (feature)
    emit featureDestroyed(feature);
}

Q_EXPORT_PLUGIN2(plg_iqauth, IqAuthPlugin)
