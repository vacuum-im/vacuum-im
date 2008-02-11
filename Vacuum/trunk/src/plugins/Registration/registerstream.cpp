#include "registerstream.h"

RegisterStream::RegisterStream(IStreamFeaturePlugin *AFeaturePlugin, IXmppStream *AXmppStream) : QObject(AXmppStream->instance())
{
  FFeaturePlugin = AFeaturePlugin;
  FXmppStream = AXmppStream;
  FNeedHook = false;
  FRegisterFinished = false;
}

RegisterStream::~RegisterStream()
{

}

bool RegisterStream::start(const QDomElement &/*AElem*/)
{
  if (!FRegisterFinished && !FXmppStream->jid().node().isEmpty() && !FXmppStream->password().isEmpty())
  {
    Stanza reg("iq");
    reg.setType("get").setId("getReg");
    reg.addElement("query",NS_JABBER_REGISTER);
    if (FXmppStream->sendStanza(reg)>0)
    {
      FNeedHook = true;
      return true;
    }
  }
  return false;
}

bool RegisterStream::needHook(Direction ADirection) const
{
  return ADirection==IStreamFeature::DirectionIn ? FNeedHook : false;
}

bool RegisterStream::hookElement(QDomElement *AElem, Direction ADirection)
{
  if (ADirection == IStreamFeature::DirectionIn)
  {
    if (AElem->attribute("id") == "getReg")
    {
      if (AElem->attribute("type") == "result")
      {
        Stanza submit("iq");
        submit.setType("set").setId("setReg");
        QDomElement query = submit.addElement("query",NS_JABBER_REGISTER);
        query.appendChild(submit.createElement("username")).appendChild(submit.createTextNode(FXmppStream->jid().eNode()));
        query.appendChild(submit.createElement("password")).appendChild(submit.createTextNode(FXmppStream->password()));
        query.appendChild(submit.createElement("key")).appendChild(submit.createTextNode(AElem->firstChildElement("query").attribute("key")));
        FXmppStream->sendStanza(submit);
        return true;
      }
      else if (AElem->attribute("type") == "error")
      {
        FNeedHook = false;
        ErrorHandler err(*AElem);
        emit error(err.message());
      }
    }
    else if (AElem->attribute("id") == "setReg")
    {
      FNeedHook = false;
      FRegisterFinished = true;
      if (AElem->attribute("type") == "result")
      {
        emit ready(false);
      }
      else if (AElem->attribute("type") == "error")
      {
        ErrorHandler err(*AElem);
        emit error(err.message());
      }
    }
    FFeaturePlugin->destroyStreamFeature(this);
    return true;
  }
  return false;
}

