#include <QtDebug>
#include "saslbind.h"

SASLBind::SASLBind(IXmppStream *AXmppStream) : QObject(AXmppStream->instance())
{
  FNeedHook = false;
  FXmppStream = AXmppStream;
  connect(FXmppStream->instance(),SIGNAL(closed(IXmppStream *)),SLOT(onStreamClosed(IXmppStream *)));
}

SASLBind::~SASLBind()
{

}

bool SASLBind::start(const QDomElement &/*AElem*/)
{
  FNeedHook = true;
  Stanza bind("iq");
  bind.setType("set").setId("bind"); 
  if (FXmppStream->jid().resource().isEmpty())
    bind.addElement("bind",NS_FEATURE_BIND);
  else
    bind.addElement("bind",NS_FEATURE_BIND)
      .appendChild(bind.createElement("resource"))
      .appendChild(bind.createTextNode(FXmppStream->jid().resource()));
  FXmppStream->sendStanza(bind); 
  return true;
}

bool SASLBind::needHook(Direction ADirection) const
{
  return ADirection == DirectionIn ? FNeedHook : false;
}

bool SASLBind::hookElement(QDomElement *AElem, Direction ADirection)
{
  if (ADirection == DirectionIn && AElem->attribute("id") == "bind")
  {
    FNeedHook = false;
    if (AElem->attribute("type") == "result")
    {
      QString jid = AElem->firstChild().firstChild().toElement().text();  
      if (jid.isEmpty())
      {
        Stanza errStanza = Stanza(*AElem).replyError("jid-malformed");
        FXmppStream->sendStanza(errStanza);
        ErrorHandler err(errStanza.element()); 
        emit error(err.message());
      }
      else 
      {
        FXmppStream->setJid(jid); 
        emit ready(false);
      }
    }
    else
    {
      ErrorHandler err(*AElem);
      emit error(err.message());
    }
    return true;
  }
  return false;
}

void SASLBind::onStreamClosed(IXmppStream * /*AXmppStream*/)
{
  FNeedHook = false;
}

