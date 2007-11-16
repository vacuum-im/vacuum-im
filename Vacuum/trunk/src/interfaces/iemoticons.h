#ifndef IEMOTICONS_H
#define IEMOTICONS_H

#define EMOTICONS_UUID "{B22901A6-4CDC-4218-A0C9-831131DDC8BA}"

class IEmoticons 
{
public:
  virtual QObject *instance() =0;
  virtual QStringList iconsets() const =0;
  virtual void setIconsets(const QStringList &AIconsetFiles) =0;
  virtual void insertIconset(const QString &AIconsetFile, const QString &ABefour = "") =0;
  virtual void insertIconset(const QStringList &AIconsetFiles, const QString &ABefour = "") =0;
  virtual void removeIconset(const QString &AIconsetFile) =0;
  virtual void removeIconset(const QStringList &AIconsetFiles) =0;
  virtual QUrl urlByTag(const QString &ATag) const =0;
  virtual QString tagByUrl(const QUrl &AUrl) const =0;
  virtual QIcon iconByTag(const QString &ATag) const =0;
  virtual QIcon iconByUrl(const QUrl &AUrl) const =0;
signals:
  virtual void iconsetInserted(const QString &AIconsetFile, const QString &ABefour) =0;
  virtual void iconsetRemoved(const QString &AIconsetFile) =0;
  virtual void iconsetsChangedBySkin() =0;
  virtual void optionsAccepted() =0;
  virtual void optionsRejected() =0;
};

Q_DECLARE_INTERFACE(IEmoticons,"Vacuum.Plugin.IEmoticons/1.0")

#endif