#ifndef SKIN_H
#define SKIN_H

#include <QString>
#include <QHash>
#include "utilsexport.h"
#include "iconset.h"

class UTILS_EXPORT Skin
{
public:
  Skin();
  ~Skin();

  //Iconset getIconset(const QString &AFileName);
  //void closeIconset(const QString &AFileName);

  static void setPathToSkins(const QString &APathToSkins);
  static const QString &pathToSkins();
  static void setSkin(const QString &ASkinName);
  static const QString &skin();
private:
  static QString FPathToSkins;
  static QString FSkinName;
  static QHash<QString,Iconset> FIconsets;
};

#endif // SKIN_H
