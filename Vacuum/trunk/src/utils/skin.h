#ifndef SKIN_H
#define SKIN_H

#include <QString>
#include <QHash>
#include <QPointer>
#include "utilsexport.h"
#include "iconset.h"

#define SYSTEM_ICONSETFILE "system/common.jisp"
#define ROSTER_ICONSETFILE "roster/common.jisp"
#define STATUS_ICONSETFILE "status/jabber.jisp"

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
  void iconsetChanged();
private:
  QString FFileName;
  Iconset FIconset;
  Iconset FDefIconset;
};

class UTILS_EXPORT Skin
{
  friend class SkinIconset;
public:
  static SkinIconset *getSkinIconset(const QString &AFileName);
  static Iconset getIconset(const QString &AFileName);
  static Iconset getDefaultIconset(const QString &AFileName);
  static QStringList skins();
  static QStringList skinFiles(const QString &ASkinType, const QString &ASubFolder, const QString &AFilter = "*.*", const QString &ASkin = "");
  static QIcon skinIcon(const QString &ASkin);
  static const QString &skin();
  static void setSkin(const QString &ASkin);
  static const QString &skinsDirectory();
  static void setSkinsDirectory(const QString &ASkinsDirectory);
protected:
  static void addSkinIconset(SkinIconset *ASkinIconset);
  static void removeSkinIconset(SkinIconset *ASkinIconset);
  static void updateSkinIconsets();
private:
  static QString FSkinsDirectory;
  static QString FSkin;
  static QHash<QString,Iconset> FIconsets;
  static QHash<QString,SkinIconset *> FSkinIconsets;
  static QList<SkinIconset *> FAllSkinIconsets;
};

#endif // SKIN_H
