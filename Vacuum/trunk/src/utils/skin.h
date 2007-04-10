#ifndef SKIN_H
#define SKIN_H

#include <QString>
#include <QHash>
#include <QPointer>
#include "utilsexport.h"
#include "iconset.h"

class UTILS_EXPORT SkinIconset :
  public QObject
{
  Q_OBJECT;
  friend class Skin;

public:
  SkinIconset(QObject *AParent = NULL);
  SkinIconset(const QString &AFileName, QObject *AParent = NULL);
  ~SkinIconset();

  bool openFile(const QString &AFileName);
  const QString &fileName() const;
  bool isValid() const;
  QList<QString> iconFiles() const;
  QList<QString> iconNames() const;
  QList<QString> tags() const;
  QList<QString> tagValues(const QString &ATagName) const;
  QIcon iconByFile(const QString &AFileName) const;
  QIcon iconByName(const QString &AIconName) const;
  QIcon iconByTagValue(const QString &ATag, QString &AValue) const;
signals:
  void skinChanged();
protected:
  void reset();
private:
  QString FFileName;
  Iconset FIconset;
  Iconset FDefIconset;
};

class UTILS_EXPORT Skin
{
public:
  static Iconset getIconset(const QString &AFileName);
  static Iconset getDefIconset(const QString &AFileName);
  static void addSkinIconset(SkinIconset *ASkinIconset);
  static void removeSkinIconset(SkinIconset *ASkinIconset);
  static void setPathToSkins(const QString &APathToSkins);
  static const QString &pathToSkins();
  static void setSkin(const QString &ASkinName);
  static const QString &skin();
protected:
  static void reset();
private:
  static QString FPathToSkins;
  static QString FSkinName;
  static QHash<QString,Iconset> FIconsets;
  static QList<QPointer<SkinIconset> > FSkinIconsets;
};

#endif // SKIN_H
