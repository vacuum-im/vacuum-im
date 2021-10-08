#include "options.h"

#include <QFile>
#include <QRect>
#include <QDataStream>
#include <QStringList>
#include <QKeySequence>
#include <QCryptographicHash>

static const QChar DelimChar   = '.';
static const QChar NsOpenChar  = '[';
static const QChar NsCloseChar = ']';

const OptionsNode OptionsNode::null = OptionsNode(QDomElement());

static void splitOptionsPath(const QString &APath, const QString &ANSpace, QString &ANodeName, QString &ANodeNS, QString &ASubPath)
{
	int nsStartAt=-1, nsStopAt=-1, subStartAt=-1;
	for (int index=0; subStartAt<0 && index<APath.length(); index++)
	{
		if (APath.at(index)==DelimChar && (nsStartAt<0 || nsStopAt>0))
			subStartAt = index;
		else if (APath.at(index)==NsOpenChar && nsStartAt<0)
			nsStartAt = index;
		else if (APath.at(index)==NsCloseChar && nsStartAt>0)
			nsStopAt = index;
	}

	if (nsStartAt > 0)
		ANodeName = APath.left(nsStartAt);
	else if (subStartAt > 0)
		ANodeName = APath.left(subStartAt);
	else
		ANodeName = APath;

	if (subStartAt > 0)
		ASubPath = APath.mid(subStartAt+1);
	else
		ASubPath = QString();

	if (nsStopAt > nsStartAt)
		ANodeNS = APath.mid(nsStartAt+1,nsStopAt-nsStartAt-1);
	else if (subStartAt > 0)
		ANodeNS = QString();
	else
		ANodeNS = ANSpace;
}

static QDomElement findChildElement(const QDomElement &AParent, const QString &ANodeName, const QString &ANodeNS)
{
	QDomElement childElem = AParent.firstChildElement(ANodeName);
	while (!childElem.isNull() && childElem.attribute("ns")!=ANodeNS)
		childElem = childElem.nextSiblingElement(ANodeName);
	return childElem;
}

static QDomText findChildText(const QDomElement &AParent)
{
	for (QDomNode node = AParent.firstChild(); !node.isNull(); node=node.nextSibling())
		if (node.isText())
			return node.toText();
	return QDomText();
}

static QString fullFileName(const QString &APath, const QString &ANSpace)
{
	QString fileKey = APath + (!ANSpace.isEmpty() ? NsOpenChar+ANSpace+NsCloseChar : QString());
	return Options::filesPath() + "/" + QCryptographicHash::hash(fileKey.toUtf8(), QCryptographicHash::Sha1).toHex();
}

static void exportOptionNode(const OptionsNode &ANode, QDomElement &AToElem)
{
	QVariant value = ANode.value();
	if (!value.isNull())
	{
		QDomText text = findChildText(AToElem);
		if (!text.isNull())
			text.setData(Options::variantToString(value));
		else
			AToElem.appendChild(AToElem.ownerDocument().createTextNode(Options::variantToString(value)));
		AToElem.setAttribute("type",value.type());
	}
	else if (AToElem.hasAttribute("type"))
	{
		AToElem.removeAttribute("type");
		AToElem.removeChild(findChildText(AToElem));
	}

	foreach(const QString &nodeName, ANode.childNames())
	{
		foreach (const QString &nodeNS, ANode.childNSpaces(nodeName))
		{
			QDomElement nodeElem = findChildElement(AToElem,nodeName,nodeNS);
			if (nodeElem.isNull())
			{
				nodeElem = AToElem.appendChild(AToElem.ownerDocument().createElement(nodeName)).toElement();
				if (!nodeNS.isEmpty())
					nodeElem.setAttribute("ns",nodeNS);
			}
			exportOptionNode(ANode.node(nodeName,nodeNS),nodeElem);
		}
	}
}

