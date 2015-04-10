#ifndef IVCARDMANAGER_H
#define IVCARDMANAGER_H

#include <QHash>
#include <QImage>
#include <QDialog>
#include <QDateTime>
#include <QStringList>
#include <QDomDocument>
#include <utils/jid.h>
#include <utils/xmpperror.h>

#define VCARD_UUID "{8AD31549-AD09-4e84-BD6F-41928B3BDA7E}"

#define VCARD_GENDER_MALE        "Male"
#define VCARD_GENDER_FEMALE      "Female"

class IVCard 
{
public:
	virtual QObject *instance() =0;
	virtual bool isValid() const =0;
	virtual bool isEmpty() const =0;
	virtual Jid contactJid() const =0;
	virtual QDomElement vcardElem() const =0;
	virtual QDateTime loadDateTime() const =0;
	virtual QMultiHash<QString, QStringList> values(const QString &AName, const QStringList &ATagList) const =0;
	virtual QString value(const QString &AName, const QStringList &ATags = QStringList(), const QStringList &ATagList = QStringList()) const =0;
	virtual void setTagsForValue(const QString &AName, const QString &AValue, const QStringList &ATags = QStringList(), const QStringList &ATagList = QStringList()) =0;
	virtual void setValueForTags(const QString &AName, const QString &AValue, const QStringList &ATags = QStringList(), const QStringList &ATagList = QStringList()) =0;
	virtual void clear() = 0;
	virtual bool update(const Jid &AStreamJid) =0;
	virtual bool publish(const Jid &AStreamJid) =0;
	virtual void unlock() =0;
protected:
	virtual void vcardUpdated() =0;
	virtual void vcardPublished() =0;
	virtual void vcardError(const XmppError &AError) =0;
};

class IVCardManager 
{
public:
	virtual QObject *instance() =0;
	virtual QString vcardFileName(const Jid &AContactJid) const =0;
	virtual bool hasVCard(const Jid &AContactJid) const =0;
	virtual bool requestVCard(const Jid &AStreamJid, const Jid &AContactJid) =0;
	virtual IVCard *getVCard(const Jid &AContactJid) =0;
	virtual bool publishVCard(const Jid &AStreamJid, IVCard *AVCard) =0;
	virtual QDialog *showVCardDialog(const Jid &AStreamJid, const Jid &AContactJid, QWidget *AParent = NULL) =0;
protected:
	virtual void vcardReceived(const Jid &AContactJid) =0;
	virtual void vcardPublished(const Jid &AContactJid) =0;
	virtual void vcardError(const Jid &AContactJid, const XmppError &AError) =0;
};

Q_DECLARE_INTERFACE(IVCard,"Vacuum.Plugin.IVCard/1.3")
Q_DECLARE_INTERFACE(IVCardManager,"Vacuum.Plugin.IVCardManager/1.4")

#endif //IVCARDMANAGER_H
