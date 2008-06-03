#include "iqauth.h"

#include <QtDebug>
#include <QCryptographicHash>

IqAuth::IqAuth(IXmppStream *AXmppStream) : QObject(AXmppStream->instance())
{
  FNeedHook = false;
  FXmppStream = AXmppStream;
  connect(FXmppStream->instance(),SIGNAL(closed(IXmppStream *)), SLOT(onStreamClosed(IXmppStream *)));
}

IqAuth::~IqAuth()
{

}

bool IqAuth::start(const QDomElement &/*AElem*/)
{
  Stanza auth("iq");
  auth.setType("set").setTo(FXmppStream->jid().domain()).setId("auth"); 
  QDomElement query = auth.addElement("query",NS_JABBER_IQ_AUTH);
  query.appendChild(auth.createElement("username")).appendChild(auth.createTextNode(FXmppStream->jid().eNode()));
  QByteArray shaData = FXmppStream->streamId().toUtf8()+FXmppStream->password().toUtf8(); 
  QByteArray shaDigest = QCryptographicHash::hash(shaData,QCryptographicHash::Sha1).toHex();
  query.appendChild(auth.createElement("digest")).appendChild(auth.createTextNode(shaDigest.toLower().trimmed()));
  query.appendChild(auth.createElement("resource")).appendChild(auth.createTextNode(FXmppStream->jid().resource()));
  
  FNeedHook = true;
  FXmppStream->sendStanza(auth);
  return true;
}

bool IqAuth::needHook(Direction ADirection) const
{
  return ADirection == DirectionIn ? FNeedHook : false;
}

bool IqAuth::hookElement(QDomElement *AElem, Direction ADirection)
{
  if (ADirection == DirectionIn && AElem->attribute("id") == "auth")
  {
    FNeedHook = false;
    if (AElem->attribute("type") == "result")
    {
      emit ready(false);
    }
    else if (AElem->attribute("type") == "error")
    {
      ErrorHandler err(*AElem);
      emit error(err.message());
    }
    return true;
  }
  return false;
}

void IqAuth::onStreamClosed(IXmppStream * /*AXmppStream*/)
{
  FNeedHook = false;
}


//IqAuthPlugin
IqAuthPlugin::IqAuthPlugin()
{
  FXmppStreams = NULL;
}

IqAuthPlugin::~IqAuthPlugin()
{

}

void IqAuthPlugin::pluginInfo(PluginInfo *APluginInfo)
{
  APluginInfo->author = tr("Potapov S.A. aka Lion");
  APluginInfo->description = tr("Implementation of Non-SASL Authentication (JEP-0078)");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = "Non-SASL Authentication";
  APluginInfo->uid = IQAUTH_UUID;
  APluginInfo->version = "0.1";
  APluginInfo->dependences.append(XMPPSTREAMS_UUID);  
}

bool IqAuthPlugin::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *plugin = APluginManager->getPlugins("IXmppStreams").value(0,NULL);
  if (plugin)
    FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());

  return FXmppStreams!=NULL;
}

bool IqAuthPlugin::initObjects()
{
  if (FXmppStreams)
  {
    FXmppStreams->registerFeature(NS_FEATURE_IQAUTH,this);
  }
  return true;
}

IStreamFeature *IqAuthPlugin::getStreamFeature(const QString &AFeatureNS, IXmppStream *AXmppStream)
{
  if (AFeatureNS == NS_FEATURE_IQAUTH)
  {
    IStreamFeature *feature = FFeatures.value(AXmppStream);
    if (!feature)
    {
      feature = new IqAuth(AXmppStream);
      FFeatures.insert(AXmppStream,feature);
      emit featureCreated(feature);
    }
    return feature;
  }
  return NULL;
}

void IqAuthPlugin::destroyStreamFeature(IStreamFeature *AFeature)
{
  if (FFeatures.value(AFeature->xmppStream()) == AFeature)
  {
    FFeatures.remove(AFeature->xmppStream());
    AFeature->xmppStream()->removeFeature(AFeature);
    emit featureDestroyed(AFeature);
    AFeature->instance()->deleteLater();
  }
}

Q_EXPORT_PLUGIN2(IqAuthPlugin, IqAuthPlugin)
