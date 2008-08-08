#ifndef SKINICONSET_H
#define SKINICONSET_H

#include "iconset.h"
#include "utilsexport.h"

class UTILS_EXPORT SkinIconset :
  public QObject
{
  Q_OBJECT;
public:
  SkinIconset(QObject *AParent = NULL);
  SkinIconset(const QString &AFileName, QObject *AParent = NULL);
  ~SkinIconset();
  bool isValid() const;
  const QString &iconsetFile() const;
  bool openIconset(const QString &AFileName);
  QByteArray fileData(const QString &AFileName) const;
  const IconsetInfo &info() const;
  QList<QString> iconFiles() const;
  QList<QString> iconNames() const;
  QList<QString> tags(const QString &AFileName = "") const;
  QList<QString> tagValues(const QString &ATagName, const QString &AFileName = "") const;
  QString fileByIconName(const QString &AIconName) const;
  QString fileByTagValue(const QString &ATag, const QString &AValue) const;
  QIcon iconByFile(const QString &AFileName) const;
  QIcon iconByName(const QString &AIconName) const;
  QIcon iconByTagValue(const QString &ATag, const QString &AValue) const;
signals:
  void iconsetChanged();
private:
  QList<QString> sumStringLists(const QList<QString> &AFirst, const QList<QString> &ASecond) const;
private:
  QString FFileName;
  Iconset FIconset;
  Iconset FDefIconset;
};


#endif // SKINICONSET_H
