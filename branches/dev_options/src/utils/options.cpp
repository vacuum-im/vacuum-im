#include "options.h"

#include <QFile>
#include <QRect>
#include <QDataStream>
#include <QStringList>
#include <QCryptographicHash>

QDomElement findChildElement(const QDomElement &AParent, const QString &APath, const QString &ANSpace, 
                             QString &ChildName, QString &SubPath, QString &NSpace)
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
    return qCompress(AVariant.toByteArray()).toBase64();
  }
  else if (AVariant.type() == QVariant::StringList)
  {
    return AVariant.toStringList().join(" ;; ");
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
    return qUncompress(QByteArray::fromBase64(AString.toLatin1()));
  }
  else if (AType == QVariant::StringList)
  {
    return AString.split(" ;; ");
  }
  else
  {
    QVariant var = QVariant(AString);
    if (var.convert(AType))
      return var;
  }
  return QVariant();
}

//OptionNode
struct OptionsNode::OptionsNodeData
{
  int refCount;
  QString path;
  QDomElement node;
};

const OptionsNode OptionsNode::null = OptionsNode(QDomElement());

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
    nspaces.append(parentElem.attribute("ns"));
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

void OptionsNode::removeChilds(const QString &AName, const QString &ANSpace) const
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

bool OptionsNode::hasValue(const QString &APath, const QString &ANSpace) const
{
  return APath.isEmpty() ? d->node.hasAttribute("type") : node(APath,ANSpace).hasValue();
}

QVariant OptionsNode::value(const QString &APath, const QString &ANSpace) const
{
  if (APath.isEmpty())
  {
    if (d->node.hasAttribute("type"))
      return stringToVariant(findChildText(d->node).data(), (QVariant::Type)d->node.attribute("type").toInt());
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
  if (d && !(--d->refCount))
    delete d;
  d = AOther.d;
  d->refCount++;
  return *this;
}


//Options
struct OptionItem
{
  QString caption;
  QVariant defValue;
};

struct Options::OptionsData
{
  QString filesPath;
  QByteArray cryptKey;
  QDomDocument options;
  QHash<QString, OptionItem> items;
};

Options::OptionsData *Options::d = new Options::OptionsData;

Options *Options::instance()
{
  static Options *options = NULL;
  if (!options)
    options = new Options;
  return options;
}

bool Options::isNull()
{
  return d->options.isNull();
}

QString Options::filesPath()
{
  return d->filesPath;
}

QByteArray Options::cryptKey()
{
  return d->cryptKey;
}

QString Options::cleanNSpaces(const QString &APath)
{
  QString cleanPath = APath;
  for (int nsStart = cleanPath.indexOf('['); nsStart>=0; nsStart = cleanPath.indexOf('['))
    cleanPath.remove(nsStart,cleanPath.indexOf(']',nsStart)-nsStart+1);
  return cleanPath;
}

OptionsNode Options::node(const QString &APath, const QString &ANSpace)
{
  return APath.isEmpty() ? OptionsNode(d->options.documentElement()) : OptionsNode(d->options.documentElement()).node(APath,ANSpace);
}

QVariant Options::fileValue(const QString &APath, const QString &ANSpace)
{
  if (!Options::filesPath().isEmpty())
  {
    QFile file(fullFileName(APath,ANSpace));
    if (file.open(QFile::ReadOnly))
    {
      QVariant value;
      QDataStream(&file) >> value;
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
        QDataStream(&file) << AValue;
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
  if (!d->options.isNull())
    emit instance()->optionsClosed();

  d->options = AOptions;
  d->filesPath = AFilesPath;
  d->cryptKey = ACryptKey;

  if (!d->options.isNull())
    emit instance()->optionsOpened();
}

QString Options::caption(const QString &APath)
{
  return d->items.value(cleanNSpaces(APath)).caption;
}

QVariant Options::defaultValue(const QString &APath)
{
  return d->items.value(cleanNSpaces(APath)).defValue;
}

QList<QString> Options::registeredOptions()
{
  return d->items.keys();
}

void Options::registerOption(const QString &APath, const QVariant &ADefault, const QString &ACaption)
{
  OptionItem &item = d->items[cleanNSpaces(APath)];
  item.defValue = ADefault;
  item.caption = ACaption;
  emit instance()->optionRegistered(APath,ADefault,ACaption);
}

QByteArray Options::encrypt(const QVariant &AValue, const QByteArray &AKey)
{
  if (AValue.type()>QVariant::Invalid && AValue.type()<QVariant::UserType)
  {
    QByteArray crypted = variantToString(AValue).toUtf8();
    for (int i = 0; i<crypted.size(); ++i)
      crypted[i] = crypted[i] ^ AKey[i % AKey.size()];
    return QByteArray::number(AValue.type()) + QByteArray(1,';') + crypted.toBase64();
  }
  return QByteArray();
}

QVariant Options::decrypt(const QByteArray &AData, const QByteArray &AKey)
{
  if (!AData.isEmpty())
  {
    QList<QByteArray> parts = AData.split(';');
    QVariant::Type valType = parts.count()>1 ? (QVariant::Type)parts.value(0).toInt() : QVariant::String;
    QByteArray valData = QByteArray::fromBase64(parts.value(parts.count()>1 ? 1 : 0));
    for (int i = 0; i<valData.size(); ++i)
      valData[i] = valData[i] ^ AKey[i % AKey.size()];
    return stringToVariant(QString::fromUtf8(valData),valType);
  }
  return QVariant();
}
