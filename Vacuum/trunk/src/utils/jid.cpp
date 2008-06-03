#include "jid.h"

QList<QChar> Jid::escChars = QList<QChar>()       << 0x20 << 0x22 << 0x26 << 0x27 << 0x2f << 0x3a << 0x3c << 0x3e << 0x40; // << 0x5c;
QList<QString> Jid::escStrings = QList<QString>() <<"\\20"<<"\\22"<<"\\26"<<"\\27"<<"\\2f"<<"\\3a"<<"\\3c"<<"\\3e"<<"\\40"; //<<"\\5c";
QHash<QString,Jid> Jid::FCache = QHash<QString,Jid>();

Jid::Jid(const char *AJidStr)
{
  d = NULL;
  parseString(AJidStr);
}

Jid::Jid(const QString &AJidStr)
{
  d = NULL;
  parseString(AJidStr);
}

Jid::Jid(const QString &ANode, const QString &ADomane, const QString &AResource) 
{
  d = new JidData;
  setNode(ANode);
  setDomain(ADomane);
  setResource(AResource);
}

Jid::~Jid()
{

}

bool Jid::isValid() const
{
  return d->FDomainValid && d->FNodeValid && d->FResourceValid;
}

bool Jid::isEmpty() const
{
  return d->FNode.isEmpty() && d->FDomain.isEmpty() && d->FResource.isEmpty();
}

void Jid::setNode(const QString &ANode)
{
  d->FNode = unescape106(ANode);
  d->FEscNode = escape106(d->FNode);
  d->FPrepNode = nodePrepare(d->FEscNode);

  if (!d->FEscNode.isEmpty() && d->FPrepNode.isEmpty())
  {
    d->FNodeValid = false;
    d->FPrepNode = d->FEscNode;
  }
  else
    d->FNodeValid = true;
}

void Jid::setDomain(const QString &ADomain)
{
  d->FDomain = ADomain;
  d->FPrepDomain = domainPrepare(ADomain);

  if (d->FPrepDomain.isEmpty())
  {
    d->FDomainValid = false;
    d->FPrepDomain = d->FDomain;
  }
  else
    d->FDomainValid = true;
}

void Jid::setResource(const QString &AResource)
{
  d->FResource = AResource;
  d->FPrepResource = resourcePrepare(AResource);

  if (!d->FResource.isEmpty() && d->FPrepResource.isEmpty())
  {
    d->FResourceValid = false;
    d->FPrepResource = d->FResource;
  }
  else
    d->FResourceValid = true;
}

Jid Jid::prepared() const
{
  return Jid(d->FPrepNode,d->FPrepDomain,d->FPrepResource);  
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
  return d->FPrepNode < AJid.pNode() || d->FPrepDomain < AJid.pDomain() || d->FResource < AJid.pResource();
}

bool Jid::operator>(const Jid &AJid) const
{
  return d->FPrepNode > AJid.pNode() || d->FPrepDomain > AJid.pDomain() || d->FResource > AJid.pResource();
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

QString Jid::stringPrepare(const Stringprep_profile *AProfile, const QString &AString)
{
  QByteArray buffer = AString.toUtf8();
  if (!buffer.isEmpty() && buffer.size()<1024)
  {
    buffer.reserve(1024);
    if (stringprep(buffer.data(),buffer.capacity(),(Stringprep_profile_flags)0, AProfile) == STRINGPREP_OK)
      return QString::fromUtf8(buffer.constData());
  }
  return QString("");
}

QString Jid::nodePrepare(const QString &ANode)
{
  return stringPrepare(stringprep_xmpp_nodeprep, ANode);
}

QString Jid::domainPrepare(const QString &ADomain)
{
  return stringPrepare(stringprep_nameprep, ADomain);
}

QString Jid::resourcePrepare(const QString &AResource)
{
  return stringPrepare(stringprep_xmpp_resourceprep, AResource);
}

Jid &Jid::parseString(const QString &AJidStr)
{	
  if (!FCache.contains(AJidStr))
  {
    if (!d)
      d = new JidData;
    if (!AJidStr.isEmpty())
    {
      int slash = AJidStr.indexOf("/");
      if (slash == -1)
        slash = AJidStr.size();
      int at = AJidStr.lastIndexOf("@",slash-AJidStr.size()-1);
      setNode(at > 0 ? AJidStr.left(at) : "");
      setDomain(slash-at-1 > 0 ? AJidStr.mid(at+1,slash-at-1) : "");
      setResource(slash < AJidStr.size()-1 ? AJidStr.right(AJidStr.size()-slash-1) : "");
    }
    else
    {
      setNode("");
      setDomain("");
      setResource("");
    }
    FCache.insert(AJidStr,*this);
  }
  else 
    *this = FCache.value(AJidStr);

  return *this;
}

QString Jid::toString(bool AEscaped, bool APrepared, bool AFull) const
{
  QString result;
  QString node =  AEscaped ? d->FEscNode : (APrepared ? d->FPrepNode : d->FNode);
  QString domain = APrepared ? d->FPrepDomain : d->FDomain;
  QString resource = APrepared ? d->FPrepResource : d->FResource;

  if (!node.isEmpty())
    result = node + "@";

  if (!domain.isEmpty())
    result += domain;

  if (AFull && !resource.isEmpty())
    result += "/" + resource;

  return result;
}

bool Jid::equals(const Jid &AJid, bool AFull) const
{
  return (d->FPrepNode == AJid.pNode()) && (d->FPrepDomain == AJid.pDomain()) && (!AFull || d->FPrepResource == AJid.pResource()); 
}

uint qHash(const Jid &key)
{
  return qHash(key.pFull()); 
}
