#include "saslbind.h"

SASLBind::SASLBind(IXmppStream *AXmppStream) : QObject(AXmppStream->instance())
{
  FXmppStream = AXmppStream;
}

SASLBind::~SASLBind()
{
  FXmppStream->removeXmppElementHandler(this, XEHO_XMPP_FEATURE);
  emit featureDestroyed();
}

bool SASLBind::xmppElementIn(IXmppStream *AXmppStream, QDomElement &AElem, int AOrder)
{
  if (AXmppStream==FXmppStream && AOrder==XEHO_XMPP_FEATURE)
  {
    FXmppStream->removeXmppElementHandler(this, XEHO_XMPP_FEATURE);
    if (AElem.attribute("id") == "bind")
    {
      if (AElem.attribute("type") == "result")
      {
        Jid streamJid = AElem.firstChild().firstChild().toElement().text();  
        if (streamJid.isValid())
        {
          deleteLater();
          FXmppStream->setStreamJid(streamJid); 
          emit finished(false);
        }
        else 
        {
          emit error(tr("Invalid XMPP stream JID in SASL bind response"));
        }
      }
      else
      {
        ErrorHandler err(AElem);
        emit error(err.message());
      }
    }
    else
    {
      emit error(tr("Wrong SASL bind response"));
    }
    return true;
  }
  return false;
}

bool SASLBind::xmppElementOut(IXmppStream *AXmppStream, QDomElement &AElem, int AOrder)
{
  Q_UNUSED(AXmppStream);
  Q_UNUSED(AElem);
  Q_UNUSED(AOrder);
  return false;
}

bool SASLBind::start(const QDomElement &AElem)
{
  if (AElem.tagName() == "bind")
  {
    Stanza bind("iq");
    bind.setType("set").setId("bind"); 
    bind.addElement("bind",NS_FEATURE_BIND);
    if (!FXmppStream->streamJid().resource().isEmpty())
      bind.firstElement("bind",NS_FEATURE_BIND).appendChild(bind.createElement("resource")).appendChild(bind.createTextNode(FXmppStream->streamJid().resource()));
    FXmppStream->insertXmppElementHandler(this, XEHO_XMPP_FEATURE);
    FXmppStream->sendStanza(bind); 
    return true;
  }
  deleteLater();
  return false;
}
