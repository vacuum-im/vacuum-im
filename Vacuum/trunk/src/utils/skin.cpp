#include "skin.h"

#include <QSet>
#include <QDir>

#define DEFAULT_SKIN_NAME         "default"

QString Skin::FSkinsDirectory = "./skin";
QString Skin::FSkin = DEFAULT_SKIN_NAME;
QHash<QString,Iconset> Skin::FIconsets;
QHash<QString,SkinIconset *> Skin::FSkinIconsets;
QList<SkinIconset *> Skin::FAllSkinIconsets;

//--SkinIconset--
SkinIconset::SkinIconset(QObject *AParent)
  : QObject(AParent)
{
  Skin::addSkinIconset(this);
}

SkinIconset::SkinIconset(const QString &AFileName, QObject *AParent)
  : QObject(AParent)
{
  Skin::addSkinIconset(this);
  openFile(AFileName);
}

SkinIconset::~SkinIconset()
{
  Skin::removeSkinIconset(this);
}

bool SkinIconset::isValid() const
{
  return FIconset.isValid() || FDefIconset.isValid();
}

const QString &SkinIconset::fileName() const
{
  return FFileName;
}

bool SkinIconset::openFile(const QString &AFileName)
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

QList<QString> SkinIconset::iconFiles() const
{
  return summStringLists(FIconset.iconFiles(),FDefIconset.iconFiles());
}

QList<QString> SkinIconset::iconNames() const
{
  return summStringLists(FIconset.iconNames(),FDefIconset.iconNames());
}

QList<QString> SkinIconset::tags(const QString &AFileName) const
{
  return summStringLists(FIconset.tags(AFileName),FDefIconset.tags(AFileName));
}

QList<QString> SkinIconset::tagValues(const QString &ATagName, const QString &AFileName) const
{
  return summStringLists(FIconset.tagValues(ATagName,AFileName),FDefIconset.tagValues(ATagName,AFileName));
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

QList<QString> SkinIconset::summStringLists(const QList<QString> &AFirst, const QList<QString> &ASecond) const
{
  QList<QString> result = AFirst;
  foreach(QString item, ASecond)
    if (!result.contains(item))
      result.append(item);
  return result;
}

//--Skin--
SkinIconset *Skin::getSkinIconset(const QString &AFileName)
{
  SkinIconset *skinIconset = FSkinIconsets.value(AFileName);
  if (!skinIconset)
  {
    skinIconset = new SkinIconset(AFileName);
    FSkinIconsets.insert(AFileName,skinIconset);
  }
  return skinIconset;
}

Iconset Skin::getIconset(const QString &AFileName)
{
  QString skinFileName = FSkin+"/iconset/"+AFileName;
  if (!FIconsets.contains(skinFileName))
  {
    QString fullFileName = FSkinsDirectory+"/"+skinFileName;
    Iconset iconset(fullFileName);
    if (iconset.isValid())
      FIconsets.insert(skinFileName,iconset);
    return iconset;
  }
  else
    return FIconsets.value(skinFileName);
}

Iconset Skin::getDefaultIconset(const QString &AFileName)
{
  QString skinFileName = QString(DEFAULT_SKIN_NAME)+"/iconset/"+AFileName;
  if (!FIconsets.contains(skinFileName))
  {
    QString fullFileName = FSkinsDirectory+"/"+skinFileName;
    Iconset iconset(fullFileName);
    if (iconset.isValid())
      FIconsets.insert(skinFileName,iconset);
    return iconset;
  }
  else
    return FIconsets.value(skinFileName);
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
    updateSkinIconsets();
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
    updateSkinIconsets();
  }
}

void Skin::addSkinIconset(SkinIconset *ASkinIconset)
{
  if (!FAllSkinIconsets.contains(ASkinIconset))
    FAllSkinIconsets.append(ASkinIconset);
}

void Skin::removeSkinIconset(SkinIconset *ASkinIconset)
{
  if (FAllSkinIconsets.contains(ASkinIconset))
    FAllSkinIconsets.removeAt(FAllSkinIconsets.indexOf(ASkinIconset));
}

void Skin::updateSkinIconsets()
{
  FIconsets.clear();
  foreach(SkinIconset *skinIconset,FAllSkinIconsets)
    skinIconset->openFile(skinIconset->fileName());
}

