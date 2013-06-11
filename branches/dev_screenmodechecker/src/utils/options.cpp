#include "options.h"

#include <QFile>
#include <QRect>
#include <QDataStream>
#include <QStringList>
#include <QKeySequence>
#include <QCryptographicHash>

const OptionsNode OptionsNode::null = OptionsNode(QDomElement());

QDomElement findChildElement(const QDomElement &AParent, const QString &APath, const QString &ANSpace, QString &ChildName, QString &SubPath, QString &NSpace)
{
	int dotIndex = APath.indexOf('.');
	ChildName = dotIndex>0 ? APath.left(dotIndex) : APath;
	SubPath = dotIndex>0 ? APath.mid(dotIndex+1) : QString::null;

	int nsStart = ChildName.indexOf('[');
	NSpace = nsStart>0 ? ChildName.mid(nsStart+1, ChildName.lastIndexOf(']')-nsStart-1) : QString::null;
	NSpace = dotIndex>0 ? NSpace : (ANSpace.isNull() ? NSpace : ANSpace);
	ChildName = nsStart>0 ? ChildName.left(nsStart) : ChildName;

	QDomElement childElem = AParent.firstChildElement(ChildName);
	while (!childElem.isNull() && childElem.attribute("ns")!=NSpace)
		childElem = childElem.nextSiblingElement(ChildName);

	return childElem;
}

QDomText findChildText(const QDomElement &AParent)
{
	for (QDomNode node = AParent.firstChild(); !node.isNull(); node=node.nextSibling())
		if (node.isText())
			return node.toText();
	return QDomText();
}

QString fullOptionsPath(const QString &APath, const QString &ASubPath)
{
	if (ASubPath.isEmpty())
		return APath;
	else if (APath.isEmpty())
		return ASubPath;
	else
		return APath+ "." +ASubPath;
}

QString fullFileName(const QString &APath, const QString &ANSpace)
{
	QString fileKey = APath + (!ANSpace.isEmpty() ? "["+ANSpace+"]" : QString::null);
	return Options::filesPath() + "/" + QCryptographicHash::hash(fileKey.toUtf8(), QCryptographicHash::Sha1).toHex();
}

QString variantToString(const QVariant &AVariant)
{
	if (AVariant.type() == QVariant::Rect)
	{
		QRect rect = AVariant.toRect();
		return QString("%1;%2;%3;%4").arg(rect.left()).arg(rect.top()).arg(rect.width()).arg(rect.height());
	}
	else if (AVariant.type() == QVariant::Point)
	{
		QPoint point = AVariant.toPoint();
		return QString("%1;%2").arg(point.x()).arg(point.y());
	}
	else if (AVariant.type() == QVariant::Size)
	{
		QSize size = AVariant.toSize();
		return QString("%1;%2").arg(size.width()).arg(size.height());
	}
	else if (AVariant.type() == QVariant::ByteArray)
	{
		return AVariant.toByteArray().toBase64();
	}
	else if (AVariant.type() == QVariant::StringList)
	{
		return AVariant.toStringList().join(" ;; ");
	}
	else if (AVariant.type() == QVariant::KeySequence)
	{
		return AVariant.value<QKeySequence>().toString(QKeySequence::PortableText);
	}
	return AVariant.toString();
}

QVariant stringToVariant(const QString &AString, QVariant::Type AType)
{
	if (AType == QVariant::Rect)
	{
		QList<QString> parts = AString.split(";",QString::SkipEmptyParts);
		if (parts.count() == 4)
			return QRect(parts.at(0).toInt(),parts.at(1).toInt(),parts.at(2).toInt(),parts.at(3).toInt());
	}
	else if (AType == QVariant::Point)
	{
		QList<QString> parts = AString.split(";",QString::SkipEmptyParts);
		if (parts.count() == 2)
			return QPoint(parts.at(0).toInt(),parts.at(1).toInt());
	}
	else if (AType == QVariant::Size)
	{
		QList<QString> parts = AString.split(";",QString::SkipEmptyParts);
		if (parts.count() == 2)
			return QSize(parts.at(0).toInt(),parts.at(1).toInt());
	}
	else if (AType == QVariant::ByteArray)
	{
		return QByteArray::fromBase64(AString.toLatin1());
	}
	else if (AType == QVariant::StringList)
	{
		return !AString.isEmpty() ? AString.split(" ;; ") : QStringList();
	}
	else if(AType == QVariant::KeySequence)
	{
		return QKeySequence::fromString(AString,QKeySequence::PortableText);
	}
	else
	{
		QVariant var = QVariant(AString);
		if (var.convert(AType))
			return var;
	}
	return QVariant();
}

