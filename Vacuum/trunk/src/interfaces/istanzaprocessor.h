#ifndef ISTANZAPROCESSOR_H
#define ISTANZAPROCESSOR_H

#include <QStringList>
#include "../../definations/stanzahandlerpriority.h"
#include "../../utils/jid.h"
#include "../../utils/stanza.h"

#define STANZAPROCESSOR_UUID "{1175D470-5D4A-4c29-A69E-EDA46C2BC387}"

class IStanzaHandler {
public:
  virtual QObject* instance() =0;
  virtual bool editStanza(int AHandlerId, const Jid &AStreamJid, Stanza *AStanza, bool &AAccept) =0;
  virtual bool readStanza(int AHandlerId, const Jid &AStreamJid, const Stanza &AStanza, bool &AAccept) =0;
};

class IIqStanzaOwner {
public:
  virtual QObject* instance() =0;
  virtual void iqStanza(const Jid &AStreamJid, const Stanza &AStanza) =0;
  virtual void iqStanzaTimeOut(const QString &AId) =0;
};

class IStanzaProcessor {
public:
  enum Direction {
    DirectionIn,
    DirectionOut
  };
public:
  virtual QObject *instance() =0;
  virtual QString newId() const =0;
  virtual bool sendStanzaIn(const Jid &AStreamJid, const Stanza &AStanza) =0;
  virtual bool sendStanzaOut(const Jid &AStreamJid, const Stanza &AStanza) =0;
  virtual bool sendIqStanza(IIqStanzaOwner *AIqOwner, const Jid &AStreamJid, const Stanza &AStanza, int ATimeOut) =0;
  virtual QList<int> handlers() const =0;
  virtual int handlerPriority(int AHandlerId) const =0;
  virtual Jid handlerStreamJid(int AHandlerId) const =0;
  virtual int handlerDirection(int AHandlerId) const =0;
  virtual QStringList handlerConditions(int AHandlerId) const =0;
  virtual void appendCondition(int AHandlerId, const QString &ACondition) =0;
  virtual void removeCondition(int AHandlerId, const QString &ACondition) =0;
  virtual int insertHandler(IStanzaHandler *AHandler, const QString &ACondition, int ADirection, 
	  int APriority = SHP_DEFAULT, const Jid &AStreamJid = Jid()) =0;
  virtual void removeHandler(int AHandlerId) =0;
  virtual bool checkStanza(const Stanza &AStanza, const QString &ACondition) const =0;
signals:
  virtual void stanzaSended(const Stanza &AStanza) =0;
  virtual void stanzaReceived(const Stanza &AStanza) =0;
  virtual void handlerInserted(int AHandlerId, IStanzaHandler *AHandler) =0;
  virtual void handlerRemoved(int AHandlerId) =0;
  virtual void conditionAppended(int AHandlerId, const QString &ACondition) =0;
  virtual void conditionRemoved(int AHandlerId, const QString &ACondition) =0;
};

Q_DECLARE_INTERFACE(IStanzaHandler,"Vacuum.Plugin.IStanzaHandler/1.0");
Q_DECLARE_INTERFACE(IIqStanzaOwner,"Vacuum.Plugin.IIqStanzaOwner/1.0");
Q_DECLARE_INTERFACE(IStanzaProcessor,"Vacuum.Plugin.IStanzaProcessor/1.0");

#endif
