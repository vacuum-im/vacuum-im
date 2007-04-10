#include "skin.h"
#include <QSet>

QString Skin::FPathToSkins = "../../skin";
QString Skin::FSkinName = "default";
QHash<QString,Iconset> Skin::FIconsets;
QList<QPointer<SkinIconset> > Skin::FSkinIconsets;

//--SkinIconset--
SkinIconset::SkinIconset(QObject *AParent) :
  QObject(AParent)
{
  Skin::addSkinIconset(this);
}

SkinIconset::SkinIconset(const QString &AFileName, QObject *AParent) :
  QObject(AParent)
{
  Skin::addSkinIconset(this);
  openFile(AFileName);
}

SkinIconset::~SkinIconset()
{
  Skin::removeSkinIconset(this);
}

bool SkinIconset::openFile(const QString &AFileName)
{
  FFileName = AFileName;
  FIconset = Skin::getIconset(AFileName);
  FDefIconset = Skin::getDefIconset(AFileName);
  return isValid();
}

const QString &SkinIconset::fileName() const
{
  return FFileName;
}

bool SkinIconset::isValid() const
{
  return FIconset.isValid() || FDefIconset.isValid();
}

QList<QString> SkinIconset::iconFiles() const
{
  return FIconset.iconFiles().toSet().unite(FDefIconset.iconFiles().toSet()).toList() ;
}

QList<QString> SkinIconset::iconNames() const
{
  return FIconset.iconNames().toSet().unite(FDefIconset.iconNames().toSet()).toList() ;
}

QList<QString> SkinIconset::tags() const
{
  return FIconset.tags().toSet().unite(FDefIconset.tags().toSet()).toList() ;
}

QList<QString> SkinIconset::tagValues(const QString &ATagName) const
{
  return FIconset.tagValues(ATagName).toSet().unite(FDefIconset.tagValues(ATagName).toSet()).toList() ;
}

QIcon SkinIconset::iconByFile(const QString &AFileName) const
{
  QIcon icon = FIconset.iconByFile(AFileName);
  if (icon.isNull())
    return FDefIconset.iconByFile(AFileName);
  return icon;  
}

QIcon SkinIconset::iconByName(const QString &AIconName) const
{
  QIcon icon = FIconset.iconByName(AIconName);
  if (icon.isNull())
    return FDefIconset.iconByName(AIconName);
  return icon;  
}

QIcon SkinIconset::iconByTagValue(const QString &ATag, QString &AValue) const
{
  QIcon icon = FIconset.iconByTagValue(ATag, AValue);
  if (icon.isNull())
    return FDefIconset.iconByTagValue(ATag, AValue);
  return icon;  
}

void SkinIconset::reset() 
{
  openFile(FFileName);
  emit skinChanged();
}


//--Skin--
Iconset Skin::getIconset(const QString &AFileName)
{
  QString fullFileName = FPathToSkins;
  if (!fullFileName.endsWith('/'))
    fullFileName.append('/');
  
  fullFileName+=FSkinName;
  if (!fullFileName.endsWith('/'))
    fullFileName.append('/');
  
  fullFileName+="iconset/"+AFileName;

  if (!FIconsets.contains(fullFileName))
  {
    Iconset iconset(fullFileName);
    if (iconset.isValid())
      FIconsets.insert(fullFileName,iconset);
    return iconset;
  }
  else
    return FIconsets.value(fullFileName);
}

Iconset Skin::getDefIconset(const QString &AFileName)
{
  QString fullFileName = FPathToSkins;
  if (!fullFileName.endsWith('/'))
    fullFileName.append('/');
  
  fullFileName+="default/iconset/"+AFileName;

  if (!FIconsets.contains(fullFileName))
  {
    Iconset iconset(fullFileName);
    if (iconset.isValid())
      FIconsets.insert(fullFileName,iconset);
    return iconset;
  }
  else
    return FIconsets.value(fullFileName);
}

void Skin::addSkinIconset(SkinIconset *ASkinIconset)
{
  if (!FSkinIconsets.contains(ASkinIconset))
    FSkinIconsets.append(ASkinIconset);
}

void Skin::removeSkinIconset(SkinIconset *ASkinIconset)
{
  if (FSkinIconsets.contains(ASkinIconset))
    FSkinIconsets.removeAt(FSkinIconsets.indexOf(ASkinIconset));
}

void Skin::setPathToSkins(const QString &APathToSkins)
{
  if (FPathToSkins != APathToSkins)
  {
    FPathToSkins = APathToSkins;
    reset();
  }
}

const QString &Skin::pathToSkins()
{
  return FPathToSkins;
}

void Skin::setSkin(const QString &ASkinName)
{
  if (FSkinName != ASkinName)
  {
    FSkinName = ASkinName;
    reset();
  }
}

const QString &Skin::skin()
{
  return FSkinName;
}

void Skin::reset()
{
  FIconsets.clear();

  QPointer<SkinIconset> skinIconset;
  foreach(skinIconset,FSkinIconsets)
    //if (!skinIconset.isNull())
      skinIconset->reset();
}
