#include "iconstorage.h"

QHash<QString,int> IconStorage::FIconsetCount;
QHash<QString,Iconset> IconStorage::FAllIconsets;

IconStorage::IconStorage(QObject *AParent) :
  QObject(AParent)
{

}

IconStorage::~IconStorage()
{
  QString fileName;
  foreach(fileName, FMyIconsets)
    closeIconset(fileName);
}

Iconset IconStorage::getIconset(const QString &AFileName)
{
  if (!hasIconset(AFileName))
  {
    Iconset iconSet(AFileName);
    if (iconSet.isValid())
    {
      FAllIconsets.insert(AFileName,iconSet);
      FIconsetCount.insert(AFileName,0);
      FMyIconsets.append(AFileName);
    }
    return iconSet;
  }
  else
  {
    Iconset iconSet = FAllIconsets.value(AFileName);
    if (iconSet.isValid())
      FIconsetCount[AFileName]++;
    return iconSet;
  }
}

void IconStorage::closeIconset(const QString &AFileName)
{
  if (FMyIconsets.contains(AFileName))
  {
    FMyIconsets.removeAt(FMyIconsets.indexOf(AFileName));
    if (FIconsetCount.value(AFileName) <= 1)
    {
      FIconsetCount.remove(AFileName);
      FAllIconsets.remove(AFileName);
    }
    else
      FIconsetCount[AFileName]--;
  }
}

QList<QString> IconStorage::openedIconsets()
{
  return FAllIconsets.keys();
}

bool IconStorage::hasIconset(const QString &AFileName)
{
  return FAllIconsets.contains(AFileName);  
}

