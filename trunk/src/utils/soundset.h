#ifndef SOUNDSET_H
#define SOUNDSET_H

#include <QDir>
#include <QHash>
#include <QSound>
#include <QDateTime>
#include <QSharedData>
#include <QDomDocument>
#include "utilsexport.h"

#define SN_DEFAULT                        "default"
#define SOUNDSET_DEFINATION_FILE          "sounddef.xml"

struct SoundsetInfo
{
  struct Author 
  {
    QString name;
    QString jid;
    QString email;
    QString homePage;
  };
  QString name;
  QString version;
  QString description;
  QString homePage;
  QDateTime creation;
  QList<Author> authors;
};

class SoundsetData :
  public QSharedData
{
public:
  SoundsetData() {}
  SoundsetData(const SoundsetData &AOther) : QSharedData(AOther)
  {
    FDir = AOther.FDir;
    FInfo = AOther.FInfo;
    FSoundDef = AOther.FSoundDef.cloneNode(true).toDocument();
    FFileByName = AOther.FFileByName;
    FMimeByFile = AOther.FMimeByFile;
  }
  ~SoundsetData() {}
public:
  QDir FDir;
  SoundsetInfo FInfo;
  QDomDocument FSoundDef;
  QHash<QString,QString> FFileByName;
  QHash<QString,QString> FMimeByFile;
};

class UTILS_EXPORT Soundset
{
public:
  Soundset();
  Soundset(const QString &ADirPath);
  ~Soundset();
  bool isValid() const;
  QString soundsetDir() const;
  bool openSoundset(const QString &ADirPath);
  QByteArray fileData(const QString &ASoundFile) const;
  const QDomDocument &soundDef() const;
  const SoundsetInfo &info() const;
  QList<QString> soundFiles() const;
  QList<QString> soundNames() const;
  QString soundMimeType(const QString &ASoundFile) const;
  QString soundByName(const QString &ASoundName) const;
  QString soundByFile(const QString &ASoundFile) const;
  bool playSoundByName(const QString &ASoundName, bool ADefault = false) const;
  bool playSoundByFile(const QString &ASoundFile, bool ADefault = false) const;
protected:
  bool loadSoundset();  
private:
  QSharedDataPointer<SoundsetData> d;
};

#endif // SOUNDSET_H
