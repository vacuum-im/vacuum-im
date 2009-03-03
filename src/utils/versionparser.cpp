#include "versionparser.h"

#include <QStringList>

VersionParser::VersionParser(const QString &AVersion)
{
  FMajor = 0;
  FMinor = 0;
  FRelease = 0;
  FBuild = 0;
  QStringList parts = AVersion.split("."); 
  if (parts.count() > 0)
    FMajor = parts[0].toInt();
  if (parts.count() > 1)
    FMinor = parts[1].toInt();
  if (parts.count() > 2)
    FRelease = parts[2].toInt();
  if (parts.count() > 3)
    FBuild = parts[3].toInt();
}

VersionParser::VersionParser(qint16 AMajor, 
                             qint16 AMinor, 
                             qint16 ARelease, 
                             qint16 ABuild)
{
  FMajor = AMajor;
  FMinor = AMinor;
  FRelease = ARelease;
  FBuild = ABuild;
}


VersionParser::~VersionParser()
{

}

qint64 VersionParser::version() const
{
  qint64 ver;
  ver = FMajor;
  ver = (ver << 16) + FMinor;
  ver = (ver << 16) + FRelease;
  ver = (ver << 16) + FBuild;
  return ver;
}

QString VersionParser::toString(Part toPart) const 
{
  if (toPart == MAJOR)
    return QString("%1").arg(FMajor);

  if (toPart == MINOR)
    return QString("%1.%2").arg(FMajor).arg(FMinor); 

  if (toPart == RELEASE)
    return QString("%1.%2.%3").arg(FMajor).arg(FMinor).arg(FRelease); 

  if (toPart == BUILD)
    return QString("%1.%2.%3.%4").arg(FMajor).arg(FMinor).arg(FRelease).arg(FBuild);

  return QString();
}

VersionParser& VersionParser::operator =(const VersionParser &AVersion)
{
  FMajor = AVersion.major();
  FMinor = AVersion.minor();
  FRelease = AVersion.release();
  FBuild = AVersion.build(); 
  return *this;
}

bool VersionParser::operator ==(const VersionParser &AVersion) const
{
  return version() == AVersion.version(); 
}

bool VersionParser::operator !=(const VersionParser &AVersion) const
{
  return version() != AVersion.version(); 
}

bool VersionParser::operator <(const VersionParser &AVersion) const
{
  return version() < AVersion.version(); 
}

bool VersionParser::operator <=(const VersionParser &AVersion) const
{
  return version() <= AVersion.version(); 
}

bool VersionParser::operator >(const VersionParser &AVersion) const
{
  return version() > AVersion.version(); 
}

bool VersionParser::operator >=(const VersionParser &AVersion) const
{
  return version() >= AVersion.version(); 
}