void exportOptionNode(const OptionsNode &ANode, QDomElement &AToElem)
{
	QVariant value = ANode.value();
	if (!value.isNull())
	{
		QDomText text = findChildText(AToElem);
		if (!text.isNull())
			text.setData(variantToString(value));
		else
			AToElem.appendChild(AToElem.ownerDocument().createTextNode(variantToString(value)));
		AToElem.setAttribute("type",value.type());
	}
	else if (AToElem.hasAttribute("type"))
	{
		AToElem.removeAttribute("type");
		AToElem.removeChild(findChildText(AToElem));
	}

	QString cname, spath, nspace;
	foreach(QString childName, ANode.childNames())
	{
		foreach (QString childNSpace, ANode.childNSpaces(childName))
		{
			QDomElement childElem = findChildElement(AToElem,childName,childNSpace,cname,spath,nspace);
			if (childElem.isNull())
			{
				childElem = AToElem.appendChild(AToElem.ownerDocument().createElement(cname)).toElement();
				if (!nspace.isEmpty())
					childElem.setAttribute("ns",nspace);
			}
			exportOptionNode(ANode.node(childName,childNSpace),childElem);
		}
	}
}

void importOptionNode(OptionsNode &ANode, const QDomElement &AFromElem)
{
	if (AFromElem.hasAttribute("type"))
	{
		QString stringValue = findChildText(AFromElem).data();
		ANode.setValue(stringToVariant(!stringValue.isNull() ? stringValue : QString(""), (QVariant::Type)AFromElem.attribute("type").toInt()));
	}
	else
		ANode.setValue(QVariant());

	QDomElement childElem = AFromElem.firstChildElement();
	while (!childElem.isNull())
	{
		OptionsNode node = ANode.node(childElem.tagName(), childElem.attribute("ns"));
		importOptionNode(node,childElem);
		childElem = childElem.nextSiblingElement();
	}
}

#define XTEA_ITERATIONS 64
#define rol(N, R) (((N) << (R)) | ((N) >> (32 - (R))))
void xtea2_encipher(unsigned int num_rounds, quint32 *v, quint32 const *k)
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

void xtea2_decipher(unsigned int num_rounds, quint32 *v, quint32 const *k)
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
	if (d->path.isEmpty())
		d->path = Options::node(QString::null).childPath(*this);
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
	QString result;
	QDomElement childElem = ANode.d->node;
	while (!childElem.isNull() && childElem!=d->node)
	{
		QString pathItem = !childElem.hasAttribute("ns") ? childElem.tagName() : childElem.tagName()+"["+childElem.attribute("ns")+"]";
		if (result.isEmpty())
			result = pathItem;
		else
			result.prepend(pathItem + ".");
		childElem = childElem.parentNode().toElement();
	}
	return childElem==d->node ? result : QString::null;
}

void OptionsNode::removeChilds(const QString &AName, const QString &ANSpace)
{
	QDomElement childElem = d->node.firstChildElement();
	while (!childElem.isNull())
	{
		QDomElement nextChild = childElem.nextSiblingElement();
		if ((AName.isNull() || childElem.tagName()==AName) && (ANSpace.isNull() || childElem.attribute("ns")==ANSpace))
		{
			OptionsNode(childElem).removeChilds();
			emit Options::instance()->optionsRemoved(OptionsNode(childElem));
			d->node.removeChild(childElem);
		}
		childElem = nextChild;
	}
}

