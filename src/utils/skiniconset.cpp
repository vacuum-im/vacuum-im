#include "skiniconset.h"

#include "skin.h"

SkinIconset::SkinIconset(QObject *AParent) : QObject(AParent)
{

}

SkinIconset::SkinIconset(const QString &AFileName, QObject *AParent) : QObject(AParent)
{
  openIconset(AFileName);
}

SkinIconset::~SkinIconset()
{

}

bool SkinIconset::isValid() const
{
  return FIconset.isValid() || FDefIconset.isValid();
}

const QString &SkinIconset::iconsetFile() const
{
  return FFileName;
}

bool SkinIconset::openIconset(const QString &AFileName)
{
  FFileName = AFileName;
  FIconset = Skin::getIconset(AFileName);
  FDefIconset = Skin::getDefaultIconset(AFileName);
  emit iconsetChanged();
  return isValid();
}

QByteArray SkinIconset::fileData(const QString &AFileName) const
{
  QByteArray data = FIconset.fileData(AFileName);
  return data.isEmpty() ? FDefIconset.fileData(AFileName) : data;
}

const IconsetInfo &SkinIconset::info() const
{
  return FIconset.isValid() ? FIconset.info() : FDefIconset.info();
}

QList<QString> SkinIconset::iconFiles() const
{
  return sumStringLists(FIconset.iconFiles(),FDefIconset.iconFiles());
}

QList<QString> SkinIconset::iconNames() const
{
  return sumStringLists(FIconset.iconNames(),FDefIconset.iconNames());
}

QList<QString> SkinIconset::tags(const QString &AFileName) const
{
  return sumStringLists(FIconset.tags(AFileName),FDefIconset.tags(AFileName));
}

QList<QString> SkinIconset::tagValues(const QString &ATagName, const QString &AFileName) const
{
  return sumStringLists(FIconset.tagValues(ATagName,AFileName),FDefIconset.tagValues(ATagName,AFileName));
}

QString SkinIconset::fileByIconName(const QString &AIconName) const
{
  QString file = FIconset.fileByIconName(AIconName);
  return file.isEmpty() ? FDefIconset.fileByIconName(AIconName) : file;
}

QString SkinIconset::fileByTagValue(const QString &ATag, const QString &AValue) const
{
  QString file = FIconset.fileByTagValue(ATag,AValue);
  return file.isEmpty() ? FDefIconset.fileByTagValue(ATag,AValue) : file;
}

QIcon SkinIconset::iconByFile(const QString &AFileName) const
{
  QIcon icon = FIconset.iconByFile(AFileName);
  return icon.isNull() ? FDefIconset.iconByFile(AFileName) : icon;  
}

QIcon SkinIconset::iconByName(const QString &AIconName) const
{
  QIcon icon = FIconset.iconByName(AIconName);
  return icon.isNull() ? FDefIconset.iconByName(AIconName) : icon;  
}

QIcon SkinIconset::iconByTagValue(const QString &ATag, const QString &AValue) const
{
  QIcon icon = FIconset.iconByTagValue(ATag, AValue);
  return icon.isNull() ? FDefIconset.iconByTagValue(ATag, AValue) : icon;  
}

QList<QString> SkinIconset::sumStringLists(const QList<QString> &AFirst, const QList<QString> &ASecond) const
{
  QList<QString> result = AFirst;
  foreach(QString item, ASecond)
    if (!result.contains(item))
      result.append(item);
  return result;
}

