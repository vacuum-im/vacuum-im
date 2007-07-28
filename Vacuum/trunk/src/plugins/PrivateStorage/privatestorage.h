#ifndef PRIVATESTORAGE_H
#define PRIVATESTORAGE_H

#include "../../definations/namespaces.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/iprivatestorage.h"
#include "../../interfaces/istanzaprocessor.h"
#include "../../utils/stanza.h"

class PrivateStorage :
  public QObject,
  public IPlugin,
  public IPrivateStorage,
  public IIqStanzaOwner
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IPrivateStorage IIqStanzaOwner);

public:
  PrivateStorage();
  ~PrivateStorage();

  virtual QObject *instance() { return this; }

  //IPlugin
  virtual QUuid pluginUuid() const { return PRIVATESTORAGE_UUID; }
  virtual void pluginInfo(PluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects() { return true; }
  virtual bool initSettings() { return true; }
  virtual bool startPlugin() { return true; }

  //IIqStanzaOwner
  virtual void iqStanza(const Jid &AStreamJid, const Stanza &AStanza);
  virtual void iqStanzaTimeOut(const QString &AId);

  //IPrivateStorage
  virtual int saveElement(const Jid &AStreamJid, const QDomElement &AElement);
  virtual int loadElement(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace, 
    bool ACheckCash = true);
  virtual int removeElement(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace);
  virtual QDomElement getFromCash(const QString &ATagName, const QString &ANamespace) const;
signals:
  virtual void requestCompleted(int ARequestId);
  virtual void requestError(int ARequestId);
  virtual void requestTimeout(int ARequestId);
protected slots:
  void emitCashedRequests();
private:
  IStanzaProcessor *FStanzaProcessor;
private:
  int FRequestId;
  QDomDocument FCash;
  QDomElement FChashElem;
  QList<int> FCashedRequests;
  QHash<QString,int> FIq2Request;
};

#endif // PRIVATESTORAGE_H
