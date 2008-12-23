#ifndef SKINSOUNDSET_H
#define SKINSOUNDSET_H

#include "soundset.h"
#include "utilsexport.h"

class UTILS_EXPORT SkinSoundset : 
  public QObject
{
  Q_OBJECT;
public:
  SkinSoundset(QObject *AParent = NULL);
  SkinSoundset(const QString &ADirPath, QObject *AParent = NULL);
  ~SkinSoundset();
  bool isValid() const;
  QString soundsetDir() const;
  bool openSoundset(const QString &ADirPath);
  QByteArray fileData(const QString &ASoundFile) const;
  QList<QString> soundFiles() const;
  QList<QString> soundNames() const;
  QString soundMimeType(const QString &ASoundFile) const;
  QString soundByName(const QString &ASoundName) const;
  QString soundByFile(const QString &ASoundFile) const;
  bool playSoundByName(const QString &ASoundName, bool ADefault = false) const;
  bool playSoundByFile(const QString &ASoundFile, bool ADefault = false) const;
signals:
  void soundsetChanged();
private:
  QList<QString> sumStringLists(const QList<QString> &AFirst, const QList<QString> &ASecond) const;
private:
  QString FDirPath;
  Soundset FSoundset;
  Soundset FDefSoundset;
};

#endif // SKINSOUNDSET_H
