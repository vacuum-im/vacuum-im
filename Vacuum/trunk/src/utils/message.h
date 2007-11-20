#ifndef MESSAGE_H
#define MESSAGE_H

#include <QSharedData>
#include <QDateTime>
#include <QStringList>
#include <QVariant>
#include "utilsexport.h"
#include "stanza.h"

class MessageData :
  public QSharedData
{
public:
  MessageData() : FStanza("message") 
  {
    FDateTime = QDateTime::currentDateTime();
  };
  MessageData(const Stanza &AStanza) : FStanza(AStanza) 
  {
    FDateTime = QDateTime::currentDateTime();
  };
  MessageData(const MessageData &AOther) : FStanza(AOther.FStanza) 
  {
    FDateTime = AOther.FDateTime;
    FData = AOther.FData;
  };
  ~MessageData() {};
public:
  Stanza FStanza;
  QDateTime FDateTime;
  QHash<int, QVariant> FData;
};


class UTILS_EXPORT Message 
{
public:
  enum MessageType
  {
    AnyType     =0x00,
    Normal      =0x01,
    Chat        =0x02,
    GroupChat   =0x04,
    Headline    =0x08,
    Error       =0x10
  };
public:
  Message();
  Message(const Stanza &AStanza);
  ~Message();

  Stanza &stanza() { return d->FStanza; }
  const Stanza &stanza() const { return d->FStanza; }
  Message &setStanza(const Stanza &AStanza) { d->FStanza = AStanza; return *this; }
  QVariant data(int ARole) const { return d->FData.value(ARole); }
  void setData(int ARole, const QVariant &AData);
  void setData(const QHash<int, QVariant> &AData);
  QString id() const { return d->FStanza.id(); }
  Message &setId(const QString &AId) { d->FStanza.setId(AId); return *this; }
  QString from() const { return d->FStanza.from(); }
  Message &setFrom(const QString &AFrom) { d->FStanza.setFrom(AFrom); return *this; }
  QString to() const { return d->FStanza.to(); }
  Message &setTo(const QString &ATo) { d->FStanza.setTo(ATo); return *this; }
  QString defLang() const { return d->FStanza.lang(); }
  Message &setDefLang(const QString &ALang) { d->FStanza.setLang(ALang); return *this; }
  MessageType type() const;
  Message &setType(MessageType AType);
  QDateTime dateTime() const;
  Message &setDateTime(const QDateTime &ADateTime);
  QStringList subjectLangs() const { return availableLangs(d->FStanza.element(),"subject"); }
  QString subject(const QString &ALang = QString()) const;
  Message &setSubject(const QString &ASubject, const QString &ALang = QString());
  QStringList bodyLangs() const { return availableLangs(d->FStanza.element(),"body"); }
  QString body(const QString &ALang = QString()) const;
  Message &setBody(const QString &ABody, const QString &ALang = QString());
  QString threadId() const;
  Message &setThreadId(const QString &AThreadId);
  QStringList availableLangs(const QDomElement &AParent, const QString &ATagName) const;
  QDomElement findChidByLang(const QDomElement &AParent, const QString &ATagName, const QString &ALang) const;
  QDomElement addChildByLang(const QDomElement &AParent, const QString &ATagName, 
    const QString &ALang, const QString &AText);
protected:
  QDomElement setTextToElem(QDomElement &AElem, const QString &AText);
private:
  QSharedDataPointer<MessageData> d;  
};

#endif // MESSAGE_H
