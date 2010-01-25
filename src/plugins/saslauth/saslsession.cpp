#include "saslsession.h"

SASLSession::SASLSession(IXmppStream *AXmppStream) : QObject(AXmppStream->instance())
{
  FNeedHook = false;
  FXmppStream = AXmppStream;
  connect(FXmppStream->instance(),SIGNAL(closed()),SLOT(onStreamClosed()));
}

SASLSession::~SASLSession()
{

}

bool SASLSession::start(const QDomElement &AElem)
{
  if (AElem.tagName() == "session")
  {
    FNeedHook = true;
    Stanza session("iq");
    session.setType("set").setId("session"); 
    session.addElement("session",NS_FEATURE_SESSION);
    FXmppStream->sendStanza(session); 
    return true;
  }
  return false;
}

bool SASLSession::needHook(Direction ADirection) const
{
  return ADirection == DirectionIn ? FNeedHook : false;
}

bool SASLSession::hookElement(QDomElement &AElem, Direction ADirection)
{
  if (ADirection == DirectionIn && AElem.attribute("id") == "session")
  {
    FNeedHook = false;
    if (AElem.attribute("type") == "result")
    {
      emit ready(false);
    }
    else
    {
      ErrorHandler err(AElem);
      emit error(err.message());
    }
    return true;
  }
  return false;
}

void SASLSession::onStreamClosed()
{
  FNeedHook = false;
}