static void importOptionNode(OptionsNode &ANode, const QDomElement &AFromElem)
{
	if (AFromElem.hasAttribute("type"))
	{
		QString stringValue = findChildText(AFromElem).data();
		ANode.setValue(Options::stringToVariant(!stringValue.isNull() ? stringValue : QString(""), (QVariant::Type)AFromElem.attribute("type").toInt()));
	}
	else
	{
		ANode.setValue(QVariant());
	}

	QDomElement childElem = AFromElem.firstChildElement();
	while (!childElem.isNull())
	{
		OptionsNode node = ANode.node(childElem.tagName(),childElem.attribute("ns"));
		importOptionNode(node,childElem);

		childElem = childElem.nextSiblingElement();
	}
}

#define XTEA_ITERATIONS 64
#define rol(N, R) (((N) << (R)) | ((N) >> (32 - (R))))
static void xtea2_encipher(unsigned int num_rounds, quint32 *v, quint32 const *k)
{
	quint32 i;
	quint32 a, b, c, d, sum=0, t,delta=0x9E3779B9;
	a = v[0];
	b = v[1] + k[0];
	c = v[2];
	d = v[3] + k[1];
	for (i = 0; i < num_rounds; i++)
	{
		a += ((b << 4) ^ (b >> 5)) + (d ^ sum) + rol(k[sum & 3], b);
		sum += delta;
		c += ((d << 4) ^ (d >> 5)) + (b ^ sum) + rol(k[(sum >> 11) & 3], d);
		t = a;
		a = b;
		b = c;
		c = d;
		d = t;
	}
	v[0] = a ^ k[2];
	v[1] = b;
	v[2] = c ^ k[3];
	v[3] = d;
}

static void xtea2_decipher(unsigned int num_rounds, quint32 *v, quint32 const *k)
{
	quint32 i;
	quint32 a, b, c, d, t, delta=0x9E3779B9, sum=delta*num_rounds;
	d = v[3];
	c = v[2] ^ k[3];
	b = v[1];
	a = v[0] ^ k[2];
	for (i = 0; i < num_rounds; i++)
	{
		t = d;
		d = c;
		c = b;
		b = a;
		a = t;
		c -= ((d << 4) ^ (d >> 5)) + (b ^ sum) + rol(k[(sum >> 11) & 3], d);
		sum -= delta;
		a -= ((b << 4) ^ (b >> 5)) + (d ^ sum) + rol(k[sum & 3], b);
	}
	v[0] = a;
	v[1] = b - k[0];
	v[2] = c;
	v[3] = d - k[1];
}

//OptionNode
struct OptionsNode::OptionsNodeData
{
	int refCount;
	QString path;
	QString cleanPath;
	QDomElement node;
};

OptionsNode::OptionsNode()
{
	d = NULL;
	operator=(OptionsNode::null);
}

OptionsNode::OptionsNode(const OptionsNode &ANode)
{
	d = NULL;
	operator=(ANode);
}

OptionsNode::OptionsNode(const QDomElement &ANode)
{
	d = new OptionsNodeData;
	d->refCount = 1;
	d->node = ANode;
}

OptionsNode::~OptionsNode()
{
	if (!(--d->refCount))
		delete d;
}

bool OptionsNode::isNull() const
{
	return d->node.isNull();
}

QString OptionsNode::path() const
{
	if (d->path.isNull())
	{
		QDomElement rootElem = d->node;
		while (!rootElem.parentNode().toElement().isNull())
			rootElem = rootElem.parentNode().toElement();
		d->path = OptionsNode(rootElem).childPath(*this);
	}
	return d->path;
}

QString OptionsNode::name() const
{
	return d->node.tagName();
}

QString OptionsNode::nspace() const
{
	return d->node.attribute("ns");
}

QString OptionsNode::cleanPath() const
{
	if (d->cleanPath.isNull())
		d->cleanPath = Options::cleanNSpaces(path());
	return d->cleanPath;
}

OptionsNode OptionsNode::parent() const
{
	return OptionsNode(d->node.parentNode().toElement());
}

QList<QString> OptionsNode::parentNSpaces() const
{
	QList<QString> nspaces;
	QDomElement parentElem = d->node.parentNode().toElement();
	while (parentElem.parentNode().isElement())
	{
		nspaces.prepend(parentElem.attribute("ns"));
		parentElem = parentElem.parentNode().toElement();
	}
	return nspaces;
}