bool OptionsNode::hasNode(const QString &APath, const QString &ANSpace) const
{
	if (!APath.isEmpty())
	{
		QString cname, spath, nspace;
		QDomElement childElem = findChildElement(d->node,APath,ANSpace,cname,spath,nspace);
		return spath.isEmpty() || childElem.isNull() ? !childElem.isNull() : OptionsNode(childElem).hasNode(spath,ANSpace);
	}
	return !isNull();
}

OptionsNode OptionsNode::node(const QString &APath, const QString &ANSpace) const
{
	QString cname, spath, nspace;
	QDomElement childElem = findChildElement(d->node,APath,ANSpace,cname,spath,nspace);
	if (!isNull() && childElem.isNull())
	{
		childElem = d->node.appendChild(d->node.ownerDocument().createElement(cname)).toElement();
		if (!nspace.isEmpty())
			childElem.setAttribute("ns",nspace);
		emit Options::instance()->optionsCreated(OptionsNode(childElem));
	}
	return spath.isEmpty() || childElem.isNull() ? OptionsNode(childElem) : OptionsNode(childElem).node(spath, ANSpace);
}

void OptionsNode::removeNode(const QString &APath, const QString &ANSpace)
{
	QString cname, spath, nspace;
	QDomElement childElem = findChildElement(d->node,APath,ANSpace,cname,spath,nspace);
	if (!isNull() && !childElem.isNull())
	{
		if (!spath.isEmpty())
		{
			OptionsNode(childElem).removeNode(spath,ANSpace);
		}
		if (spath.isEmpty() || (!childElem.hasAttribute("type") && !childElem.hasChildNodes()))
		{
			emit Options::instance()->optionsRemoved(OptionsNode(childElem));
			d->node.removeChild(childElem);
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
			return stringToVariant(!stringValue.isNull() ? stringValue : QString(""), (QVariant::Type)d->node.attribute("type").toInt());
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
						text.setData(variantToString(AValue));
					else
						d->node.appendChild(d->node.ownerDocument().createTextNode(variantToString(AValue)));
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
	QString cleanPath = APath;
	for (int nsStart = cleanPath.indexOf('['); nsStart>=0; nsStart = cleanPath.indexOf('['))
		cleanPath.remove(nsStart,cleanPath.indexOf(']',nsStart)-nsStart+1);
	return cleanPath;
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

void Options::setOptions(QDomDocument AOptions, const QString &AFilesPath, const QByteArray &ACryptKey)
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
	item.defValue = ADefault;
	emit instance()->defaultValueChanged(APath,ADefault);
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

void Options::exportNode(const QString &APath, QDomElement &AToElem)
{
	if (hasNode(APath))
	{
		QString path = APath;
		QString cname, spath, nspace;
		QDomElement nodeElem = AToElem;
		while (!path.isEmpty())
		{
			QDomElement childElem = findChildElement(nodeElem,path,QString::null,cname,spath,nspace);
			if (childElem.isNull())
			{
				childElem = nodeElem.appendChild(nodeElem.ownerDocument().createElement(cname)).toElement();
				if (!nspace.isEmpty())
					childElem.setAttribute("ns",nspace);
			}
			path = spath;
			nodeElem = childElem;
		}
		exportOptionNode(Options::node(APath), nodeElem);
	}
}

void Options::importNode(const QString &APath, const QDomElement &AFromElem)
{
	QString path = APath;
	QString cname, spath, nspace;
	QDomElement nodeElem = AFromElem;
	while (!nodeElem.isNull() && !path.isEmpty())
	{
		QDomElement childElem = findChildElement(nodeElem,path,QString::null,cname,spath,nspace);
		path = spath;
		nodeElem = childElem;
	}

	if (!nodeElem.isNull())
	{
		OptionsNode node = Options::node(APath);
		importOptionNode(node,nodeElem);
	}
}
