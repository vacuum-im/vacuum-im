#ifndef IEMOTICONS_H
#define IEMOTICONS_H

#include <QUrl>
#include <QIcon>
#include <QString>
#include <QStringList>

#define EMOTICONS_UUID "{B22901A6-4CDC-4218-A0C9-831131DDC8BA}"

class IEmoticons
{
public:
  virtual QObject *instance() =0;
  virtual QStringList iconsets() const =0;
  virtual void setIconsets(const QStringList &ASubStorages) =0;
  virtual void insertIconset(const QString &ASubStorage, const QString &ABefour = "") =0;
  virtual void removeIconset(const QString &ASubStorages) =0;
  virtual QUrl urlByKey(const QString &AKey) const =0;
  virtual QString keyByUrl(const QUrl &AUrl) const =0;
signals:
  virtual void iconsetInserted(const QString &ASubStorage, const QString &ABefour) =0;
  virtual void iconsetRemoved(const QString &ASubStorage) =0;
};

Q_DECLARE_INTERFACE(IEmoticons,"Vacuum.Plugin.IEmoticons/1.0")

#endif
