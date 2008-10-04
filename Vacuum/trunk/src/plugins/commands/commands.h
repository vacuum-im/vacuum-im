#ifndef COMMANDS_H
#define COMMANDS_H

#include "../../definations/namespaces.h"
#include "../../definations/discofeatureorder.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/icommands.h"
#include "../../interfaces/idataforms.h"
#include "../../interfaces/istanzaprocessor.h"
#include "../../interfaces/iservicediscovery.h"
#include "../../interfaces/ipresence.h"
#include "../../utils/errorhandler.h"
#include "../../utils/menu.h"
#include "commanddialog.h"

class Commands : 
  public QObject,
  public IPlugin,
  public ICommands,
  public IStanzaHandler,
  public IIqStanzaOwner,
  public IDiscoHandler,
  public IDiscoFeatureHandler
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin ICommands IStanzaHandler IIqStanzaOwner IDiscoHandler IDiscoFeatureHandler);
public:
  Commands();
  ~Commands();
  virtual QObject *instance() { return this; }
  //IPlugin
  virtual QUuid pluginUuid() const { return COMMANDS_UUID; }
  virtual void pluginInfo(IPluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects();
  virtual bool initSettings() { return true; }
  virtual bool startPlugin() { return true; }
  //IStanzaHandler
  virtual bool editStanza(int /*AHandlerId*/, const Jid &/*AStreamJid*/, Stanza * /*AStanza*/, bool &/*AAccept*/) { return false; }
  virtual bool readStanza(int AHandlerId, const Jid &AStreamJid, const Stanza &AStanza, bool &AAccept);
  //IIqStanzaOwner
  virtual void iqStanza(const Jid &AStreamJid, const Stanza &AStanza);
  virtual void iqStanzaTimeOut(const QString &AId);
  //IDiscoHandler
  virtual void fillDiscoInfo(IDiscoInfo &ADiscoInfo);
  virtual void fillDiscoItems(IDiscoItems &ADiscoItems);
  //IDiscoFeatureHandler
  virtual bool execDiscoFeature(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo);
  virtual Action *createDiscoFeatureAction(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo, QWidget *AParent);
  //ICommands
  virtual void insertCommand(const QString &ANode, ICommandServer *AServer);
  virtual QList<QString> commandNodes() const { return FCommands.keys(); }
  virtual ICommandServer *commandServer(const QString &ANode) const { return FCommands.value(ANode); }
  virtual void removeCommand(const QString &ANode);
  virtual void insertCommandClient(ICommandClient *AClient);
  virtual void removeCommandClient(ICommandClient *AClient);
  virtual QString sendCommandRequest(const ICommandRequest &ARequest);
  virtual bool sendCommandResult(const ICommandResult &AResult);
  virtual void executeCommnad(const Jid &AStreamJid, const Jid &ACommandJid, const QString &ANode);
signals:
  virtual void commandInserted(const QString &ANode, ICommandServer *AServer);
  virtual void commandRemoved(const QString &ANode);
  virtual void clientInserted(ICommandClient *AClient);
  virtual void clientRemoved(ICommandClient *AClient);
protected:
  void registerDiscoFeatures();
protected slots:
  void onExecuteActionTriggered(bool);
  void onDiscoInfoReceived(const IDiscoInfo &AInfo);
  void onPresenceAdded(IPresence *APresence);
  void onPresenceRemoved(IPresence *APresence);
private:
  IDataForms *FDataForms;
  IStanzaProcessor *FStanzaProcessor;
  IServiceDiscovery *FDiscovery;
  IPresencePlugin *FPresencePlugin;
private:
  QList<QString> FRequests;
  QHash<int,IPresence*> FStanzaHandlers;
  QList<ICommandClient *> FClients;
  QHash<QString,ICommandServer *> FCommands;
};

#endif // COMMANDS_H