QList<QString> OptionsNode::childNames() const
{
	QList<QString> names;
	QDomElement childElem = d->node.firstChildElement();
	while (!childElem.isNull())
	{
		if (!names.contains(childElem.tagName()))
			names.append(childElem.tagName());
		childElem = childElem.nextSiblingElement();
	}
	return names;
}

QList<QString> OptionsNode::childNSpaces(const QString &AName) const
{
	QList<QString> nspaces;
	QDomElement childElem = d->node.firstChildElement(AName);
	while (!childElem.isNull())
	{
		nspaces.append(childElem.attribute("ns"));
		childElem = childElem.nextSiblingElement(AName);
	}
	return nspaces;
}

bool OptionsNode::isChildNode(const OptionsNode &ANode) const
{
	QDomElement childElem = ANode.d->node;
	while (!childElem.isNull())
	{
		if (d->node == childElem)
			return true;
		childElem = childElem.parentNode().toElement();
	}
	return false;
}

QString OptionsNode::childPath(const OptionsNode &ANode) const
{
	QString path;
	QDomElement childElem = ANode.d->node;
	while (!childElem.isNull() && childElem!=d->node)
	{
		QString pathItem = childElem.hasAttribute("ns") ? childElem.tagName() + NsOpenChar + childElem.attribute("ns") + NsCloseChar : childElem.tagName();
		if (path.isEmpty())
			path = pathItem;
		else
			path.prepend(pathItem + DelimChar);
		childElem = childElem.parentNode().toElement();
	}
	return childElem==d->node ? path : QString();
}

void OptionsNode::removeChilds(const QString &AName, const QString &ANSpace)
{
	QDomElement childElem = d->node.firstChildElement();
	while (!childElem.isNull())
	{
		QDomElement nextChild = childElem.nextSiblingElement();
		if ((AName.isNull() || childElem.tagName()==AName) && (ANSpace.isNull() || childElem.attribute("ns")==ANSpace))
		{
			OptionsNode childNode(childElem);
			childNode.removeChilds();

			emit Options::instance()->optionsRemoved(childNode);
			d->node.removeChild(childElem);
		}
		childElem = nextChild;
	}
}

bool OptionsNode::hasNode(const QString &APath, const QString &ANSpace) const
{
	if (!APath.isEmpty())
	{
		QString nodeName, nodeNS, subPath;
		splitOptionsPath(APath,ANSpace,nodeName,nodeNS,subPath);
		QDomElement childElem = findChildElement(d->node,nodeName,nodeNS);
		return !childElem.isNull() ? (!subPath.isEmpty() ? OptionsNode(childElem).hasNode(subPath,ANSpace) :true) : false;
	}
	return !isNull() && nspace()==ANSpace;
}

OptionsNode OptionsNode::node(const QString &APath, const QString &ANSpace) const
{
	if (!isNull())
	{
		QString nodeName, nodeNS, subPath;
		splitOptionsPath(APath,ANSpace,nodeName,nodeNS,subPath);

		QDomElement childElem = findChildElement(d->node,nodeName,nodeNS);
		if (childElem.isNull())
		{
			childElem = d->node.appendChild(d->node.ownerDocument().createElement(nodeName)).toElement();
			if (!nodeNS.isEmpty())
				childElem.setAttribute("ns",nodeNS);
			emit Options::instance()->optionsCreated(OptionsNode(childElem));
		}
		return !subPath.isEmpty() ?  OptionsNode(childElem).node(subPath,ANSpace) : OptionsNode(childElem);
	}
	return OptionsNode::null;
}

void OptionsNode::removeNode(const QString &APath, const QString &ANSpace)
{
	if (!isNull())
	{
		QString nodeName, nodeNS, subPath;
		splitOptionsPath(APath,ANSpace,nodeName,nodeNS,subPath);

		QDomElement childElem = findChildElement(d->node,nodeName,nodeNS);
		if (!childElem.isNull())
		{
			if (!subPath.isEmpty())
				OptionsNode(childElem).removeNode(subPath,ANSpace);

			if (subPath.isEmpty() || (!childElem.hasAttribute("type") && !childElem.hasChildNodes()))
			{
				emit Options::instance()->optionsRemoved(OptionsNode(childElem));
				d->node.removeChild(childElem);
			}
		}
	}
}

