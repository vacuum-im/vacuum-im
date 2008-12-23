#include "skinsoundset.h"

#include "skin.h"

SkinSoundset::SkinSoundset(QObject *AParent) : QObject(AParent)
{

}

SkinSoundset::SkinSoundset(const QString &ADirPath, QObject *AParent) : QObject(AParent)
{
  openSoundset(ADirPath);
}

SkinSoundset::~SkinSoundset()
{

}

bool SkinSoundset::isValid() const
{
  return FSoundset.isValid() || FDefSoundset.isValid();
}

QString SkinSoundset::soundsetDir() const
{
  return FDirPath;
}

bool SkinSoundset::openSoundset(const QString &ADirPath)
{
  FDirPath = ADirPath;
  FSoundset = Skin::getSoundset(ADirPath);
  FDefSoundset = Skin::getDefaultSoundset(ADirPath);
  emit soundsetChanged();
  return isValid();
}

QByteArray SkinSoundset::fileData(const QString &ASoundFile) const
{
  QByteArray data = FSoundset.fileData(ASoundFile);
  return data.isEmpty() ? FDefSoundset.fileData(ASoundFile) : data;
}

QList<QString> SkinSoundset::soundFiles() const
{
  return sumStringLists(FSoundset.soundFiles(),FDefSoundset.soundFiles());
}

QList<QString> SkinSoundset::soundNames() const
{
  return sumStringLists(FSoundset.soundNames(),FDefSoundset.soundNames());
}

QString SkinSoundset::soundMimeType(const QString &ASoundFile) const
{
  QString mimeType = FSoundset.soundMimeType(ASoundFile);
  return mimeType.isNull() ? FDefSoundset.soundMimeType(ASoundFile) : mimeType;
}

QString SkinSoundset::soundByName(const QString &ASoundName) const
{
  QString sound = FSoundset.soundByName(ASoundName);
  return sound.isNull() ? FDefSoundset.soundByName(ASoundName) : sound;
}

QString SkinSoundset::soundByFile(const QString &ASoundFile) const
{
  QString sound = FSoundset.soundByFile(ASoundFile);
  return sound.isNull() ? FDefSoundset.soundByFile(ASoundFile) : sound;
}

bool SkinSoundset::playSoundByName(const QString &ASoundName, bool ADefault) const
{
  return FSoundset.playSoundByName(ASoundName,ADefault) || FDefSoundset.playSoundByName(ASoundName,ADefault);
}

bool SkinSoundset::playSoundByFile(const QString &ASoundFile, bool ADefault) const
{
  return FSoundset.playSoundByFile(ASoundFile,ADefault) || FDefSoundset.playSoundByFile(ASoundFile,ADefault);
}

QList<QString> SkinSoundset::sumStringLists(const QList<QString> &AFirst, const QList<QString> &ASecond) const
{
  QList<QString> sum = AFirst;
  foreach(QString item, ASecond)
    if (!sum.contains(item))
      sum.append(item);
  return sum;
}
