#include <QtDebug>
#include "iconset.h"

Iconset::Iconset()
{
  d = new IconsetData;
}

Iconset::Iconset(const QString &AFileName)
{
  d = new IconsetData;
  openFile(AFileName);
}

Iconset::~Iconset()
{

}

bool Iconset::isValid() const 
{ 
  return !d->FIconFiles.isEmpty(); 
}

const QString &Iconset::fileName() const 
{ 
  return zipFileName(); 
}

bool Iconset::openFile(const QString &AFileName)
{
  UnzipFile::openFile(AFileName);
  return loadIconset();
}

QByteArray Iconset::fileData(const QString &AFileName) const
{
  return UnzipFile::fileData(AFileName);
}

const QDomDocument &Iconset::iconDef() const 
{ 
  return d->FIconDef; 
}

const IconsetInfo &Iconset::info() const
{
  return d->FInfo;
}

QList<QString> Iconset::iconFiles() const 
{ 
  return d->FIconFiles; 
}

QList<QString> Iconset::iconNames() const 
{ 
  return d->FFileByName.keys(); 
}

QList<QString> Iconset::tags(const QString &AFileName) const 
{ 
  return AFileName.isEmpty() ? d->FFileByTagValue.keys() : d->FTagValuesByFile.value(AFileName).keys(); 
}

QList<QString> Iconset::tagValues(const QString &ATagName, const QString &AFileName) const
{
  return AFileName.isEmpty() ? d->FFileByTagValue.value(ATagName).keys() : d->FTagValuesByFile.value(AFileName).value(ATagName);
}

QString Iconset::fileByIconName(const QString &AIconName) const
{
  return d->FFileByName.value(AIconName);
}

QString Iconset::fileByTagValue(const QString &ATag, const QString &AValue) const
{
  return d->FFileByTagValue.value(ATag).value(AValue);
}

QIcon Iconset::iconByFile(const QString &AFileName) const
{
  return d->FIconByFile.value(AFileName);
}

QIcon Iconset::iconByName(const QString &AIconName) const
{
  return iconByFile(fileByIconName(AIconName));
}

QIcon Iconset::iconByTagValue(const QString &ATag, const QString &AValue) const
{
  return iconByFile(fileByTagValue(ATag,AValue));
}

bool Iconset::loadIconset()
{
  d->FIconDef.clear();
  d->FIconByFile.clear();
  d->FIconFiles.clear();
  d->FFileByName.clear();
  d->FFileByTagValue.clear();
  d->FTagValuesByFile.clear();

  if (UnzipFile::isValid() && fileNames().contains("icondef.xml"))
  {
    if (d->FIconDef.setContent(fileData("icondef.xml"),true))
    {
      QDomElement elem = d->FIconDef.firstChildElement("icondef").firstChildElement("meta");
      if (!elem.isNull())
      {
        d->FInfo.name = elem.firstChildElement("name").text();
        d->FInfo.version = elem.firstChildElement("version").text();
        d->FInfo.description = elem.firstChildElement("description").text();
        d->FInfo.homePage = elem.firstChildElement("home").text();
        //d->FInfo.creation = QDateTime::fromString(elem.firstChildElement("creation").text());
        for (QDomElement aElem = elem.firstChildElement("author"); !aElem.isNull(); aElem = aElem.nextSiblingElement("author"))
        {
          IconsetInfo::Author author;
          author.name = aElem.text();
          author.email = aElem.attribute("email");
          author.jid = aElem.attribute("jid");
          author.homePage = aElem.attribute("www");
          d->FInfo.authors.append(author);
        }
      }

      elem = d->FIconDef.firstChildElement("icondef").firstChildElement("icon");
      while (!elem.isNull())
      {
        QDomElement objElem = elem.firstChildElement("object");
        if (!objElem.isNull())
        {
          QString mime = objElem.attribute("mime");
          QString file = objElem.text();
          if (mime.startsWith("image") && fileNames().contains(file))
          {
            QPixmap pixmap;
            QByteArray iconData = fileData(file);
            if (pixmap.loadFromData(iconData))
            {
              QIcon icon(pixmap);
              d->FIconFiles.append(file);
              d->FIconByFile.insert(file,icon);
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
                  d->FTagValuesByFile[file][tagName].append(tagValue);
                }
                tagElem = tagElem.nextSiblingElement();
              }
            }
          }
        }
        elem = elem.nextSiblingElement("icon");
      }
    }
  }
  return isValid();
}

