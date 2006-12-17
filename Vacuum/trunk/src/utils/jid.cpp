#include "jid.h"
#include <QHash>

Jid::Jid(const QString &ANode, const QString &ADomane, 
         const QString &AResource, QObject *parent) 
  : QObject(parent)
{
  setNode(ANode);
  setDomane(ADomane);
  setResource(AResource);
}

Jid::Jid(const QString &jidStr, QObject *parent)
  : QObject(parent)
{
  if (!jidStr.isEmpty()) 
    setJid(jidStr);
}

Jid::Jid(const char *ch, QObject *parent)
  : QObject(parent)
{
  QString jidStr(ch);
  if (!jidStr.isEmpty()) 
    setJid(jidStr);
}


Jid::Jid(const Jid &AJid)
  : QObject(AJid.parent())
{
  operator=(AJid);
}

Jid::~Jid()
{

}

bool Jid::isValid() const
{
  if (FDomane.isEmpty() || FDomane.size() > 1023)
    return false;

  if (FNode.size() > 1023)
    return false;

  if (FResource.size() > 1023)
    return false;

  return true;
}

Jid& Jid::setJid(const QString &jidStr)
{	
  int at = jidStr.indexOf("@");

  int slash;
  if ((slash = jidStr.indexOf("/", at+1)) == -1)
    slash = jidStr.size();

  if (at>0)
    FNode = jidStr.left(at);
  else 
    FNode.clear(); 

  if (slash < jidStr.size()-1)
    FResource = jidStr.right(jidStr.size()-1 - slash);
  else
    FResource.clear();  

  if (slash-at-1>0)
    FDomane = jidStr.mid(at+1,slash-at-1);
  else
    FDomane.clear();

  return *this;
}

QString Jid::toString(bool withRes) const
{
  QString result;
  if (!FNode.isEmpty())
    result = FNode + "@";

  if (!FDomane.isEmpty())
    result += FDomane;

  if (withRes && !FResource.isEmpty())
    result += "/" + FResource;

  return result;
}

Jid Jid::prep() const
{
  return Jid(node().toLower(),domane().toLower(),resource());  
}

bool Jid::equals(const Jid &AJid, bool withRes) const
{
  return (FNode.toLower() == AJid.node().toLower()) &&
    (FDomane.toLower() == AJid.domane().toLower()) &&
    (!withRes || FResource == AJid.resource()); 
}

Jid &Jid::operator =(const Jid &AJid)
{
  return setJid(AJid.full());
}

bool Jid::operator ==(const QString &jidStr) const
{
  return equals(Jid(jidStr));
}

bool Jid::operator !=(const QString &jidStr) const
{
  return !equals(Jid(jidStr));
}

uint qHash(const Jid &key)
{
  return qHash(key.prep().full()); 
}
