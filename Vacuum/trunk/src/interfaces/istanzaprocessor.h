#ifndef ISTANZAPROCESSOR_H
#define ISTANZAPROCESSOR_H

#include <QStringList>
#include "../../utils/jid.h"
#include "../../utils/stanza.h"

#define STANZAPROCESSOR_UUID "{1175D470-5D4A-4c29-A69E-EDA46C2BC387}"

typedef quint32 HandlerId;
typedef quint32 PriorityId;

class IStanzaHandler {
public:
  virtual QObject* instance() =0;
  virtual bool editStanza(HandlerId AHandlerId, const Jid &AStreamJid, Stanza *AStanza, bool &AAccept) =0;
  virtual bool readStanza(HandlerId AHandlerId, const Jid &AStreamJid, const Stanza &AStanza, bool &AAccept) =0;
};

class IIqStanzaOwner {
public:
  virtual QObject* instance() =0;
  virtual void iqStanza(const Jid &AStreamJid, const Stanza &AStanza) =0;
  virtual void iqStanzaTimeOut(const QString &AId) =0;
};

class IStanzaProcessor {
public:
  enum PriorityLevel {
    PriorityVeryHigh,
    PriorityHigh,
    PriorityMiddle,
    PriorityLow,
    PriorityVeryLow
  };

  enum Direction {
    DirectionIn,
    DirectionOut
  };

  virtual QObject* instance() =0;

  virtual QString newId() const =0;

  virtual bool sendStanzaIn(const Jid &AStreamJid, const Stanza &AStanza) =0;
  virtual bool sendStanzaOut(const Jid &AStreamJid, const Stanza &AStanza) =0;
  virtual bool sendIqStanza(IIqStanzaOwner *, const Jid &AStreamJid, 
    const Stanza &AStanza, qint32 ATimeOut) =0;

  virtual PriorityId setPriority(PriorityLevel ALevel, qint32 AOffset, QObject *AOwner = NULL) =0;
  virtual void removePriority(PriorityId APriorityId) =0;

  virtual HandlerId setHandler(IStanzaHandler *, const QString &ACondition, 
    Direction ADirection, PriorityId APriorityId = 0, const Jid &AStreamJid = Jid()) =0; 
  virtual void addCondition(HandlerId AHandlerId, const QString &ACondition) =0;
  virtual void removeHandler(HandlerId AHandlerId) =0;
};

Q_DECLARE_INTERFACE(IStanzaHandler,"Vacuum.Plugin.IStanzaHandler/1.0");
Q_DECLARE_INTERFACE(IIqStanzaOwner,"Vacuum.Plugin.IIqStanzaOwner/1.0");
Q_DECLARE_INTERFACE(IStanzaProcessor,"Vacuum.Plugin.IStanzaProcessor/1.0");

#endif