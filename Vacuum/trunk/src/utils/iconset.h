#ifndef ICONSET_H
#define ICONSET_H

#include <QList>
#include <QIcon>
#include <QDomDocument>
#include <QDateTime>
#include "utilsexport.h"
#include "unzipfile.h"

struct IconsetInfo
{
  struct Author 
  {
    QString name;
    QString jid;
    QString email;
    QString homePage;
  };
  QString name;
  QString version;
  QString description;
  QString homePage;
  QDateTime creation;
  QList<Author> authors;
};

class IconsetData :
  public QSharedData
{
public:
  IconsetData() {}
  IconsetData(const IconsetData &AOther) : QSharedData(AOther)
  {
    FIconDef = AOther.FIconDef.cloneNode(true).toDocument();
    FInfo = AOther.FInfo;
    FIconFiles = AOther.FIconFiles;
    FIconByFile = AOther.FIconByFile;
    FFileByName = AOther.FFileByName;
    FFileByTagValue = AOther.FFileByTagValue;
    FTagValuesByFile = AOther.FTagValuesByFile;
  }
  ~IconsetData() {}
public:
  QDomDocument FIconDef;
  IconsetInfo FInfo;
  QList<QString> FIconFiles;
  QHash<QString,QIcon> FIconByFile;
  QHash<QString,QString> FFileByName;
  QHash<QString/*Tag*/,QHash<QString/*Value*/,QString/*File*/> > FFileByTagValue;
  QHash<QString/*File*/, QHash<QString/*Tag*/,QList<QString> /*Values*/ > > FTagValuesByFile; 
};

class UTILS_EXPORT Iconset : 
  private UnzipFile
{
public:
  Iconset();
  Iconset(const QString &AFileName);
  ~Iconset();

  bool isValid() const;
  const QString &iconsetFile() const;
  bool openIconset(const QString &AFileName);
  QByteArray fileData(const QString &AFileName) const;
  const QDomDocument &iconDef() const;
  const IconsetInfo &info() const;
  QList<QString> iconFiles() const;
  QList<QString> iconNames() const;
  QList<QString> tags(const QString &AFileName = "") const;
  QList<QString> tagValues(const QString &ATagName, const QString &AFileName = "") const;
  QString fileByIconName(const QString &AIconName) const;
  QString fileByTagValue(const QString &ATag, const QString &AValue) const;
  QIcon iconByFile(const QString &AFileName) const;
  QIcon iconByName(const QString &AIconName) const;
  QIcon iconByTagValue(const QString &ATag, const QString &AValue) const;
protected:
  bool loadIconset();  
private:
  QSharedDataPointer<IconsetData> d;
};

#endif // ICONSET_H
