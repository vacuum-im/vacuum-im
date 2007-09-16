#include "iconset.h"

Iconset::Iconset() :
  UnzipFile()
{
  d = new IconsetData;
}

Iconset::Iconset(const QString &AFileName) :
  UnzipFile(AFileName)
{
  d = new IconsetData;
  loadIconDefination();
}

Iconset::~Iconset()
{

}
bool Iconset::openFile(const QString &AFileName)
{
  d->FIconByFile.clear();
  d->FFileByName.clear();
  d->FFileByTagValue.clear();
  UnzipFile::openFile(AFileName);
  return loadIconDefination();
}

bool Iconset::isValid() const 
{ 
  return !d->FIconDef.isNull(); 
}

const QString &Iconset::fileName() const 
{ 
  return zipFileName(); 
}

QByteArray Iconset::fileData(const QString &AFileName) const
{
  return UnzipFile::fileData(AFileName);
}

const QDomDocument Iconset::iconDef() const 
{ 
  return d->FIconDef; 
}

const QString &Iconset::iconsetName() const
{
  return d->FIconsetName;
}

QList<QString> Iconset::iconFiles() const 
{ 
  return d->FIconByFile.keys(); 
}

QList<QString> Iconset::iconNames() const 
{ 
  return d->FFileByName.keys(); 
}

QList<QString> Iconset::tags() const 
{ 
  return  d->FFileByTagValue.keys(); 
}

QList<QString> Iconset::tagValues( const QString &ATagName ) const
{
  return d->FFileByTagValue.value(ATagName).keys();
}

QIcon Iconset::iconByFile( const QString &AFileName ) const
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

QIcon Iconset::iconByName( const QString &AIconName ) const
{
  return iconByFile(d->FFileByName.value(AIconName));
}

QIcon Iconset::iconByTagValue( const QString &ATag, QString &AValue ) const
{
  return iconByFile(d->FFileByTagValue.value(ATag).value(AValue));
}

bool Iconset::loadIconDefination()
{
  if (UnzipFile::isValid() && fileNames().contains("icondef.xml"))
  {
    if (d->FIconDef.setContent(fileData("icondef.xml"),true))
    {
      d->FIconsetName = d->FIconDef.firstChildElement("icondef")
                                   .firstChildElement("meta")
                                   .firstChildElement("name").text();
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

