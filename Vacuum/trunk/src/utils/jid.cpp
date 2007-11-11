#include "jid.h"

QList<QChar> Jid::escChars = QList<QChar>()       << 0x20 << 0x22 << 0x26 << 0x27 << 0x2f << 0x3a << 0x3c << 0x3e << 0x40 << 0x5c;
QList<QString> Jid::escStrings = QList<QString>() <<"\\20"<<"\\22"<<"\\26"<<"\\27"<<"\\2f"<<"\\3a"<<"\\3c"<<"\\3e"<<"\\40"<<"\\5c";

Jid::Jid(const char *AJidStr)
{
  d = new JidData;
  parseString(AJidStr);
}

Jid::Jid(const QString &AJidStr)
{
  d = new JidData;
  parseString(AJidStr);
}

Jid::Jid(const QString &ANode, const QString &ADomane, const QString &AResource) 
{
  d = new JidData;
  setNode(ANode);
  setDomane(ADomane);
  setResource(AResource);
}

Jid::~Jid()
{

}

bool Jid::isValid() const
{
  if (d->FDomane.isEmpty() || d->FDomane.size() > 1023)
    return false;

  if (d->FNode.size() > 1023)
    return false;

  if (d->FResource.size() > 1023)
    return false;

  return true;
}

bool Jid::isEmpty() const
{
  return d->FNode.isEmpty() && d->FDomane.isEmpty() && d->FResource.isEmpty();
}

void Jid::setNode(const QString &ANode)
{
  d->FNode = unescape106(ANode);
  d->FEscNode = escape106(d->FNode);
  d->FPrepNode = d->FEscNode.toLower();
}

void Jid::setDomane(const QString &ADomane)
{
  d->FDomane = ADomane;
  d->FPrepDomane = ADomane.toLower();
}

void Jid::setResource(const QString &AResource)
{
  d->FResource = AResource;
  d->FPrepResource = AResource;
}

Jid Jid::prepared() const
{
  return Jid(d->FPrepNode,d->FPrepDomane,d->FPrepResource);  
}

Jid& Jid::operator=(const QString &AJidStr)
{
  return parseString(AJidStr);
}

bool Jid::operator==(const Jid &AJid) const
{
  return equals(AJid,true);
}

bool Jid::operator==(const QString &AJidStr) const
{
  return equals(Jid(AJidStr),true);
}

bool Jid::operator!=(const Jid &AJid) const
{
  return !equals(AJid,true);
}

bool Jid::operator!=(const QString &AJidStr) const
{
  return !equals(Jid(AJidStr),true);
}

bool Jid::operator&&(const Jid &AJid) const
{
  return equals(AJid,false);
}

bool Jid::operator&&(const QString &AJidStr) const
{
  return equals(Jid(AJidStr),false);
}

bool Jid::operator<(const Jid &AJid) const
{
  return d->FPrepNode < AJid.pNode() || d->FPrepDomane < AJid.pDomane() || d->FResource < AJid.pResource();
}

bool Jid::operator>(const Jid &AJid) const
{
  return d->FPrepNode > AJid.pNode() || d->FPrepDomane > AJid.pDomane() || d->FResource > AJid.pResource();
}

QString Jid::encode(const QString &AJidStr)
{
  QString encJid;
  encJid.reserve(AJidStr.length()*3);

  for(int i = 0; i < AJidStr.length(); i++) 
  {
    if(AJidStr.at(i) == '@') 
    {
      encJid.append("_at_");
    }
    else if(AJidStr.at(i) == '.') 
    {
      encJid.append('.');
    }
    else if(!AJidStr.at(i).isLetterOrNumber()) 
    {
      QString hex;
      hex.sprintf("%%%02X", AJidStr.at(i).toLatin1());
      encJid.append(hex);
    }
    else 
    {
      encJid.append(AJidStr.at(i));
    }
  }

  encJid.squeeze();
  return encJid;
}

