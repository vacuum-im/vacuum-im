#include "soundset.h"

Soundset::Soundset()
{
  d = new SoundsetData;
}

Soundset::Soundset(const QString &ADirPath)
{
  d = new SoundsetData;
  openSoundset(ADirPath);
}

Soundset::~Soundset()
{

}

bool Soundset::isValid() const
{
  return !d->FFileByName.isEmpty();
}

QString Soundset::soundsetDir() const
{
  return d->FDir.absolutePath();
}

bool Soundset::openSoundset(const QString &ADirPath)
{
  d->FDir.setPath(ADirPath);
  return loadSoundset();
}

QByteArray Soundset::fileData(const QString &ASoundFile) const
{
  QFile file(d->FDir.absoluteFilePath(ASoundFile));
  file.open(QFile::ReadOnly);
  return file.readAll();
}

const QDomDocument &Soundset::soundDef() const
{
  return d->FSoundDef;
}

const SoundsetInfo &Soundset::info() const
{
  return d->FInfo;
}

QList<QString> Soundset::soundFiles() const
{
  return d->FMimeByFile.keys();
}

QList<QString> Soundset::soundNames() const
{
  return d->FFileByName.keys();
}

QString Soundset::soundMimeType(const QString &ASoundFile) const
{
  return d->FMimeByFile.value(ASoundFile);
}

QString Soundset::soundByName(const QString &ASoundName) const
{
  return d->FDir.absoluteFilePath(d->FFileByName.value(ASoundName));
}

QString Soundset::soundByFile(const QString &ASoundFile) const
{
  return d->FDir.absoluteFilePath(ASoundFile);
}

bool Soundset::playSoundByName(const QString &ASoundName, bool ADefault) const
{
  QString soundFile = soundByName(ASoundName);
  if (ADefault && soundFile.isEmpty())
    soundFile = soundByName(SN_DEFAULT);
  if (!soundFile.isEmpty())
  {
    QSound::play(soundFile);
    return true;
  }
  return false;
}

bool Soundset::playSoundByFile(const QString &ASoundFile, bool ADefault) const
{
  QString soundFile = soundByFile(ASoundFile);
  if (ADefault && soundFile.isEmpty())
    soundFile = soundByName(SN_DEFAULT);
  if (!soundFile.isEmpty())
  {
    QSound::play(soundFile);
    return true;
  }
  return false;
}

bool Soundset::loadSoundset()
{
  d->FSoundDef.clear();
  d->FFileByName.clear();
  d->FMimeByFile.clear();

  if (d->FDir.exists(SOUNDSET_DEFINATION_FILE))
  {
    if (d->FSoundDef.setContent(fileData(SOUNDSET_DEFINATION_FILE),true))
    {
      QDomElement metaElem = d->FSoundDef.firstChildElement("sounddef").firstChildElement("meta");
      d->FInfo.name = metaElem.firstChildElement("name").text();
      d->FInfo.version = metaElem.firstChildElement("version").text();
      d->FInfo.description = metaElem.firstChildElement("description").text();
      d->FInfo.homePage = metaElem.firstChildElement("home").text();
      //d->FInfo.creation = QDateTime::fromString(metaElem.firstChildElement("creation").text());
      d->FInfo.authors.clear();
      QDomElement authorElem = metaElem.firstChildElement("author");
      while(!authorElem.isNull())
      {
        SoundsetInfo::Author author;
        author.name = authorElem.text();
        author.email = authorElem.attribute("email");
        author.jid = authorElem.attribute("jid");
        author.homePage = authorElem.attribute("www");
        d->FInfo.authors.append(author);
        authorElem = authorElem.nextSiblingElement("author");
      }

      QDomElement soundElem = d->FSoundDef.firstChildElement("sounddef").firstChildElement("sound");
      while (!soundElem.isNull())
      {
        QDomElement objElem = soundElem.firstChildElement("object");
        if (!objElem.isNull())
        {
          QString file = objElem.text();
          QString mime = objElem.attribute("mime");
          if (d->FDir.exists(file))
          {
            d->FMimeByFile.insert(file,mime);
            QDomElement tagElem = soundElem.firstChildElement();
            while (!tagElem.isNull())
            {
              QString tagName = tagElem.tagName();
              QString tagValue = tagElem.text();
              if (tagName == "x")
              {
                if (tagElem.namespaceURI() == "name")
                  d->FFileByName.insert(tagValue,file);
              }
              else if (tagName == "name")
              {
                d->FFileByName.insert(tagValue,file);
              }
              tagElem = tagElem.nextSiblingElement();
            }
          }
        }
        soundElem = soundElem.nextSiblingElement("sound");
      }
    }
  }
  return isValid();
}

