#ifndef VCARD_H
#define VCARD_H

#include "../../definations/namespaces.h"
#include "../../interfaces/ivcard.h"

#define VCARD_FILE_ROOT_TAGNAME         "VCard"
#define VCARD_TAGNAME                   "vCard"

class VCardPlugin;

class VCard : 
  public QObject,
  public IVCard
{
  Q_OBJECT;
  Q_INTERFACES(IVCard);
public:
  VCard(const Jid &AContactJid, VCardPlugin *APlugin);
  ~VCard();
  virtual QObject *instance() { return this; }
  virtual bool isValid() const { return FContactJid.isValid() && !vcardElem().isNull(); }
  virtual bool isEmpty() const { return !isValid() || !vcardElem().hasChildNodes(); }
  virtual const Jid &contactJid() const { return FContactJid; }
  virtual QDomElement vcardElem() const { return FDoc.documentElement().firstChildElement(VCARD_TAGNAME); }
  virtual QString value(const QString &AName, const QStringList &ATags = QStringList(),
    const QStringList &ATagList = QStringList()) const;
  virtual QMultiHash<QString,QStringList> values(const QString &AName, const QStringList &ATagList) const;
  virtual void setTagsForValue(const QString &AName, const QString &AValue, const QStringList &ATags = QStringList(), 
    const QStringList &ATagList = QStringList());
  virtual void setValueForTags(const QString &AName, const QString &AValue, const QStringList &ATags = QStringList(),
    const QStringList &ATagList = QStringList());
  virtual QImage logoImage() const { return FLogo; }
  virtual void setLogoImage(const QImage &AImage);
  virtual QImage photoImage() const { return FPhoto; }
  virtual void setPhotoImage(const QImage &AImage);
  virtual void clear();
  virtual bool update(const Jid &AStreamJid);
  virtual bool publish(const Jid &AStreamJid);
  virtual void unlock();
signals:
  virtual void vcardUpdated();
  virtual void vcardPublished();
  virtual void vcardError(const QString &AError);
protected:
  void loadVCardFile();
  QDomElement createElementByName(const QString AName, const QStringList &ATags, const QStringList &ATagList);
  QDomElement firstElementByName(const QString AName) const;
  QDomElement nextElementByName(const QString AName, const QDomElement APrevElem) const;
  QDomElement setTextToElem(QDomElement &AElem, const QString &AText) const;
protected slots:
  void onVCardReceived(const Jid &AContactJid);
  void onVCardPublished(const Jid &AContactJid);
  void onVCardError(const Jid &AContactJid, const QString &AError);
private:
  VCardPlugin *FVCardPlugin;
private:
  Jid FContactJid;   
  QDomDocument FDoc;
private:
  QImage FPhoto;
  QImage FLogo;
};

#endif // VCARD_H
