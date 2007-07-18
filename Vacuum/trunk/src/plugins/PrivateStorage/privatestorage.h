#ifndef PRIVATESTORAGE_H
#define PRIVATESTORAGE_H

#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/iprivatestorage.h"
#include "../../interfaces/istanzaprocessor.h"

class PrivateStorage :
  public QObject,
  public IPlugin,
  public IPrivateStorage,
  public IIqStanzaOwner
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IPrivateStorage);
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
private:
  IStanzaProcessor *FStanzaProcessor;
};

#endif // PRIVATESTORAGE_H
