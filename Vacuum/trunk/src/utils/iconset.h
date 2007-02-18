#ifndef ICONSET_H
#define ICONSET_H

#include <QList>
#include <QIcon>
#include <QDomDocument>
#include <QSharedData>
#include "utilsexport.h"
#include "unzipfile.h"

class IconSetData;

class UTILS_EXPORT IconSet : 
  private UnzipFile
{
public:
  IconSet();
  IconSet(const QString &AFileName);
  ~IconSet();

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
  mutable QSharedDataPointer<IconSetData> d;
};

#endif // ICONSET_H
