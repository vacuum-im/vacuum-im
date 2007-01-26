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

Jid::Jid(const QString &AJidStr, QObject *parent)
  : QObject(parent)
{
  if (!AJidStr.isEmpty()) 
    setJid(AJidStr);
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

Jid& Jid::setJid(const QString &AJidStr)
{	
  int at = AJidStr.indexOf("@");

  int slash;
  if ((slash = AJidStr.indexOf("/", at+1)) == -1)
    slash = AJidStr.size();

  if (at>0)
    FNode = AJidStr.left(at);
  else 
    FNode.clear(); 

  if (slash < AJidStr.size()-1)
    FResource = AJidStr.right(AJidStr.size()-1 - slash);
  else
    FResource.clear();  

  if (slash-at-1>0)
    FDomane = AJidStr.mid(at+1,slash-at-1);
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

uint qHash(const Jid &key)
{
  return qHash(key.pFull()); 
}