bool OptionsNode::hasValue(const QString &APath, const QString &ANSpace) const
{
	return APath.isEmpty() ? d->node.hasAttribute("type") : node(APath,ANSpace).hasValue();
}

QVariant OptionsNode::value(const QString &APath, const QString &ANSpace) const
{
	if (APath.isEmpty())
	{
		if (d->node.hasAttribute("type"))
		{
			QString stringValue = findChildText(d->node).data();
			return Options::stringToVariant(!stringValue.isNull() ? stringValue : QString(""), (QVariant::Type)d->node.attribute("type").toInt());
		}
		return Options::defaultValue(path());
	}
	return node(APath,ANSpace).value();
}

void OptionsNode::setValue(const QVariant &AValue, const QString &APath, const QString &ANSpace)
{
	if (!isNull())
	{
		if (APath.isEmpty())
		{
			if (value()!=AValue || d->node.hasAttribute("type")==AValue.isNull())
			{
				if (!AValue.isNull() && AValue!=Options::defaultValue(path()))
				{
					QDomText text = findChildText(d->node);
					if (!text.isNull())
						text.setData(Options::variantToString(AValue));
					else
						d->node.appendChild(d->node.ownerDocument().createTextNode(Options::variantToString(AValue)));
					d->node.setAttribute("type",AValue.type());
					emit Options::instance()->optionsChanged(*this);
				}
				else if (d->node.hasAttribute("type"))
				{
					d->node.removeChild(findChildText(d->node));
					d->node.removeAttribute("type");
					emit Options::instance()->optionsChanged(*this);
				}
			}
		}
		else
		{
			node(APath,ANSpace).setValue(AValue);
		}
	}
}

bool OptionsNode::operator==(const OptionsNode &AOther) const
{
	return d->node == AOther.d->node;
}

bool OptionsNode::operator!=(const OptionsNode &AOther) const
{
	return !operator==(AOther);
}

OptionsNode &OptionsNode::operator=(const OptionsNode &AOther)
{
	if (this!=&AOther && d!=AOther.d)
	{
		if (d && !(--d->refCount))
			delete d;
		d = AOther.d;
		d->refCount++;
	}
	return *this;
}

//Options
struct OptionItem
{
	QVariant defValue;
};

struct Options::OptionsData
{
	QString filesPath;
	QByteArray cryptKey;
	QDomDocument options;
	QHash<QString, OptionItem> items;
	QHash<QString, QString> cleanPathCache;
};

Options::Options()
{
	d = new OptionsData;
}

Options::~Options()
{
	delete d;
}

Options *Options::instance()
{
	static Options *inst = new Options;
	return inst;
}

bool Options::isNull()
{
	return instance()->d->options.isNull();
}

QString Options::filesPath()
{
	return instance()->d->filesPath;
}

QByteArray Options::cryptKey()
{
	return instance()->d->cryptKey;
}

QString Options::cleanNSpaces(const QString &APath)
{
	if (!APath.isEmpty())
	{
		QString &cleanPath = instance()->d->cleanPathCache[APath];
		if (cleanPath.isEmpty())
		{
			QString nodePath = APath;
			while(!nodePath.isEmpty())
			{
				QString nodeName, nodeNS, subPath;
				splitOptionsPath(nodePath,QString(),nodeName,nodeNS,subPath);

				if (cleanPath.isEmpty())
					cleanPath = nodeName;
				else
					cleanPath += DelimChar + nodeName;

				nodePath = subPath;
			}
		}
		return cleanPath;
	}
	return QString();
}

bool Options::hasNode(const QString &APath, const QString &ANSpace)
{
	return OptionsNode(instance()->d->options.documentElement()).hasNode(APath,ANSpace);
}

