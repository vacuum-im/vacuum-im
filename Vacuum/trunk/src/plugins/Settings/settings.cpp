#include <QtDebug>
#include "settings.h"
#include <QStringList>
#include <QRect>
#include <QPoint>

Settings::Settings(const QUuid &APluginId, ISettingsPlugin *ASettingsPlugin)
	: QObject(ASettingsPlugin->instance())
{
  FPluginId = APluginId;
  FSettingsPlugin = ASettingsPlugin;
  updatePluginNode();
}

Settings::~Settings()
{

}

QByteArray Settings::encript(const QString &AValue, const QByteArray &AKey) const
{
  QByteArray cripted = AValue.toUtf8();
  for (int i = 0; i<cripted.size(); ++i)
    cripted[i] = cripted[i] ^ AKey[i % AKey.size()];
  return cripted.toBase64();
}

QString Settings::decript(const QByteArray &AValue, const QByteArray &AKey) const
{
  QByteArray plain = QByteArray::fromBase64(AValue);
  for (int i = 0; i<plain.size(); ++i)
    plain[i] = plain[i] ^ AKey[i % AKey.size()];
  return QString::fromUtf8(plain);
}

bool Settings::isValueNSExists(const QString &AName, const QString &ANameNS) const
{
  return !getElement(AName, ANameNS, false).isNull();
}

bool Settings::isValueExists(const QString &AName) const
{
  return !getElement(AName, "", false).isNull();
}

QVariant Settings::valueNS(const QString &AName, const QString &ANameNS, 
                           const QVariant &ADefault) const
{
  QDomElement elem = getElement(AName,ANameNS,false);
  if (!elem.isNull() && elem.hasAttribute("value"))
  {
    QString strValue = elem.attribute("value");
    QVariant::Type varType = (QVariant::Type)elem.attribute("type",QString::number(QVariant::String)).toInt();
    return stringToVariant(strValue, varType, ADefault);
  }

  return ADefault;
}

QVariant Settings::value(const QString &AName, const QVariant &ADefault) const
{
  return valueNS(AName,"",ADefault);
}

QHash<QString,QVariant> Settings::values(const QString &AName) const
{
  QHash<QString,QVariant> result;

  if (!isSettingsOpened() || AName.isEmpty())
    return result;

  QStringList path = AName.split(':',QString::SkipEmptyParts);
  
  QStringList constPath;
  QStringList varPath;
  int i = 0;
  do
    constPath.append(path[i]);
  while (!path[i++].endsWith("[]") && i<path.count());
  
  while (i<path.count())
  {
    if (path[i].endsWith("[]"))
      path[i].chop(2); 
    varPath.append(path[i++]); 
  }
  
  i = 0;
  QDomElement constElem = FPluginNode;
  while (!constElem.isNull() && i<constPath.count())
  {
    if (constPath[i].endsWith("[]"))
      constPath[i].chop(2); 
    constElem = constElem.firstChildElement(constPath[i++]); 
  }

  while (!constElem.isNull())
  {
    QString NS = constElem.attribute("ns"); 
    
    int i = 0;   
    QDomElement elem = constElem;
    while (!elem.isNull() && i<varPath.count())
      elem = elem.firstChildElement(varPath[i++]);
    
    if (!elem.isNull())
      result.insert(NS,stringToVariant(elem.attribute("value"),
        (QVariant::Type)elem.attribute("type",QString::number(QVariant::String)).toInt(), QVariant()));

    constElem = constElem.nextSiblingElement(constPath.last());  
  }

  return result;
}

ISettings &Settings::setValueNS(const QString &AName, const QString &ANameNS, const QVariant &AValue)
{
  static QList<QVariant::Type> customVariantCasts = QList<QVariant::Type>()
    << QVariant::Rect << QVariant::Point << QVariant::ByteArray << QVariant::StringList;

  QDomElement elem = getElement(AName,ANameNS,true);
  if (!elem.isNull())
  {
    elem.setAttribute("value",variantToString(AValue));  
    if (customVariantCasts.contains(AValue.type()))
      elem.setAttribute("type",QString::number(AValue.type()));
  }
  return *this;
}

ISettings &Settings::setValue(const QString &AName, const QVariant &AValue)
{
  return setValueNS(AName,"",AValue);
}

