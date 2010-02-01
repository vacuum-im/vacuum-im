#include "saslsession.h"

SASLSession::SASLSession(IXmppStream *AXmppStream) : QObject(AXmppStream->instance())
{
  FXmppStream = AXmppStream;
}

SASLSession::~SASLSession()
{
  FXmppStream->removeXmppElementHandler(this, XEHO_XMPP_FEATURE);
  emit featureDestroyed();
}

bool SASLSession::xmppElementIn(IXmppStream *AXmppStream, QDomElement &AElem, int AOrder)
{
  if (AXmppStream==FXmppStream && AOrder==XEHO_XMPP_FEATURE)
  {
    FXmppStream->removeXmppElementHandler(this, XEHO_XMPP_FEATURE);
    if (AElem.attribute("id") == "session")
    {
      if (AElem.attribute("type") == "result")
      {
        deleteLater();
        emit finished(false);
      }
      else
      {
        ErrorHandler err(AElem);
        emit error(err.message());
      }
    }
    else
    {
      emit error(tr("Wrong SASL session response"));
    }
    return true;
  }
  return false;
}

bool SASLSession::xmppElementOut(IXmppStream *AXmppStream, QDomElement &AElem, int AOrder)
{
  Q_UNUSED(AXmppStream);
  Q_UNUSED(AElem);
  Q_UNUSED(AOrder);
  return false;
}

bool SASLSession::start(const QDomElement &AElem)
{
  if (AElem.tagName() == "session")
  {
    Stanza session("iq");
    session.setType("set").setId("session"); 
    session.addElement("session",NS_FEATURE_SESSION);
    FXmppStream->insertXmppElementHandler(this, XEHO_XMPP_FEATURE);
    FXmppStream->sendStanza(session); 
    return true;
  }
  deleteLater();
  return false;
}
