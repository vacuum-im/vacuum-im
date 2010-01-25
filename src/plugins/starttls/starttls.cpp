#include "starttls.h"

StartTLS::StartTLS(IXmppStream *AXmppStream) : QObject(AXmppStream->instance())
{
  FConnection = NULL;
  FNeedHook = false;
  FXmppStream = AXmppStream;
  connect(FXmppStream->instance(),SIGNAL(closed()), SLOT(onStreamClosed()));
}

StartTLS::~StartTLS()
{

}

bool StartTLS::start(const QDomElement &AElem)
{
  FConnection = qobject_cast<IDefaultConnection *>(FXmppStream->connection()->instance());
  if (FConnection && AElem.tagName()=="starttls")
  {
    FNeedHook = true;
    Stanza request("starttls");
    request.setAttribute("xmlns",NS_FEATURE_STARTTLS);
    FXmppStream->sendStanza(request);
    return true;
  }
  return false;
}

bool StartTLS::needHook(Direction ADirection) const
{
  return ADirection == DirectionIn ? FNeedHook : false;
}

bool StartTLS::hookElement(QDomElement &AElem, Direction ADirection)
{
  if (ADirection == DirectionIn && AElem.namespaceURI() == NS_FEATURE_STARTTLS)
  {
    FNeedHook = false;
    if (AElem.tagName() == "proceed")
    {
      connect(FConnection->instance(),SIGNAL(encrypted()),SLOT(onConnectionEncrypted()));
      FConnection->startClientEncryption();
    }
    else if (AElem.tagName() == "failure")
      emit error(tr("StartTLS negotiation failed.")); 
    else
      emit error(tr("Wrong StartTLS negotiation answer.")); 

    return true;
  }
  return false;
}

void StartTLS::onConnectionEncrypted()
{
  emit ready(true);
}

void StartTLS::onStreamClosed()
{
  if (FConnection)
  {
    FConnection->instance()->disconnect(this);
    FConnection = NULL;
  }
  FNeedHook = false;
}

