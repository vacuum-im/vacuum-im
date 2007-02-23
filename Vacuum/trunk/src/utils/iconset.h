#ifndef ICONSET_H
#define ICONSET_H

#include <QList>
#include <QIcon>
#include <QDomDocument>
#include "utilsexport.h"
#include "unzipfile.h"

class IconsetData :
  public QSharedData
{
public:
  IconsetData() {}
  IconsetData(const IconsetData &AOther)
  {
    FIconDef = AOther.FIconDef;
    FIconByFile = AOther.FIconByFile;
    FFileByName = AOther.FFileByName;
    FFileByTagValue = AOther.FFileByTagValue;
  }
  ~IconsetData() {}
public:
  QDomDocument FIconDef;
  QHash<QString,QIcon> FIconByFile;
  QHash<QString,QString> FFileByName;
  QHash<QString/*Tag*/,QHash<QString/*Value*/,QString/*File*/> > FFileByTagValue;
};

class UTILS_EXPORT Iconset : 
  private UnzipFile
{
public:
  Iconset();
  Iconset(const QString &AFileName);
  ~Iconset();

  bool openFile(const QString &AFileName);
  bool isValid() const;
  const QString &fileName() const;
  const QDomDocument iconDef() const;
  QList<QString> iconFiles() const;
  QList<QString> iconNames() const;
  QList<QString> tags() const;
  QList<QString> tagValues(const QString &ATagName) const;
  QIcon iconByFile(const QString &AFileName) const;
  QIcon iconByName(const QString &AIconName) const;
  QIcon iconByTagValue(const QString &ATag, QString &AValue) const;
protected:
  bool loadIconDefination();  
private:
  mutable QSharedDataPointer<IconsetData> d;
};

#endif // ICONSET_H
