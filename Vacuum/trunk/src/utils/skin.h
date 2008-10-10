#ifndef SKIN_H
#define SKIN_H

#include <QSet>
#include "iconset.h"
#include "soundset.h"
#include "skiniconset.h"
#include "skinsoundset.h"
#include "utilsexport.h"

#define DEFAULT_SKIN_NAME                 "default"

#define SKIN_TYPE_ICONSET                 "iconset"
#define SKIN_TYPE_SOUNDS                  "sounds"

#define SYSTEM_ICONSETFILE                "system/common.jisp"
#define ROSTER_ICONSETFILE                "roster/common.jisp"
#define STATUS_ICONSETFILE                "status/jabber.jisp"
#define EMOTICONS_ICONSETFILE             "emoticons/common.jisp"

class UTILS_EXPORT Skin
{
public:
  static SkinIconset *getSkinIconset(const QString &AFileName);
  static SkinSoundset *getSkinSoundset(const QString &ADirName);
  static Iconset getIconset(const QString &AFileName);
  static Iconset getDefaultIconset(const QString &AFileName);
  static Soundset getSoundset(const QString &ADirName);
  static Soundset getDefaultSoundset(const QString &ADirName);
  static QStringList skins();
  static bool skinFileExists(const QString &ASkinType, const QString &AFileName, const QString &ASubFolder ="", const QString &ASkin = "");
  static QStringList skinFiles(const QString &ASkinType, const QString &ASubFolder, const QString &AFilter = "*.*", const QString &ASkin = "");
  static QStringList skinFilesWithDef(const QString &ASkinType, const QString &ASubFolder, const QString &AFilter = "*.*", const QString &ASkin = "");
  static QIcon skinIcon(const QString &ASkin);
  static const QString &skin();
  static void setSkin(const QString &ASkin);
  static const QString &skinsDirectory();
  static void setSkinsDirectory(const QString &ASkinsDirectory);
protected:
  static void updateSkin();
private:
  static QString FSkin;
  static QString FSkinsDirectory;
  static QHash<QString,Iconset> FIconsets;
  static QHash<QString,Soundset> FSoundsets;
  static QHash<QString,SkinIconset *> FSkinIconsets;
  static QHash<QString,SkinSoundset *> FSkinSoundsets;
};

#endif // SKIN_H