OptionsNode Options::node(const QString &APath, const QString &ANSpace)
{
	return APath.isEmpty() ? OptionsNode(instance()->d->options.documentElement()) : OptionsNode(instance()->d->options.documentElement()).node(APath,ANSpace);
}

QVariant Options::fileValue(const QString &APath, const QString &ANSpace)
{
	if (!Options::filesPath().isEmpty())
	{
		QFile file(fullFileName(APath,ANSpace));
		if (file.open(QFile::ReadOnly))
		{
			QVariant value;
			QDataStream stream(&file);
			stream >> value;
			file.close();
			return value;
		}
	}
	return Options::defaultValue(APath);
}

void Options::setFileValue(const QVariant &AValue, const QString &APath, const QString &ANSpace)
{
	if (!Options::filesPath().isEmpty())
	{
		if (!AValue.isNull())
		{
			QFile file(fullFileName(APath,ANSpace));
			if (file.open(QFile::WriteOnly|QFile::Truncate))
			{
				QDataStream stream(&file);
				stream << AValue;
				file.close();
			}
		}
		else
		{
			QFile::remove(fullFileName(APath,ANSpace));
		}
	}
}

void Options::setOptions(const QDomDocument &AOptions, const QString &AFilesPath, const QByteArray &ACryptKey)
{
	OptionsData *q = instance()->d;

	if (!q->options.isNull())
		emit instance()->optionsClosed();

	q->options = AOptions;
	q->filesPath = AFilesPath;
	q->cryptKey = ACryptKey;

	if (!q->options.isNull())
		emit instance()->optionsOpened();
}

QVariant Options::defaultValue(const QString &APath)
{
	return instance()->d->items.value(cleanNSpaces(APath)).defValue;
}

void Options::setDefaultValue(const QString &APath, const QVariant &ADefault)
{
	OptionItem &item = instance()->d->items[cleanNSpaces(APath)];
	if (item.defValue != ADefault)
	{
		item.defValue = ADefault;
		emit instance()->defaultValueChanged(APath,ADefault);
	}
}

QByteArray Options::encrypt(const QVariant &AValue, const QByteArray &AKey)
{
	if (AValue.type()>QVariant::Invalid && AValue.type()<QVariant::UserType && !AKey.isEmpty())
	{
		QByteArray cryptData = variantToString(AValue).toUtf8();
		if (cryptData.size() % 16)
			cryptData.append(QByteArray(16-(cryptData.size()%16),'\0'));

		QByteArray cryptKey = AKey;
		if (cryptKey.size() < 16)
		{
			int startSize = cryptKey.size();
			cryptKey.resize(16);
			for (int i = startSize; i < 16; i++)
				cryptKey[i] = cryptKey[i % startSize];
		}

		for (int i = 0; i<cryptData.size(); i+=16)
			xtea2_encipher(XTEA_ITERATIONS,(quint32 *)(cryptData.data()+i),(const quint32 *)cryptKey.constData());

		return QByteArray::number(AValue.type()) + QByteArray(1,';') + cryptData.toBase64();
	}
	return QByteArray();
}

QVariant Options::decrypt(const QByteArray &AData, const QByteArray &AKey)
{
	if (!AData.isEmpty() && !AKey.isEmpty())
	{
		QList<QByteArray> parts = AData.split(';');
		QVariant::Type valType = parts.count()>1 ? (QVariant::Type)parts.value(0).toInt() : QVariant::String;

		QByteArray cryptData = QByteArray::fromBase64(parts.value(parts.count()>1 ? 1 : 0));
		if ((cryptData.size() % 16) == 0)
		{
			QByteArray cryptKey = AKey;
			if (cryptKey.size() < 16)
			{
				int startSize = cryptKey.size();
				cryptKey.resize(16);
				for (int i = startSize; i < 16; i++)
					cryptKey[i] = cryptKey[i % startSize];
			}

			for (int i = 0; i<cryptData.size(); i+=16)
				xtea2_decipher(XTEA_ITERATIONS,(quint32 *)(cryptData.data()+i),(const quint32 *)cryptKey.constData());

			return stringToVariant(QString::fromUtf8(cryptData),valType);
		}
	}
	return QVariant();
}

