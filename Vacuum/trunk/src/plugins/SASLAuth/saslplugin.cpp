#include "saslplugin.h"

SASLPlugin::SASLPlugin()
{
  FXmppStreams = NULL;
}

SASLPlugin::~SASLPlugin()
{
  QList<IStreamFeature *> features = FAuthFeatures.values();
  foreach(IStreamFeature *feature, features)
    destroyStreamFeature(feature);

  features = FBindFeatures.values();
  foreach(IStreamFeature *feature, features)
    destroyStreamFeature(feature);

  features = FSessionFeatures.values();
  foreach(IStreamFeature *feature, features)
    destroyStreamFeature(feature);
}

void SASLPlugin::pluginInfo(PluginInfo *APluginInfo)
{
  APluginInfo->author = tr("Potapov S.A. aka Lion");
  APluginInfo->description = tr("Implementation of SASL Authentication");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = "SASL Authentication";
  APluginInfo->uid = SASLAUTH_UUID;
  APluginInfo->version = "0.1";
  APluginInfo->dependences.append(XMPPSTREAMS_UUID);
}

bool SASLPlugin::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *plugin = APluginManager->getPlugins("IXmppStreams").value(0,NULL);
  if (plugin)
    FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());

  return FXmppStreams!=NULL;
}

bool SASLPlugin::initObjects()
{
  ErrorHandler::addErrorItem("aborted", ErrorHandler::CANCEL, 
    ErrorHandler::FORBIDDEN, tr("Authorization Aborted"),NS_FEATURE_SASL);

  ErrorHandler::addErrorItem("incorrect-encoding", ErrorHandler::CANCEL, 
    ErrorHandler::NOT_ACCEPTABLE, tr("Incorrect Encoding"),NS_FEATURE_SASL);

  ErrorHandler::addErrorItem("invalid-authzid", ErrorHandler::CANCEL, 
    ErrorHandler::FORBIDDEN, tr("Invalid Authzid"),NS_FEATURE_SASL);

  ErrorHandler::addErrorItem("invalid-mechanism", ErrorHandler::CANCEL, 
    ErrorHandler::NOT_ACCEPTABLE, tr("Invalid Mechanism"),NS_FEATURE_SASL);

  ErrorHandler::addErrorItem("mechanism-too-weak", ErrorHandler::CANCEL, 
    ErrorHandler::NOT_ACCEPTABLE, tr("Mechanism Too Weak"),NS_FEATURE_SASL);

  ErrorHandler::addErrorItem("not-authorized", ErrorHandler::CANCEL, 
    ErrorHandler::NOT_AUTHORIZED, tr("Not Authorized"),NS_FEATURE_SASL);

  ErrorHandler::addErrorItem("temporary-auth-failure", ErrorHandler::CANCEL,
    ErrorHandler::NOT_AUTHORIZED, tr("Temporary Auth Failure"),NS_FEATURE_SASL);

  if (FXmppStreams)
  {
    FXmppStreams->registerFeature(NS_FEATURE_SASL,this);
    FXmppStreams->registerFeature(NS_FEATURE_BIND,this);
    FXmppStreams->registerFeature(NS_FEATURE_SESSION,this);
  }
  return true;
}

QList<QString> SASLPlugin::streamFeatures() const
{
  return QList<QString>() << NS_FEATURE_SASL << NS_FEATURE_BIND << NS_FEATURE_SESSION;
}

IStreamFeature *SASLPlugin::getStreamFeature(const QString &AFeatureNS, IXmppStream *AXmppStream)
{
  if (AFeatureNS == NS_FEATURE_SASL)
  {
    IStreamFeature *feature = FAuthFeatures.value(AXmppStream);
    if (!feature)
    {
      feature = new SASLAuth(AXmppStream);
      FAuthFeatures.insert(AXmppStream,feature);
      emit featureCreated(feature);
    }
    return feature;
  }
  else if (AFeatureNS == NS_FEATURE_BIND)
  {
    IStreamFeature *feature = FBindFeatures.value(AXmppStream);
    if (!feature)
    {
      feature = new SASLBind(AXmppStream);
      FBindFeatures.insert(AXmppStream,feature);
      emit featureCreated(feature);
    }
    return feature;
  }
  else if (AFeatureNS == NS_FEATURE_SESSION)
  {
    IStreamFeature *feature = FSessionFeatures.value(AXmppStream);
    if (!feature)
    {
      feature = new SASLSession(AXmppStream);
      FSessionFeatures.insert(AXmppStream,feature);
      emit featureCreated(feature);
    }
    return feature;
  }
  return NULL;
}

void SASLPlugin::destroyStreamFeature(IStreamFeature *AFeature)
{
  if (AFeature)
  {
    if (FAuthFeatures.value(AFeature->xmppStream()) == AFeature)
    {
      FAuthFeatures.remove(AFeature->xmppStream());
      AFeature->xmppStream()->removeFeature(AFeature);
      emit featureDestroyed(AFeature);
      AFeature->instance()->deleteLater();
    }
    else if (FBindFeatures.value(AFeature->xmppStream()) == AFeature)
    {
      FBindFeatures.remove(AFeature->xmppStream());
      AFeature->xmppStream()->removeFeature(AFeature);
      emit featureDestroyed(AFeature);
      AFeature->instance()->deleteLater();
    }
    else if (FSessionFeatures.value(AFeature->xmppStream()) == AFeature)
    {
      FSessionFeatures.remove(AFeature->xmppStream());
      AFeature->xmppStream()->removeFeature(AFeature);
      emit featureDestroyed(AFeature);
      AFeature->instance()->deleteLater();
    }
  }
}

Q_EXPORT_PLUGIN2(SASLPlugin, SASLPlugin)
