#include "jid.h"

#ifdef USE_SYSTEM_IDN
#	include <stringprep.h>
#else
#	include <thirdparty/idn/stringprep.h>
#endif

static const QChar CharDog = '@';
static const QChar CharSlash = '/';
QHash<QString,Jid> JidCache = QHash<QString,Jid>();
QList<QChar> EscChars =     QList<QChar>()   << 0x20 << 0x22 << 0x26 << 0x27 << 0x2f << 0x3a << 0x3c << 0x3e << 0x40; // << 0x5c;
QList<QString> EscStrings = QList<QString>() <<"\\20"<<"\\22"<<"\\26"<<"\\27"<<"\\2f"<<"\\3a"<<"\\3c"<<"\\3e"<<"\\40"; //<<"\\5c";

Jid Jid::null = Jid();

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
	FFull = AOther.FFull;
	FEscFull = AOther.FEscFull;
	FPrepFull = AOther.FPrepFull;

	FBare = AOther.FBare;
	FEscBare = AOther.FEscBare;
	FPrepBare = AOther.FPrepBare;

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
	parseFromString(AJidStr);
}

Jid::Jid(const QString &AJidStr)
{
	d = NULL;
	parseFromString(AJidStr);
}

Jid::Jid(const QString &ANode, const QString &ADomane, const QString &AResource)
{
	d = NULL;
	parseFromString(ANode+CharDog+ADomane+CharSlash+AResource);
}

Jid::~Jid()
{

}

bool Jid::isValid() const
{
	return d->FDomainValid && d->FNodeValid && d->FResourceValid && d->FEscFull.size()<1023;
}

bool Jid::isEmpty() const
{
	return d->FNode.isEmpty() && d->FDomain.isEmpty() && d->FResource.isEmpty();
}

QString Jid::node() const
{
	return d->FNode.toString();
}

QString Jid::eNode() const
{
	return d->FEscNode.toString();
}

QString Jid::pNode() const
{
	return d->FPrepNode.toString();
}

void Jid::setNode(const QString &ANode)
{
	parseFromString(ANode+CharDog+domain()+CharSlash+resource());
}

QString Jid::domain() const
{
	return d->FDomain.toString();
}

QString Jid::pDomain() const
{
	return d->FPrepDomain.toString();
}

void Jid::setDomain(const QString &ADomain)
{
	parseFromString(node()+CharDog+ADomain+CharSlash+resource());
}

QString Jid::resource() const
{
	return d->FResource.toString();
}

QString Jid::pResource() const
{
	return d->FPrepResource.toString();
}

void Jid::setResource(const QString &AResource)
{
	parseFromString(node()+CharDog+domain()+CharSlash+AResource);
}

Jid Jid::prepared() const
{
	return d->FPrepFull;
}

QString Jid::full() const
{
	return d->FFull;
}

QString Jid::bare() const
{
	return d->FBare.toString();
}

QString Jid::eFull() const
{
	return d->FEscFull;
}

QString Jid::eBare() const
{
	return d->FEscBare.toString();
}

QString Jid::pFull() const
{
	return d->FPrepFull;
}

QString Jid::pBare() const
{
	return d->FPrepBare.toString();
}

Jid &Jid::operator=(const QString &AJidStr)
{
	return parseFromString(AJidStr);
}

bool Jid::operator==(const Jid &AJid) const
{
	return (d->FPrepFull == AJid.d->FPrepFull);
}

bool Jid::operator==(const QString &AJidStr) const
{
	return (d->FPrepFull == Jid(AJidStr).d->FPrepFull);
}

bool Jid::operator!=(const Jid &AJid) const
{
	return !operator==(AJid);
}

bool Jid::operator!=(const QString &AJidStr) const
{
	return !operator==(AJidStr);
}

bool Jid::operator&&(const Jid &AJid) const
{
	return (d->FPrepBare == AJid.d->FPrepBare);
}

bool Jid::operator&&(const QString &AJidStr) const
{
	return (d->FPrepBare == Jid(AJidStr).d->FPrepBare);
}

bool Jid::operator<(const Jid &AJid) const
{
	return (d->FPrepFull < AJid.d->FPrepFull);
}

