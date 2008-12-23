#include "skin.h"
#include <QtDebug>

QString Skin::FSkinsDirectory = QApplication::applicationDirPath()+"/skin";
QString Skin::FSkin = DEFAULT_SKIN_NAME;
QHash<QString,Iconset> Skin::FIconsets;
QHash<QString,Soundset> Skin::FSoundsets;
QHash<QString,SkinIconset *> Skin::FSkinIconsets;
QHash<QString,SkinSoundset *> Skin::FSkinSoundsets;

//--Skin--
SkinIconset *Skin::getSkinIconset(const QString &AFileName)
{
  SkinIconset *skinIconset = FSkinIconsets.value(AFileName);
  if (skinIconset == NULL)
  {
    skinIconset = new SkinIconset(AFileName);
    FSkinIconsets.insert(AFileName,skinIconset);
  }
  return skinIconset;
}

SkinSoundset *Skin::getSkinSoundset(const QString &ADirName)
{
  SkinSoundset *skinSoundSet = FSkinSoundsets.value(ADirName);
  if (skinSoundSet == NULL)
  {
    skinSoundSet = new SkinSoundset(ADirName);
    FSkinSoundsets.insert(ADirName,skinSoundSet);
  }
  return skinSoundSet;
}

Iconset Skin::getIconset(const QString &AFileName)
{
  QString skinFileName = FSkinsDirectory+"/"+FSkin+"/iconset/"+AFileName;
  if (!FIconsets.contains(skinFileName))
  {
    Iconset iconset(skinFileName);
    if (iconset.isValid())
      FIconsets.insert(skinFileName,iconset);
    return iconset;
  }
  return FIconsets.value(skinFileName);
}

Iconset Skin::getDefaultIconset(const QString &AFileName)
{
  QString skinFileName = FSkinsDirectory+"/"DEFAULT_SKIN_NAME"/iconset/"+AFileName;
  if (!FIconsets.contains(skinFileName))
  {
    Iconset iconset(skinFileName);
    if (iconset.isValid())
      FIconsets.insert(skinFileName,iconset);
    return iconset;
  }
  return FIconsets.value(skinFileName);
}

Soundset Skin::getSoundset(const QString &ADirName)
{
  QString skinDirName = FSkinsDirectory+"/"+FSkin+"/sounds/"+ADirName;
  if (!FSoundsets.contains(skinDirName))
  {
    Soundset soundset(skinDirName);
    if (soundset.isValid())
      FSoundsets.insert(skinDirName,soundset);
    return soundset;
  }
  return FSoundsets.value(skinDirName);
}

Soundset Skin::getDefaultSoundset(const QString &ADirName)
{
  QString skinDirName = FSkinsDirectory+"/"DEFAULT_SKIN_NAME"/sounds/"+ADirName;
  if (!FSoundsets.contains(skinDirName))
  {
    Soundset soundset(skinDirName);
    if (soundset.isValid())
      FSoundsets.insert(skinDirName,soundset);
    return soundset;
  }
  return FSoundsets.value(skinDirName);
}

QStringList Skin::skins()
{
  QDir dir(FSkinsDirectory,"",QDir::Name|QDir::IgnoreCase,QDir::AllDirs|QDir::NoDotAndDotDot);
  return dir.entryList();
}

bool Skin::skinFileExists(const QString &ASkinType, const QString &AFileName, const QString &ASubFolder, const QString &ASkin)
{
  QString fullFileName = FSkinsDirectory+"/"+AFileName;
  if (ASkin.isEmpty())
    fullFileName = FSkinsDirectory+"/"+FSkin+"/"+ASkinType+"/"+ASubFolder+"/"+AFileName;
  else
    fullFileName = FSkinsDirectory+"/"+ASkin+"/"+ASkinType+"/"+ASubFolder+"/"+AFileName;
  return QFile::exists(fullFileName);
}

QStringList Skin::skinFiles(const QString &ASkinType, const QString &ASubFolder, const QString &AFilter, const QString &ASkin)
{
  QString dirPath;
  if (ASkin.isEmpty())
    dirPath = FSkinsDirectory+"/"+FSkin+"/"+ASkinType+"/"+ASubFolder;
  else
    dirPath = FSkinsDirectory+"/"+ASkin+"/"+ASkinType+"/"+ASubFolder;

  QDir dir(dirPath,AFilter,QDir::Name|QDir::IgnoreCase,QDir::Files);
  return dir.entryList();
}

QStringList Skin::skinFilesWithDef(const QString &ASkinType, const QString &ASubFolder, const QString &AFilter, const QString &ASkin)
{
  QStringList files = skinFiles(ASkinType,ASubFolder,AFilter,ASkin);
  if (ASkin != DEFAULT_SKIN_NAME)
    files += (skinFiles(ASkinType,ASubFolder,AFilter,DEFAULT_SKIN_NAME).toSet() - files.toSet()).toList();
  return files;
}

QIcon Skin::skinIcon(const QString &ASkin)
{
  return QIcon(FSkinsDirectory+"/"+ASkin+"/skinicon.png");
}

const QString &Skin::skin()
{
  return FSkin;
}

void Skin::setSkin(const QString &ASkin)
{
  if (FSkin != ASkin)
  {
    FSkin = ASkin;
    updateSkin();
  }
}

const QString &Skin::skinsDirectory()
{
  return FSkinsDirectory;
}

void Skin::setSkinsDirectory(const QString &ASkinsDirectory)
{
  if (FSkinsDirectory!=ASkinsDirectory)
  {
    FSkinsDirectory = ASkinsDirectory;
    updateSkin();
  }
}

void Skin::updateSkin()
{
  FIconsets.clear();
  foreach(SkinIconset *skinIconset,FSkinIconsets)
    skinIconset->openIconset(skinIconset->iconsetFile());

  FSoundsets.clear();
  foreach(SkinSoundset *soundset, FSkinSoundsets)
    soundset->openSoundset(soundset->soundsetDir());
}

