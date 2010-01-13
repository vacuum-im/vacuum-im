#ifndef STANZAPROCESSOR_H
#define STANZAPROCESSOR_H

#include <QTimer>
#include <QMultiMap>
#include <QDomDocument>
#include <interfaces/ipluginmanager.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/ixmppstreams.h>

struct StanzaRequest 
{
  StanzaRequest() { timer=NULL; owner=NULL; }
  Jid streamJid;
  QTimer *timer;
  IStanzaRequestOwner *owner;
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
  virtual bool sendStanzaRequest(IStanzaRequestOwner *AIqOwner, const Jid &AStreamJid, const Stanza &AStanza, int ATimeout);
  virtual QList<int> stanzaHandles() const;
  virtual IStanzaHandle stanzaHandle(int AHandleId) const;
  virtual int insertStanzaHandle(const IStanzaHandle &AHandle);
  virtual void removeStanzaHandle(int AHandleId);
  virtual bool checkStanza(const Stanza &AStanza, const QString &ACondition) const;
signals:
  void stanzaSent(const Jid &AStreamJid, const Stanza &AStanza);
  void stanzaReceived(const Jid &AStreamJid, const Stanza &AStanza);
  void stanzaHandleInserted(int AHandleId, const IStanzaHandle &AHandle);
  void stanzaHandleRemoved(int AHandleId, const IStanzaHandle &AHandle);
protected:
  virtual bool checkCondition(const QDomElement &AElem, const QString &ACondition, const int APos = 0) const;
  virtual bool processStanzaIn(const Jid &AStreamJid, Stanza &AStanza) const;
  virtual bool processStanzaOut(const Jid &AStreamJid, Stanza &AStanza) const;
  virtual bool processStanzaRequest(const Jid &AStreamJid, const Stanza &AStanza);
  virtual void removeStanzaRequest(const QString &AStanzaId);
protected slots:
  void onStreamElement(IXmppStream *AStream, const QDomElement &AElem);
  void onStreamJidChanged(IXmppStream *AStream, const Jid &ABefour);
  void onStreamClosed(IXmppStream *AStream);
  void onStanzaRequestTimeout();
  void onStanzaRequestOwnerDestroyed(QObject *AOwner);
  void onStanzaHandlerDestroyed(QObject *AHandler);
private:
  IXmppStreams *FXmppStreams;
private:
  QMap<int, IStanzaHandle> FHandles;
  QMap<QString, StanzaRequest> FRequests;
  QMultiMap<int, int> FHandleIdByOrder;
};

#endif
