#include "starttls.h"

StartTLS::StartTLS(IXmppStream *AXmppStream) : QObject(AXmppStream->instance())
{
  FConnection = NULL;
  FXmppStream = AXmppStream;
}

StartTLS::~StartTLS()
{
  FXmppStream->removeXmppElementHandler(this,XEHO_XMPP_FEATURE);
  emit featureDestroyed();
}

bool StartTLS::xmppElementIn(IXmppStream *AXmppStream, QDomElement &AElem, int AOrder)
{
  if (AXmppStream==FXmppStream && AOrder==XEHO_XMPP_FEATURE)
  {
    FXmppStream->removeXmppElementHandler(this,XEHO_XMPP_FEATURE);
    if (AElem.tagName() == "proceed")
    {
      connect(FConnection->instance(),SIGNAL(encrypted()),SLOT(onConnectionEncrypted()));
      FConnection->startClientEncryption();
    }
    else if (AElem.tagName() == "failure")
    {
      emit error(tr("StartTLS negotiation failed")); 
    }
    else
    {
      emit error(tr("Wrong StartTLS negotiation response"));
    }
    return true;
  }
  return false;
}

bool StartTLS::xmppElementOut(IXmppStream *AXmppStream, QDomElement &AElem, int AOrder)
{
  Q_UNUSED(AXmppStream);
  Q_UNUSED(AElem);
  Q_UNUSED(AOrder);
  return false;
}

bool StartTLS::start(const QDomElement &AElem)
{
  FConnection = qobject_cast<IDefaultConnection *>(FXmppStream->connection()->instance());
  if (FConnection && AElem.tagName()=="starttls")
  {
    Stanza request("starttls");
    request.setAttribute("xmlns",NS_FEATURE_STARTTLS);
    FXmppStream->insertXmppElementHandler(this,XEHO_XMPP_FEATURE);
    FXmppStream->sendStanza(request);
    return true;
  }
  deleteLater();
  return false;
}

void StartTLS::onConnectionEncrypted()
{
  deleteLater();
  emit finished(true);
}