QString Jid::decode(const QString &AEncJid)
{
  QString jidStr;
  jidStr.reserve(AEncJid.length());

  for(int i = 0; i < AEncJid.length(); i++)
  {
    if(AEncJid.at(i) == '%' && (AEncJid.length()-i-1) >= 2)
    {
      jidStr.append(char(AEncJid.mid(i+1,2).toInt(NULL,16)));
      i += 2;
    }
    else 
    {
      jidStr.append(AEncJid.at(i));
    }
  }

  for(int i = jidStr.length(); i >= 3; i--) 
  {
    if(jidStr.mid(i, 4) == "_at_") 
    {
      jidStr.replace(i, 4, "@");
      break;
    }
  }

  jidStr.squeeze();
  return jidStr;
}

QString Jid::encode822(const QString &AJidStr)
{
  QString encJid;
  encJid.reserve(AJidStr.length()*4);

  for(int i = 0; i < AJidStr.length(); i++) 
  {
    if(AJidStr.at(i) == '\\' || AJidStr.at(i) == '<' || AJidStr.at(i) == '>') 
    {
      QString hex;
      hex.sprintf("\\x%02X", AJidStr.at(i).toLatin1());
      encJid.append(hex);
    }
    else
    {
      encJid.append(AJidStr.at(i));
    }
  }

  encJid.squeeze();
  return encJid;
}

QString Jid::decode822(const QString &AEncJid)
{
  QString jidStr;
  jidStr.reserve(AEncJid.length());

  for(int i = 0; i < AEncJid.length(); i++) 
  {
    if(AEncJid.at(i) == '\\' && i+3 < AEncJid.length() && AEncJid.at(i+1) == 'x')
    {
      jidStr.append(char(AEncJid.mid(i+2,2).toInt(NULL,16)));
      i += 3;
    }
    else
      jidStr.append(AEncJid.at(i));
  }

  jidStr.squeeze();
  return jidStr;
}

QString Jid::escape106(const QString &ANode)
{
  QString escNode;
  escNode.reserve(ANode.length()*3);

  for (int i = 0; i<ANode.length(); i++)
  {
    int index = escChars.indexOf(ANode.at(i));
    if (index >= 0)
      escNode.append(escStrings.at(index));
    else
      escNode.append(ANode.at(i));
  }

  escNode.squeeze();
  return escNode;
}

QString Jid::unescape106(const QString &AEscNode)
{
  QString nodeStr;
  nodeStr.reserve(AEscNode.length());

  int index;
  for (int i = 0; i<AEscNode.length(); i++)
  {
    if (AEscNode.at(i) == '\\' && (index = escStrings.indexOf(AEscNode.mid(i,3))) >= 0)
    {
      nodeStr.append(escChars.at(index));
      i+=2;
    }
    else
      nodeStr.append(AEscNode.at(i));
  }

  nodeStr.squeeze();
  return nodeStr;
}

Jid &Jid::parseString(const QString &AJidStr)
{	
  if (!AJidStr.isEmpty())
  {
    int at = AJidStr.lastIndexOf("@");
    int slash = AJidStr.indexOf("/", at+1);
    if (slash == -1)
      slash = AJidStr.size();
    setNode(at > 0 ? AJidStr.left(at) : "");
    setDomane(slash-at-1 > 0 ? AJidStr.mid(at+1,slash-at-1) : "");
    setResource(slash < AJidStr.size()-1 ? AJidStr.right(AJidStr.size()-1 - slash) : "");
  }
  else
  {
    setNode("");
    setDomane("");
    setResource("");
  }
  return *this;
}

QString Jid::toString(bool AEscaped, bool APrepared, bool AFull) const
{
  QString result;
  QString node =  AEscaped ? d->FEscNode : (APrepared ? d->FPrepNode : d->FNode);
  QString domane = APrepared ? d->FPrepDomane : d->FDomane;
  QString resource = APrepared ? d->FPrepResource : d->FResource;

  if (!node.isEmpty())
    result = node + "@";

  if (!domane.isEmpty())
    result += domane;

  if (AFull && !resource.isEmpty())
    result += "/" + resource;

  return result;
}

bool Jid::equals(const Jid &AJid, bool AFull) const
{
  return (d->FPrepNode == AJid.pNode()) && (d->FPrepDomane == AJid.pDomane()) && (!AFull || d->FPrepResource == AJid.pResource()); 
}

uint qHash(const Jid &key)
{
  return qHash(key.pFull()); 
}
