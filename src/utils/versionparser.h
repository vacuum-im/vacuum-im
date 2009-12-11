#ifndef VERSIONPARSER_H
#define VERSIONPARSER_H

#include <QString>
#include "utilsexport.h"

class UTILS_EXPORT VersionParser 
{
public:
  enum Part {
    MajorVersion, 
    MinorVersion, 
    ReleaseNumber, 
    BuildNumber
  };
public:
  VersionParser(const QString &AVersion);
  VersionParser(qint16 AMajor=0, qint16 AMinor=0, qint16 ARelease=0, qint16 ABuild=0);
  ~VersionParser();
  qint16 majorVersion() const;
  qint16 minorVersion() const;
  qint16 releaseNumber() const;
  qint16 buildNumber() const;
  qint64 versionNumber() const;
  QString toString(Part toPart = MinorVersion) const;
  VersionParser& operator=(const VersionParser &AVersion);
  bool operator ==(const VersionParser &AVersion) const;
  bool operator !=(const VersionParser &AVersion) const;
  bool operator <(const VersionParser &AVersion) const;
  bool operator <=(const VersionParser &AVersion) const;
  bool operator >(const VersionParser &AVersion) const;
  bool operator >=(const VersionParser &AVersion) const;
private:
  qint16 FMajor, FMinor, FRelease, FBuild;
};

#endif // VERSIONPARSER_H
