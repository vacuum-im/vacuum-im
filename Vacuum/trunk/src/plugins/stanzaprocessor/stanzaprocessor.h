#ifndef STANZAPROCESSOR_H
#define STANZAPROCESSOR_H

#include <QHash>
#include <QTimer>
#include <QVector>
#include <QMultiMap>
#include <QDomDocument>
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/istanzaprocessor.h"
#include "../../interfaces/ixmppstreams.h"

struct IqStanzaItem {
  IqStanzaItem() { timer=NULL; owner=NULL; }
	QString stanzaId;
	Jid streamJid;
	QTimer *timer;
	IIqStanzaOwner *owner;
};

struct HandlerItem {
  HandlerItem() { handlerId=0; priority=0; handler=NULL; }
	int handlerId;
	int priority;
  int direction;
	Jid streamJid;
	QStringList conditions;
	IStanzaHandler *handler;
};

class StanzaProcessor :
  public QObject,
  public IPlugin,
  public IStanzaProcessor
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IStanzaProcessor);
public:
  StanzaProcessor();
  ~StanzaProcessor();
  //IPlugin
  virtual QObject* instance() { return this; }
  virtual QUuid pluginUuid() const { return STANZAPROCESSOR_UUID; }
  virtual void pluginInfo(IPluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects() { return true; }
  virtual bool initSettings() { return true; }
  virtual bool startPlugin() { return true; }
  //IStanzaProcessor
  virtual QString newId() const;
  virtual bool sendStanzaIn(const Jid &AStreamJid, const Stanza &AStanza);
  virtual bool sendStanzaOut(const Jid &AStreamJid, const Stanza &AStanza);
  virtual bool sendIqStanza(IIqStanzaOwner *AIqOwner, const Jid &AStreamJid, const Stanza &AStanza, int ATimeOut);
  virtual QList<int> handlers() const;
  virtual int handlerDirection(int AHandlerId) const;
  virtual int handlerPriority(int AHandlerId) const;
  virtual Jid handlerStreamJid(int AHandlerId) const;
  virtual QStringList handlerConditions(int AHandlerId) const;
  virtual void appendCondition(int AHandlerId, const QString &ACondition);
  virtual void removeCondition(int AHandlerId, const QString &ACondition);
  virtual int insertHandler(IStanzaHandler *AHandler, const QString &ACondition, int ADirection, 
    int APriority = SHP_DEFAULT, const Jid &AStreamJid = Jid());
  virtual void removeHandler(int AHandlerId);
  virtual bool checkStanza(const Stanza &AStanza, const QString &ACondition) const;
signals:
  virtual void stanzaSended(const Stanza &AStanza);
  virtual void stanzaReceived(const Stanza &AStanza);
  virtual void handlerInserted(int AHandlerId, IStanzaHandler *AHandler);
  virtual void handlerRemoved(int AHandlerId);
  virtual void conditionAppended(int AHandlerId, const QString &ACondition);
  virtual void conditionRemoved(int AHandlerId, const QString &ACondition);
protected:
  virtual bool checkCondition(const QDomElement &AElem, const QString &ACondition, const int APos = 0) const;
  virtual bool processStanzaIn(const Jid &AStreamJid, Stanza *AStanza) const;
  virtual bool processStanzaOut(const Jid &AStreamJid, Stanza *AStanza) const;
  virtual bool processIqStanza(const Jid &AStreamJid, const Stanza &AStanza);
  virtual void removeIqStanzaItem(const QString &AStanzaId);
protected slots:
  void onStreamElement(IXmppStream *AStream, const QDomElement &AElem);
  void onStreamJidChanged(IXmppStream *AStream, const Jid &ABefour);
  void onIqStanzaTimeOut();
  void onHandlerOwnerDestroyed(QObject *AOwner);
  void onIqStanzaOwnerDestroyed(QObject *AOwner);
private:
  IXmppStreams *FXmppStreams;
private:
  QHash<QString, IqStanzaItem> FIqStanzaItems;
  QHash<int, HandlerItem> FHandlerItems;
  QMultiMap<int,int> FHandlerIdsByPriority;
};

#endif