QString Options::variantToString(const QVariant &AValue)
{
	if (AValue.type() == QVariant::Rect)
	{
		QRect rect = AValue.toRect();
		return QString("%1;%2;%3;%4").arg(rect.left()).arg(rect.top()).arg(rect.width()).arg(rect.height());
	}
	else if (AValue.type() == QVariant::Point)
	{
		QPoint point = AValue.toPoint();
		return QString("%1;%2").arg(point.x()).arg(point.y());
	}
	else if (AValue.type() == QVariant::Size)
	{
		QSize size = AValue.toSize();
		return QString("%1;%2").arg(size.width()).arg(size.height());
	}
	else if (AValue.type() == QVariant::ByteArray)
	{
		return AValue.toByteArray().toBase64();
	}
	else if (AValue.type() == QVariant::StringList)
	{
		return AValue.toStringList().join(" ;; ");
	}
	else if (AValue.type() == QVariant::KeySequence)
	{
		return AValue.value<QKeySequence>().toString(QKeySequence::PortableText);
	}
	return AValue.toString();
}

QVariant Options::stringToVariant(const QString &AValue, QVariant::Type AType)
{
	if (AType == QVariant::Rect)
	{
		QList<QString> parts = AValue.split(";", Qt::SkipEmptyParts);
		if (parts.count() == 4)
			return QRect(parts.at(0).toInt(),parts.at(1).toInt(),parts.at(2).toInt(),parts.at(3).toInt());
	}
	else if (AType == QVariant::Point)
	{
		QList<QString> parts = AValue.split(";", Qt::SkipEmptyParts);
		if (parts.count() == 2)
			return QPoint(parts.at(0).toInt(),parts.at(1).toInt());
	}
	else if (AType == QVariant::Size)
	{
		QList<QString> parts = AValue.split(";", Qt::SkipEmptyParts);
		if (parts.count() == 2)
			return QSize(parts.at(0).toInt(),parts.at(1).toInt());
	}
	else if (AType == QVariant::ByteArray)
	{
		return QByteArray::fromBase64(AValue.toLatin1());
	}
	else if (AType == QVariant::StringList)
	{
		return !AValue.isEmpty() ? AValue.split(" ;; ") : QStringList();
	}
	else if(AType == QVariant::KeySequence)
	{
		return QKeySequence::fromString(AValue,QKeySequence::PortableText);
	}
	else
	{
		QVariant var = QVariant(AValue);
		if (var.convert(AType))
			return var;
	}
	return QVariant();
}

void Options::exportNode(const QString &APath, QDomElement &AToElem)
{
	if (hasNode(APath))
	{
		QString nodePath = APath;
		QDomElement nodeElem = AToElem;
		while (!nodePath.isEmpty())
		{
			QString nodeName, nodeNS, subPath;
			splitOptionsPath(nodePath,QString(),nodeName,nodeNS,subPath);

			QDomElement childElem = findChildElement(nodeElem,nodeName,nodeNS);
			if (childElem.isNull())
			{
				childElem = nodeElem.appendChild(nodeElem.ownerDocument().createElement(nodeName)).toElement();
				if (!nodeNS.isEmpty())
					childElem.setAttribute("ns",nodeNS);
			}

			nodeElem = childElem;
			nodePath = subPath;
		}
		exportOptionNode(Options::node(APath),nodeElem);
	}
}

void Options::importNode(const QString &APath, const QDomElement &AFromElem)
{
	QString nodePath = APath;
	QDomElement nodeElem = AFromElem;
	while (!nodeElem.isNull() && !nodePath.isEmpty())
	{
		QString nodeName, nodeNS, subPath;
		splitOptionsPath(nodePath,QString(),nodeName,nodeNS,subPath);

		nodeElem = findChildElement(nodeElem,nodeName,nodeNS);
		nodePath = subPath;
	}

	if (!nodeElem.isNull())
	{
		OptionsNode node = Options::node(APath);
		importOptionNode(node,nodeElem);
	}
}

OptionsNode Options::createNodeForElement(const QDomElement &AElement)
{
	return OptionsNode(AElement);
}
