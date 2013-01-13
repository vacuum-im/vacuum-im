#ifndef IVCARD_H
#define IVCARD_H

#include <QHash>
#include <QImage>
#include <QDateTime>
#include <QStringList>
#include <QDomDocument>
#include <utils/jid.h>

#define VCARD_UUID "{8AD31549-AD09-4e84-BD6F-41928B3BDA7E}"

class IVCard {
public:
  virtual QObject *instance() =0;
  virtual bool isValid() const =0;
  virtual bool isEmpty() const =0;
  virtual const Jid &contactJid() const =0;
  virtual QDomElement vcardElem() const =0;
  virtual QDateTime loadDateTime() const =0;
  virtual QString value(const QString &AName, const QStringList &ATags = QStringList(),
    const QStringList &ATagList = QStringList()) const =0;
  virtual QMultiHash<QString,QStringList> values(const QString &AName, const QStringList &ATagList) const =0;
  virtual void setTagsForValue(const QString &AName, const QString &AValue, const QStringList &ATags = QStringList(),
    const QStringList &ATagList = QStringList()) =0;
  virtual void setValueForTags(const QString &AName, const QString &AValue, const QStringList &ATags = QStringList(),
    const QStringList &ATagList = QStringList()) =0;
  virtual QImage logoImage() const =0;
  virtual void setLogoImage(const QImage &AImage, const QByteArray &AFormat = QByteArray()) =0;
  virtual QImage photoImage() const =0;
  virtual void setPhotoImage(const QImage &AImage, const QByteArray &AFormat = QByteArray()) =0;
  virtual void clear() = 0;
  virtual bool update(const Jid &AStreamJid) =0;
  virtual bool publish(const Jid &AStreamJid) =0;
  virtual void unlock() =0;
protected:
  virtual void vcardUpdated() =0;
  virtual void vcardPublished() =0;
  virtual void vcardError(const QString &AError) =0;
};

class IVCardPlugin {
public:
  virtual QObject *instance() =0;
  virtual QString vcardFileName(const Jid &AContactJid) const =0;
  virtual bool hasVCard(const Jid &AContactJid) const =0;
  virtual bool requestVCard(const Jid &AStreamJid, const Jid &AContactJid) =0;
  virtual IVCard *vcard(const Jid &AContactJid) =0;
  virtual bool publishVCard(IVCard *AVCard, const Jid &AStreamJid) =0;
  virtual void showVCardDialog(const Jid &AStreamJid, const Jid &AContactJid) =0;
protected:
  virtual void vcardReceived(const Jid &AContactJid) =0;
  virtual void vcardPublished(const Jid &AContactJid) =0;
  virtual void vcardError(const Jid &AContactJid, const QString &AError) =0;
};

Q_DECLARE_INTERFACE(IVCard,"Vacuum.Plugin.IVCard/1.0")
Q_DECLARE_INTERFACE(IVCardPlugin,"Vacuum.Plugin.IVCardPlugin/1.0")

#endif
