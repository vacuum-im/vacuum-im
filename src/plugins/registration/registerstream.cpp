#include "registerstream.h"

RegisterStream::RegisterStream(IXmppStream *AXmppStream) : QObject(AXmppStream->instance())
{
  FXmppStream = AXmppStream;
}

RegisterStream::~RegisterStream()
{
  FXmppStream->removeXmppElementHandler(this, XEHO_XMPP_FEATURE);
  emit featureDestroyed();
}

bool RegisterStream::xmppElementIn(IXmppStream *AXmppStream, QDomElement &AElem, int AOrder)
{
  if (AXmppStream==FXmppStream && AOrder==XEHO_XMPP_FEATURE)
  {
    if (AElem.attribute("id") == "getReg")
    {
      if (AElem.attribute("type") == "result")
      {
        Stanza submit("iq");
        submit.setType("set").setId("setReg");
        QDomElement query = submit.addElement("query",NS_JABBER_REGISTER);
        query.appendChild(submit.createElement("username")).appendChild(submit.createTextNode(FXmppStream->streamJid().eNode()));
        query.appendChild(submit.createElement("password")).appendChild(submit.createTextNode(FXmppStream->password()));
        query.appendChild(submit.createElement("key")).appendChild(submit.createTextNode(AElem.firstChildElement("query").attribute("key")));
        FXmppStream->sendStanza(submit);
      }
      else if (AElem.attribute("type") == "error")
      {
        ErrorHandler err(AElem);
        emit error(err.message());
      }
    }
    else if (AElem.attribute("id") == "setReg")
    {
      FXmppStream->removeXmppElementHandler(this, XEHO_XMPP_FEATURE);
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
      emit error(tr("Wrong registration response"));
    }
    return true;
  }
  return false;
}

bool RegisterStream::xmppElementOut(IXmppStream *AXmppStream, QDomElement &AElem, int AOrder)
{
  Q_UNUSED(AXmppStream);
  Q_UNUSED(AElem);
  Q_UNUSED(AOrder);
  return false;
}

bool RegisterStream::start(const QDomElement &AElem)
{
  if (AElem.tagName()=="register" && !FXmppStream->password().isEmpty())
  {
    Stanza reg("iq");
    reg.setType("get").setId("getReg");
    reg.addElement("query",NS_JABBER_REGISTER);
    FXmppStream->insertXmppElementHandler(this,XEHO_XMPP_FEATURE);
    FXmppStream->sendStanza(reg);
    return true;
  }
  deleteLater();
  return false;
}
