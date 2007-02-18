#include "iconset.h"
#include <QHash>
#include <QtDebug>

class IconSetData :
  public QSharedData
{
public:
  IconSetData() { qDebug() << "IconsetData create"; }
  IconSetData(const IconSetData &AOther)
  {
    qDebug() << "IconsetData copy";
    FIconDef = AOther.FIconDef;
    FIconByFile = AOther.FIconByFile;
    FFileByName = AOther.FFileByName;
    FFileByTagValue = AOther.FFileByTagValue;
  }
  ~IconSetData() { qDebug() << "IconsetData delete"; }
public:
  QDomDocument FIconDef;
  QHash<QString,QIcon> FIconByFile;
  QHash<QString,QString> FFileByName;
  QHash<QString/*Tag*/,QHash<QString/*Value*/,QString/*File*/> > FFileByTagValue;
};


IconSet::IconSet() :
  UnzipFile()
{
  d = new IconSetData;
}

IconSet::IconSet(const QString &AFileName) :
  UnzipFile(AFileName)
{
  d = new IconSetData;
  loadIconDefination();
}

IconSet::~IconSet()
{

}
bool IconSet::openFile(const QString &AFileName)
{
  d.detach();
  d->FIconByFile.clear();
  d->FFileByName.clear();
  d->FFileByTagValue.clear();
  UnzipFile::openFile(AFileName);
  return loadIconDefination();
}

bool IconSet::isValid() const 
{ 
  return !d->FIconDef.isNull(); 
}

const QString &IconSet::fileName() const 
{ 
  return zipFileName(); 
}

const QDomDocument IconSet::iconDef() const 
{ 
  return d->FIconDef; 
}

QList<QString> IconSet::iconFiles() const 
{ 
  return d->FIconByFile.keys(); 
}

QList<QString> IconSet::iconNames() const 
{ 
  return d->FFileByName.keys(); 
}

QList<QString> IconSet::tags() const 
{ 
  return  d->FFileByTagValue.keys(); 
}

QList<QString> IconSet::tagValues( const QString &ATagName ) const
{
  return d->FFileByTagValue.value(ATagName).keys();
}

QIcon IconSet::iconByFile( const QString &AFileName ) const
{
  if (d->FIconByFile.contains(AFileName))
  {
    QIcon icon = d->FIconByFile.value(AFileName);
    if (icon.isNull())
    {
      QByteArray iconData = fileData(AFileName);
      QPixmap pixmap;
      pixmap.loadFromData(iconData);
      if (!pixmap.isNull())
        icon.addPixmap(pixmap);
      if (!icon.isNull())
        d->FIconByFile.insert(AFileName,icon);
    }
    return icon;
  }
  return QIcon();
}

QIcon IconSet::iconByName( const QString &AIconName ) const
{
  return iconByFile(d->FFileByName.value(AIconName));
}

QIcon IconSet::iconByTagValue( const QString &ATag, QString &AValue ) const
{
  return iconByFile(d->FFileByTagValue.value(ATag).value(AValue));
}

bool IconSet::loadIconDefination()
{
  if (UnzipFile::isValid() && fileNames().contains("icondef.xml"))
  {
    if (d->FIconDef.setContent(fileData("icondef.xml"),true))
    {
      QDomElement elem = d->FIconDef.firstChildElement("icondef").firstChildElement("icon");
      while (!elem.isNull())
      {
        QDomElement objElem = elem.firstChildElement("object");
        if (!objElem.isNull())
        {
          QString mime = objElem.attribute("mime");
          QString file = objElem.text();
          if (mime.startsWith("image/") && fileNames().contains(file))
          {
            d->FIconByFile.insert(file,QIcon());
            QDomElement tagElem = elem.firstChildElement();
            while (!tagElem.isNull())
            {
              QString tagName = tagElem.tagName();
              QString tagValue = tagElem.text();
              if (tagName == "x")
              {
                if (tagElem.namespaceURI() == "name")
                  d->FFileByName.insert(tagValue,file);
              }
              else if (tagName == "name")
              {
                d->FFileByName.insert(tagValue,file);
              }
              else if (tagName != "object")
              {
                d->FFileByTagValue[tagName].insert(tagValue,file);
              }
              tagElem = tagElem.nextSiblingElement();
            }
          }
        }
        elem = elem.nextSiblingElement("icon");
      }
      return true;
    }
  }
  return false;
}

