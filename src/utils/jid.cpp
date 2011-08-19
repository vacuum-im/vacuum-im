#include "jid.h"

#include <QTextDocument>

#ifdef USE_SYSTEM_IDN
#	include <stringprep.h>
#else
#	include <thirdparty/idn/stringprep.h>
#endif

QList<QChar> EscChars = QList<QChar>()       << 0x20 << 0x22 << 0x26 << 0x27 << 0x2f << 0x3a << 0x3c << 0x3e << 0x40; // << 0x5c;
QList<QString> EscStrings = QList<QString>() <<"\\20"<<"\\22"<<"\\26"<<"\\27"<<"\\2f"<<"\\3a"<<"\\3c"<<"\\3e"<<"\\40"; //<<"\\5c";
QHash<QString,Jid> JidCache = QHash<QString,Jid>();

QString stringPrepare(const Stringprep_profile *AProfile, const QString &AString)
{
	QByteArray buffer = AString.toUtf8();
	if (!buffer.isEmpty() && buffer.size()<1024)
	{
		buffer.reserve(1024);
		if (stringprep(buffer.data(),buffer.capacity(),(Stringprep_profile_flags)0, AProfile) == STRINGPREP_OK)
			return QString::fromUtf8(buffer.constData());
	}
	return QString::null;
}

JidData::JidData()
{
	FNodeValid = true;
	FDomainValid = false;
	FResourceValid = true;
}

JidData::JidData(const JidData &AOther) : QSharedData(AOther)
{
	FNode = AOther.FNode;
	FEscNode = AOther.FEscNode;
	FPrepNode = AOther.FPrepNode;
	FDomain = AOther.FDomain;
	FPrepDomain = AOther.FPrepDomain;
	FResource = AOther.FResource;
	FPrepResource = AOther.FPrepResource;
	FNodeValid = AOther.FNodeValid;
	FDomainValid = AOther.FDomainValid;
	FResourceValid = AOther.FResourceValid;
}


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

QString Jid::node() const
{
	return d->FNode;
}

QString Jid::hNode() const
{
	return Qt::escape(d->FEscNode);
}

QString Jid::eNode() const
{
	return d->FEscNode;
}

QString Jid::pNode() const
{
	return d->FPrepNode;
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

QString Jid::domain() const
{
	return d->FDomain;
}

QString Jid::pDomain() const
{
	return d->FPrepDomain;
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

QString Jid::resource() const
{
	return d->FResource;
}

QString Jid::pResource() const
{
	return d->FPrepResource;
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

QString Jid::full() const
{
	return toString(false,false,true);
}

QString Jid::bare() const
{
	return toString(false,false,false);
}

QString Jid::hFull() const
{
	return Qt::escape(toString(false,false,true));
}

QString Jid::hBare() const
{
	return Qt::escape(toString(false,false,false));
}

QString Jid::eFull() const
{
	return toString(true,false,true);
}

QString Jid::eBare() const
{
	return toString(true,false,false);
}

QString Jid::pFull() const
{
	return toString(false,true,true);
}

QString Jid::pBare() const
{
	return toString(false,true,false);
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
	return pFull() < AJid.pFull();
}

bool Jid::operator>(const Jid &AJid) const
{
	return pFull() > AJid.pFull();
}

QString Jid::encode(const QString &AJidStr)
{
	QString encJid;
	encJid.reserve(AJidStr.length()*3);

	for (int i = 0; i < AJidStr.length(); i++)
	{
		if (AJidStr.at(i) == '@')
		{
			encJid.append("_at_");
		}
		else if (AJidStr.at(i) == '.')
		{
			encJid.append('.');
		}
		else if (!AJidStr.at(i).isLetterOrNumber())
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

	for (int i = 0; i < AEncJid.length(); i++)
	{
		if (AEncJid.at(i) == '%' && (AEncJid.length()-i-1) >= 2)
		{
			jidStr.append(char(AEncJid.mid(i+1,2).toInt(NULL,16)));
			i += 2;
		}
		else
		{
			jidStr.append(AEncJid.at(i));
		}
	}

	for (int i = jidStr.length(); i >= 3; i--)
	{
		if (jidStr.mid(i, 4) == "_at_")
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

	for (int i = 0; i < AJidStr.length(); i++)
	{
		if (AJidStr.at(i) == '\\' || AJidStr.at(i) == '<' || AJidStr.at(i) == '>')
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

	for (int i = 0; i < AEncJid.length(); i++)
	{
		if (AEncJid.at(i) == '\\' && i+3 < AEncJid.length() && AEncJid.at(i+1) == 'x')
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
		int index = EscChars.indexOf(ANode.at(i));
		if (index >= 0)
			escNode.append(EscStrings.at(index));
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
		if (AEscNode.at(i) == '\\' && (index = EscStrings.indexOf(AEscNode.mid(i,3))) >= 0)
		{
			nodeStr.append(EscChars.at(index));
			i+=2;
		}
		else
			nodeStr.append(AEscNode.at(i));
	}

	nodeStr.squeeze();
	return nodeStr;
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
	if (!JidCache.contains(AJidStr))
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
			setNode(QString::null);
			setDomain(QString::null);
			setResource(QString::null);
		}
		JidCache.insert(AJidStr,*this);
	}
	else
		*this = JidCache.value(AJidStr);

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

uint qHash(const Jid &AKey)
{
	return qHash(AKey.pFull());
}

QDataStream &operator<<(QDataStream &AStream, const Jid &AJid)
{
	AStream << AJid.full();
	return AStream;
}

QDataStream &operator>>(QDataStream &AStream, Jid &AJid)
{
	QString jid;
	AStream >> jid;
	AJid = jid;
	return AStream;
}
