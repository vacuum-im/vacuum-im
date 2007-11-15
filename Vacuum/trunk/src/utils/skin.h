#ifndef SKIN_H
#define SKIN_H

#include <QString>
#include <QHash>
#include <QPointer>
#include "utilsexport.h"
#include "iconset.h"

#define SKIN_TYPE_ICONSET                 "iconset"

#define SYSTEM_ICONSETFILE                "system/common.jisp"
#define ROSTER_ICONSETFILE                "roster/common.jisp"
#define STATUS_ICONSETFILE                "status/jabber.jisp"
#define EMOTICONS_ICONSETFILE             "emoticons/common.jisp"

class UTILS_EXPORT SkinIconset :
  public QObject
{
  Q_OBJECT;
public:
  SkinIconset(QObject *AParent = NULL);
  SkinIconset(const QString &AFileName, QObject *AParent = NULL);
  ~SkinIconset();
  bool isValid() const;
  const QString &fileName() const;
  bool openFile(const QString &AFileName);
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
private:
  QList<QString> summStringLists(const QList<QString> &AFirst, const QList<QString> &ASecond) const;
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
  static bool skinFileExists(const QString &ASkinType, const QString &AFileName, const QString &ASubFolder ="", const QString &ASkin = "");
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
