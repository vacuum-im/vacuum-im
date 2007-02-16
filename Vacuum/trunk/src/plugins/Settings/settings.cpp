#include "settings.h"
#include <QtDebug>
#include <QStringList>
#include <QRect>
#include <QPoint>

Settings::Settings(const QUuid &AUuid, ISettingsPlugin *ASettingsPlugin, QObject *parent)
	: QObject(parent)
{
  FUuid = AUuid;
  FSettingsPlugin = ASettingsPlugin;
  FSettingsOpened = false;
  connect(FSettingsPlugin->instance(),SIGNAL(profileOpened()),SLOT(onProfileOpened()));
  connect(FSettingsPlugin->instance(),SIGNAL(profileClosed()),SLOT(onProfileClosed()));
}

Settings::~Settings()
{
  qDebug() << "~Settings";
}

QVariant Settings::valueNS(const QString &AName, const QString &ANameNS, 
                           const QVariant &ADefault) const
{
  if (!FSettingsOpened || AName.isEmpty())
    return ADefault;

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

  if (AName.isEmpty())
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
  QDomElement constElem = FSettings;
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

ISettings &Settings::setValueNS(const QString &AName, const QString &ANameNS,
                                        const QVariant &AValue)
{
  if (!FSettingsOpened)
    return *this;
 
  QDomElement elem = getElement(AName,ANameNS,true);
  elem.setAttribute("value",variantToString(AValue));  
  if (!AValue.canConvert(QVariant::String))
    elem.setAttribute("type",QString::number(AValue.type()));

  return *this;
}

ISettings &Settings::setValue(const QString &AName, const QVariant &AValue)
{
  return setValueNS(AName,"",AValue);
}

ISettings &Settings::delValueNS(const QString &AName, const QString &ANameNS)
{
  if (!FSettingsOpened)
    return *this;

  QDomElement elem = getElement(AName,ANameNS,false);  
  if (!elem.isNull())
    elem.parentNode().removeChild(elem);

  return *this;
}

ISettings &Settings::delValue(const QString &AName)
{
  return delValueNS(AName,"");
}

ISettings &Settings::delNS(const QString &ANameNS)
{
  if (!ANameNS.isEmpty())
    delNSRecurse(ANameNS,FSettings); 
  return *this;
}

QDomElement Settings::getElement(const QString &AName, const QString &ANameNS, 
                                 bool ACreate) const
{
  if (!FSettingsOpened)
    return QDomElement();

  int i =0;
  bool usedNS = false;
  QDomElement elem = FSettings;
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

void Settings::delNSRecurse(const QString &ANameNS, QDomNode node)
{
  while (!node.isNull())
  {
    if (node.toElement().attribute("ns") == ANameNS)
    {
      QDomNode tmp = node;
      node = node.nextSibling();
      tmp.parentNode().removeChild(tmp); 
      continue;
    } 
    else if (node.hasChildNodes())
      delNSRecurse(ANameNS,node.firstChild()); 

    node = node.nextSibling(); 
  }
}

QString Settings::variantToString( const QVariant &AVariant )
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
  else
    return AVariant.toString();
}

QVariant Settings::stringToVariant( const QString &AString, QVariant::Type AType, const QVariant &ADefault)
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
  else
    return QVariant(AString);
}

void Settings::onProfileOpened()
{
  FSettings = FSettingsPlugin->getPluginNode(FUuid); 
  if (!FSettings.isNull())
  {
    FSettingsOpened = true;
    emit opened();
  }
}

void Settings::onProfileClosed()
{
  emit closed();
  FSettingsOpened = false;
}

