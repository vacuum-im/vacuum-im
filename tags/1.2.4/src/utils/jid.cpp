#include "jid.h"

#ifdef USE_SYSTEM_IDN
#	include <stringprep.h>
#else
#	include <thirdparty/idn/stringprep.h>
#endif

static const QChar CharDog = '@';
static const QChar CharSlash = '/';
QHash<QString,Jid> JidCache = QHash<QString,Jid>();
QList<QChar> EscChars =     QList<QChar>()   << 0x5c << 0x20 << 0x22 << 0x26 << 0x27 << 0x2f << 0x3a << 0x3c << 0x3e << 0x40;
QList<QString> EscStrings = QList<QString>() <<"\\5c"<<"\\20"<<"\\22"<<"\\26"<<"\\27"<<"\\2f"<<"\\3a"<<"\\3c"<<"\\3e"<<"\\40";

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
	FPrepFull = AOther.FPrepFull;

	FBare = AOther.FBare;
	FPrepBare = AOther.FPrepBare;

	FNode = AOther.FNode;
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
	return d->FDomainValid && d->FNodeValid && d->FResourceValid && d->FFull.size()<1023;
}

bool Jid::isEmpty() const
{
	return d->FNode.isEmpty() && d->FDomain.isEmpty() && d->FResource.isEmpty();
}

QString Jid::node() const
{
	return d->FNode.toString();
}

QString Jid::pNode() const
{
	return d->FPrepNode.toString();
}

