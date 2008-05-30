#ifndef ISESSIONNEGOTIATION_H
#define ISESSIONNEGOTIATION_H

#include <QHash>
#include "../interfaces/idataforms.h"
#include "../utils/jid.h"

#define SESSIONNEGOTIATION_UUID       "{D4908366-6204-4199-AFB8-BA0CB4CAC91C}"

#define SESSION_FIELD_ACCEPT          "accept"
#define SESSION_FIELD_CONTINUE        "continue"
#define SESSION_FIELD_RENEGOTIATE     "renegotiate"
#define SESSION_FIELD_TERMINATE       "terminate"
#define SESSION_FIELD_REASON          "reason"

struct IStanzaSession 
{
  enum Status {
    Empty,
    Init,
    Accept,
    Pending,
    Active,
    Continue,
    Terminate,
    Error
  };
  IStanzaSession() { status = Empty; }
  QString sessionId;
  Jid streamJid;
  Jid contactJid;
  int status;
  IDataForm form;
  QString errorCondition;
  QStringList errorFields;
};

class ISessionNegotiator 
{
public:
  enum Result {
    Skip    =0,
    Cancel  =1,
    Wait    =2,
    Manual  =4,
    Auto    =8
  };
public:
  virtual int sessionInit(const IStanzaSession &ASession, IDataForm &ARequest) =0;
  virtual int sessionAccept(const IStanzaSession &ASession, const IDataForm &ARequest, IDataForm &ASubmit) =0;
  virtual int sessionContinue(const IStanzaSession &ASession, const QString &AResource) =0;
  virtual void sessionLocalize(const IStanzaSession &ASession, IDataForm &AForm) =0;
};

class ISessionNegotiation 
{
public:
  virtual QObject *instance() =0;
  virtual IStanzaSession currentSession(const Jid &AStreamJid, const Jid &AContactJid) const =0;
  virtual int initSession(const Jid &AStreamJid, const Jid &AContactJid) =0;
  virtual bool isSuspenedSession(const QString &ASessionId) const =0;
  virtual void resumeSession(const Jid &AStreamJid, const Jid &AContactJid) =0;
  virtual void terminateSession(const Jid &AStreamJid, const Jid &AContactJid) =0;
  virtual void insertNegotiator(ISessionNegotiator *ANegotiator, int AOrder) =0;
  virtual void removeNegotiator(ISessionNegotiator *ANegotiator, int AOrder) =0;
signals:
  virtual void sessionInited(const IStanzaSession &ASession) =0;
  virtual void sessionAccepted(const IStanzaSession &ASession) =0;
  virtual void sessionActivated(const IStanzaSession &ASession) =0;
  virtual void sessionTerminated(const IStanzaSession &ASession) =0;
  virtual void sessionFailed(const IStanzaSession &ASession) =0;
};

Q_DECLARE_INTERFACE(ISessionNegotiator,"Vacuum.Plugin.ISessionNegotiator/1.0")
Q_DECLARE_INTERFACE(ISessionNegotiation,"Vacuum.Plugin.ISessionNegotiation/1.0")

#endif