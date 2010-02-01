#include "saslplugin.h"

SASLPlugin::SASLPlugin()
{
  FXmppStreams = NULL;
}

SASLPlugin::~SASLPlugin()
{

}

void SASLPlugin::pluginInfo(IPluginInfo *APluginInfo)
{
  APluginInfo->name = tr("SASL Authentication");
  APluginInfo->description = tr("Allows to log in to Jabber server using SASL authentication");
  APluginInfo->version = "1.0";
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->dependences.append(XMPPSTREAMS_UUID);
}

bool SASLPlugin::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
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
    FXmppStreams->registerXmppFeature(this,NS_FEATURE_SASL,XFO_SASL);
    FXmppStreams->registerXmppFeature(this,NS_FEATURE_BIND,XFO_BIND);
    FXmppStreams->registerXmppFeature(this,NS_FEATURE_SESSION,XFO_SESSION);
  }
  return true;
}

QList<QString> SASLPlugin::xmppFeatures() const
{
  return QList<QString>() << NS_FEATURE_SASL << NS_FEATURE_BIND << NS_FEATURE_SESSION;
}

IXmppFeature *SASLPlugin::newXmppFeature(const QString &AFeatureNS, IXmppStream *AXmppStream)
{
  if (AFeatureNS == NS_FEATURE_SASL)
  {
    IXmppFeature *feature = new SASLAuth(AXmppStream);
    connect(feature->instance(),SIGNAL(featureDestroyed()),SLOT(onFeatureDestroyed()));
    emit featureCreated(feature);
    return feature;
  }
  else if (AFeatureNS == NS_FEATURE_BIND)
  {
    IXmppFeature *feature = new SASLBind(AXmppStream);
    connect(feature->instance(),SIGNAL(featureDestroyed()),SLOT(onFeatureDestroyed()));
    emit featureCreated(feature);
    return feature;
  }
  else if (AFeatureNS == NS_FEATURE_SESSION)
  {
    IXmppFeature *feature = new SASLSession(AXmppStream);
    connect(feature->instance(),SIGNAL(featureDestroyed()),SLOT(onFeatureDestroyed()));
    emit featureCreated(feature);
    return feature;
  }
  return NULL;
}

void SASLPlugin::onFeatureDestroyed()
{
  IXmppFeature *feature = qobject_cast<IXmppFeature *>(sender());
  if (feature)
    emit featureDestroyed(feature);
}

Q_EXPORT_PLUGIN2(plg_sasl, SASLPlugin)