bool Jid::operator>(const Jid &AJid) const
{
	return (d->FPrepFull > AJid.d->FPrepFull);
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

Jid &Jid::parseFromString(const QString &AJidStr)
{
	if (!JidCache.contains(AJidStr))
	{
		if (!d)
			d = new JidData;
		if (!AJidStr.isEmpty())
		{
			int slash = AJidStr.indexOf(CharSlash);
			if (slash == -1)
				slash = AJidStr.size();
			int at = AJidStr.lastIndexOf(CharDog,slash-AJidStr.size()-1);

			// Build normal JID
			d->FFull = QString::null;

			if (at > 0)
			{
				d->FFull += unescape106(AJidStr.left(at));
				d->FNode = QStringRef(&d->FFull,0,d->FFull.size());
				d->FFull.append(CharDog);
			}
			else
			{
				d->FNode = QStringRef(NULL,0,0);
			}

			if (slash-at-1 > 0)
			{
				int nodeSize = d->FFull.size();
				d->FFull += AJidStr.mid(at+1,slash-at-1);
				d->FDomain = QStringRef(&d->FFull,nodeSize,d->FFull.size()-nodeSize);
			}
			else
			{
				d->FDomain = QStringRef(NULL,0,0);
			}

			if (!d->FFull.isEmpty())
			{
				d->FBare = QStringRef(&d->FFull,0,d->FFull.size());
			}
			else
			{
				d->FBare = QStringRef(NULL,0,0);
			}

			if (slash<AJidStr.size()-1)
			{
				d->FFull.append(CharSlash);
				int bareSize = d->FFull.size();
				d->FFull += AJidStr.right(AJidStr.size()-slash-1);
				d->FResource = QStringRef(&d->FFull,bareSize,d->FFull.size()-bareSize);
			}
			else
			{
				d->FResource = QStringRef(NULL,0,0);
			}

			// Build escaped JID
			d->FEscFull = QString::null;

			if (d->FNode.string())
			{
				d->FEscFull += escape106(d->FNode.toString());
				d->FEscNode = QStringRef(&d->FEscFull,0,d->FEscFull.size());
				d->FEscFull.append(CharDog);
			}
			else
			{
				d->FEscNode = QStringRef(NULL,0,0);
			}

			if (d->FDomain.string())
			{
				d->FEscFull += d->FDomain.toString();
			}

			if (!d->FEscFull.isEmpty())
			{
				d->FEscBare = QStringRef(&d->FEscFull,0,d->FEscFull.size());
			}
			else
			{
				d->FEscBare = QStringRef(NULL,0,0);
			}

			if (d->FResource.string())
			{
				d->FEscFull.append(CharSlash);
				d->FEscFull += d->FResource.toString();
			}

			//Build prepared JID
			d->FPrepFull = QString::null;

			if (d->FEscNode.string())
			{
				QString prepNode = nodePrepare(d->FEscNode.toString());
				if (!prepNode.isEmpty())
				{
					d->FPrepFull += prepNode;
					d->FNodeValid = true;
				}
				else
				{
					d->FPrepFull += d->FEscNode.toString();
					d->FNodeValid = false;
				}
				d->FPrepNode = QStringRef(&d->FPrepFull,0,d->FPrepFull.size());
				d->FPrepFull.append(CharDog);
			}
			else
			{
				d->FPrepNode = QStringRef(NULL,0,0);
				d->FNodeValid = true;
			}

			if (d->FDomain.string())
			{
				int nodeSize = d->FPrepFull.size();
				QString prepDomain = domainPrepare(d->FDomain.toString());
				if (!prepDomain.isEmpty())
				{
					d->FPrepFull += prepDomain;
					d->FDomainValid = true;
				}
				else
				{
					d->FPrepFull += d->FDomain.toString();
					d->FDomainValid = false;
				}
				d->FPrepDomain = QStringRef(&d->FPrepFull,nodeSize,d->FPrepFull.size()-nodeSize);
			}
			else
			{
				d->FPrepDomain = QStringRef(NULL,0,0);
				d->FDomainValid = false;
			}

			if (!d->FPrepFull.isEmpty())
			{
				d->FPrepBare = QStringRef(&d->FPrepFull,0,d->FPrepFull.size());
			}
			else	
			{
				d->FPrepBare = QStringRef(NULL,0,0);
			}

			if (d->FResource.string())
			{
				d->FPrepFull.append(CharSlash);
				int bareSize = d->FPrepFull.size();
				QString prepResource = resourcePrepare(d->FResource.toString());
				if (!prepResource.isEmpty())
				{
					d->FPrepFull += prepResource;
					d->FResourceValid = true;
				}
				else
				{
					d->FPrepFull += d->FResource.toString();
					d->FResourceValid = false;
				}
				d->FPrepResource = QStringRef(&d->FPrepFull,bareSize,d->FPrepFull.size()-bareSize);
			}
			else
			{
				d->FPrepResource = QStringRef(NULL,0,0);
				d->FResourceValid = true;
			}
		}
		else
		{
			d->FFull = d->FEscFull = d->FPrepFull = QString::null;
			d->FBare = d->FEscBare = d->FPrepBare = QStringRef(NULL,0,0);
			d->FNode = d->FEscNode = d->FPrepNode = QStringRef(NULL,0,0);
			d->FDomain = d->FPrepDomain = QStringRef(NULL,0,0);
			d->FResource = d->FPrepResource = QStringRef(NULL,0,0);
			d->FNodeValid = d->FDomainValid = d->FResourceValid = false;
		}
		JidCache.insert(AJidStr,*this);
	}
	else
	{
		*this = JidCache.value(AJidStr);
	}
	return *this;
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