QString Jid::uNode() const
{
	return unescape(d->FNode.toString());
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

QString Jid::bare() const
{
	return d->FBare.toString();
}

QString Jid::pBare() const
{
	return d->FPrepBare.toString();
}

QString Jid::uBare() const
{
	QString ubare;
	if (d->FNode.string())
	{
		ubare += unescape(d->FNode.toString());
		ubare += CharDog;
	}
	ubare += d->FDomain.toString();
	return ubare;
}

QString Jid::full() const
{
	return d->FFull;
}

QString Jid::pFull() const
{
	return d->FPrepFull;
}

QString Jid::uFull() const
{
	QString ufull = uBare();
	if (d->FResource.string())
	{
		ufull += CharSlash;
		ufull += d->FResource.toString();
	}
	return ufull;
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

Jid Jid::fromUserInput(const QString &AJidStr)
{
	if (!AJidStr.isEmpty())
	{
		int at = AJidStr.indexOf(CharDog);
		int slash = AJidStr.lastIndexOf(CharSlash);
		if (slash==-1 || slash<at)
			slash = AJidStr.size();
		at = AJidStr.lastIndexOf(CharDog,slash-AJidStr.size()-1);
		if (at > 0)
			return Jid(escape(unescape(AJidStr.left(at))) + AJidStr.right(AJidStr.size()-at));
		return Jid(AJidStr);
	}
	return Jid::null;
}

QString Jid::escape(const QString &AUserNode)
{
	QString escNode;
	if (!AUserNode.isEmpty())
	{
		escNode.reserve(AUserNode.length()*3);

		for (int i = 0; i<AUserNode.length(); i++)
		{
			int index = EscChars.indexOf(AUserNode.at(i));
			if (index==0 && EscStrings.indexOf(AUserNode.mid(i,3))>=0)
				escNode.append(EscStrings.at(index));
			else if (index > 0)
				escNode.append(EscStrings.at(index));
			else
				escNode.append(AUserNode.at(i));
		}
		
		escNode.squeeze();
	}
	return escNode;
}

QString Jid::unescape(const QString &AEscNode)
{
	QString nodeStr;
	if (!AEscNode.isEmpty())
	{
		nodeStr.reserve(AEscNode.length());

		int index;
		for (int i = 0; i<AEscNode.length(); i++)
		{
			if (AEscNode.at(i)=='\\' && (index = EscStrings.indexOf(AEscNode.mid(i,3)))>=0)
			{
				nodeStr.append(EscChars.at(index));
				i+=2;
			}
			else
			{
				nodeStr.append(AEscNode.at(i));
			}
		}

		nodeStr.squeeze();
	}
	return nodeStr;
}

QString Jid::encode(const QString &AJidStr)
{
	QString encJid;
	if (!AJidStr.isEmpty())
	{
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
	}
	return encJid;
}

QString Jid::decode(const QString &AEncJid)
{
	QString jidStr;
	if (!AEncJid.isEmpty())
	{
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

		for (int i = jidStr.length(); i >= 0; i--)
		{
			if (jidStr.mid(i, 4) == "_at_")
			{
				jidStr.replace(i, 4, "@");
				break;
			}
		}

		jidStr.squeeze();
	}
	return jidStr;
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
		JidData *dd = d.data();

		if (!AJidStr.isEmpty())
		{
			int slash = AJidStr.indexOf(CharSlash);
			if (slash == -1)
				slash = AJidStr.size();
			int at = AJidStr.lastIndexOf(CharDog,slash-AJidStr.size()-1);

			// Build normal JID
			dd->FFull = QString::null;

			if (at > 0)
			{
				dd->FFull += AJidStr.left(at);
				dd->FNode = QStringRef(&dd->FFull,0,dd->FFull.size());
				dd->FFull.append(CharDog);
			}
			else
			{
				dd->FNode = QStringRef(NULL,0,0);
			}

			if (slash-at-1 > 0)
			{
				int nodeSize = dd->FFull.size();
				dd->FFull += AJidStr.mid(at+1,slash-at-1);
				dd->FDomain = QStringRef(&dd->FFull,nodeSize,dd->FFull.size()-nodeSize);
			}
			else
			{
				dd->FDomain = QStringRef(NULL,0,0);
			}

			if (!dd->FFull.isEmpty())
			{
				dd->FBare = QStringRef(&dd->FFull,0,dd->FFull.size());
			}
			else
			{
				dd->FBare = QStringRef(NULL,0,0);
			}

			if (slash<AJidStr.size()-1)
			{
				dd->FFull.append(CharSlash);
				int bareSize = dd->FFull.size();
				dd->FFull += AJidStr.right(AJidStr.size()-slash-1);
				dd->FResource = QStringRef(&dd->FFull,bareSize,dd->FFull.size()-bareSize);
			}
			else
			{
				dd->FResource = QStringRef(NULL,0,0);
			}

			//Build prepared JID
			dd->FPrepFull = QString::null;

			if (dd->FNode.string())
			{
				QString prepNode = nodePrepare(dd->FNode.toString());
				if (!prepNode.isEmpty())
				{
					dd->FPrepFull += prepNode;
					dd->FNodeValid = !prepNode.startsWith("\\20") && !prepNode.endsWith("\\20");
				}
				else
				{
					dd->FPrepFull += dd->FNode.toString();
					dd->FNodeValid = false;
				}
				dd->FPrepNode = QStringRef(&dd->FPrepFull,0,dd->FPrepFull.size());
				dd->FPrepFull.append(CharDog);
			}
			else
			{
				dd->FPrepNode = QStringRef(NULL,0,0);
				dd->FNodeValid = true;
			}

			if (dd->FDomain.string())
			{
				int nodeSize = dd->FPrepFull.size();
				QString prepDomain = domainPrepare(dd->FDomain.toString());
				if (!prepDomain.isEmpty())
				{
					dd->FPrepFull += prepDomain;
					dd->FDomainValid = true;
				}
				else
				{
					dd->FPrepFull += dd->FDomain.toString();
					dd->FDomainValid = false;
				}
				dd->FPrepDomain = QStringRef(&dd->FPrepFull,nodeSize,dd->FPrepFull.size()-nodeSize);
			}
			else
			{
				dd->FPrepDomain = QStringRef(NULL,0,0);
				dd->FDomainValid = false;
			}

			if (!dd->FPrepFull.isEmpty())
			{
				dd->FPrepBare = QStringRef(&dd->FPrepFull,0,dd->FPrepFull.size());
			}
			else
			{
				dd->FPrepBare = QStringRef(NULL,0,0);
			}

			if (dd->FResource.string())
			{
				dd->FPrepFull.append(CharSlash);
				int bareSize = dd->FPrepFull.size();
				QString prepResource = resourcePrepare(dd->FResource.toString());
				if (!prepResource.isEmpty())
				{
					dd->FPrepFull += prepResource;
					dd->FResourceValid = true;
				}
				else
				{
					dd->FPrepFull += dd->FResource.toString();
					dd->FResourceValid = false;
				}
				dd->FPrepResource = QStringRef(&dd->FPrepFull,bareSize,dd->FPrepFull.size()-bareSize);
			}
			else
			{
				dd->FPrepResource = QStringRef(NULL,0,0);
				dd->FResourceValid = true;
			}
		}
		else
		{
			dd->FFull = dd->FPrepFull = QString::null;
			dd->FBare = dd->FPrepBare = QStringRef(NULL,0,0);
			dd->FNode = dd->FPrepNode = QStringRef(NULL,0,0);
			dd->FDomain = dd->FPrepDomain = QStringRef(NULL,0,0);
			dd->FResource = dd->FPrepResource = QStringRef(NULL,0,0);
			dd->FNodeValid = dd->FDomainValid = dd->FResourceValid = false;
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
