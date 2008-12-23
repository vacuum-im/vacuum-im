#ifndef VERSIONPARSER_H
#define VERSIONPARSER_H

#include <QObject>
#include "utilsexport.h"

class UTILS_EXPORT VersionParser 
{
public:
  enum Part {MAJOR, MINOR, RELEASE, BUILD};
  VersionParser(const QString &AVersion);
  VersionParser(qint16 AMajor=0, qint16 AMinor=0, qint16 ARelease=0, qint16 ABuild=0);
  ~VersionParser();
  qint16 major() const { return FMajor; }
  qint16 minor() const { return FMinor; }
  qint16 release() const { return FRelease; }
  qint16 build() const { return FBuild; }
  qint64 version() const;
  QString toString(Part toPart = MINOR) const;
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