ISettings &Settings::deleteValueNS(const QString &AName, const QString &ANameNS)
{
  QDomElement elem = getElement(AName,ANameNS,false);  
  if (!elem.isNull())
    elem.parentNode().removeChild(elem);
  return *this;
}

ISettings &Settings::deleteValue(const QString &AName)
{
  return deleteValueNS(AName,"");
}

ISettings &Settings::deleteNS(const QString &ANameNS)
{
  if (isSettingsOpened() && !ANameNS.isEmpty())
    delNSRecurse(ANameNS,FPluginNode); 
  return *this;
}

void Settings::updatePluginNode()
{
  FPluginNode = FSettingsPlugin->pluginNode(FPluginId);
}

QDomElement Settings::getElement(const QString &AName, const QString &ANameNS, 
                                 bool ACreate) const
{
  if (!isSettingsOpened() || AName.isEmpty())
    return QDomElement();

  int i =0;
  bool usedNS = false;
  QDomElement elem = FPluginNode;
  QStringList path = AName.split(':',QString::SkipEmptyParts);
  while (i<path.count())
  {
    bool useNS = path[i].endsWith("[]");
    if (useNS) 
      path[i].chop(2); 
    useNS = !usedNS && (useNS || i==path.count()-1);
    usedNS = usedNS || useNS;

    QDomElement childElem = elem.firstChildElement(path[i]);
    while (!childElem.isNull() && childElem.attribute("ns")  != (useNS ? ANameNS : QString()))
      childElem = childElem.nextSiblingElement(path[i]);  

    if (!ACreate && childElem.isNull())
      return QDomElement();

    if (childElem.isNull())
    {
      childElem = elem.appendChild(elem.ownerDocument().createElement(path[i])).toElement();
      if (useNS && !ANameNS.isEmpty())
        childElem.setAttribute("ns",ANameNS); 
    }
    elem = childElem;
    i++;
  }
  return elem;
}

QString Settings::variantToString(const QVariant &AVariant)
{
  if (AVariant.type() == QVariant::Rect)
  {
    QRect rect = AVariant.toRect();
    return QString("%1::%2::%3::%4").arg(rect.left()).arg(rect.top()).arg(rect.width()).arg(rect.height());
  }
  else if (AVariant.type() == QVariant::Point)
  {
    QPoint point = AVariant.toPoint();
    return QString("%1::%2").arg(point.x()).arg(point.y());
  }
  else if (AVariant.type() == QVariant::ByteArray)
  {
    return qCompress(AVariant.toByteArray()).toBase64();
  }
  else if (AVariant.type() == QVariant::StringList)
  {
    return AVariant.toStringList().join(" || ");
  }
  else
    return AVariant.toString();
}

QVariant Settings::stringToVariant(const QString &AString, QVariant::Type AType, const QVariant &ADefault)
{
  if (AType == QVariant::Rect)
  {
    QList<QString> parts = AString.split("::",QString::SkipEmptyParts);
    if (parts.count() == 4)
      return QRect(parts.at(0).toInt(),parts.at(1).toInt(),parts.at(2).toInt(),parts.at(3).toInt());
    else
      return ADefault;
  }
  else if (AType == QVariant::Point)
  {
    QList<QString> parts = AString.split("::",QString::SkipEmptyParts);
    if (parts.count() == 2)
      return QPoint(parts.at(0).toInt(),parts.at(1).toInt()); 
    else
      return ADefault;
  }
  else if (AType == QVariant::ByteArray)
  {
    QByteArray result = qUncompress(QByteArray::fromBase64(AString.toLatin1()));
    if (!result.isNull()) 
      return result;
    else
     return ADefault;
  }
  else if (AType == QVariant::StringList)
  {
    QStringList list;
    if (!AString.isEmpty())
      AString.split(" || ");
    return list;
  }
  else
    return QVariant(AString);
}

void Settings::delNSRecurse(const QString &ANameNS, QDomElement elem)
{
  while (!elem.isNull())
  {
    if (elem.attribute("ns") == ANameNS)
    {
      QDomElement oldElem = elem;
      elem = elem.nextSiblingElement();
      oldElem.parentNode().removeChild(oldElem); 
    } 
    else 
    { 
      if (elem.hasChildNodes())
        delNSRecurse(ANameNS,elem.firstChildElement()); 
      elem = elem.nextSiblingElement(); 
    }
  }
}

