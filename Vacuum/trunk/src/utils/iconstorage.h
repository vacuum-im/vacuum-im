#ifndef ICONSTORAGE_H
#define ICONSTORAGE_H

#include <QObject>
#include <QHash>
#include "utilsexport.h"
#include "iconset.h"

class UTILS_EXPORT IconStorage : 
  public QObject
{
  Q_OBJECT;

public:
  IconStorage(QObject *AParent = NULL);
  ~IconStorage();
  
  Iconset getIconset(const QString &AFileName);
  void closeIconset(const QString &AFileName);

  static QList<QString> openedIconsets();
  static bool hasIconset(const QString &AFileName);
private:
  QList<QString> FMyIconsets;
  static QHash<QString,int> FIconsetCount;
  static QHash<QString,Iconset> FAllIconsets;
};

#endif // ICONSTORAGE_H
