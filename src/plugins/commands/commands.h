#ifndef COMMANDS_H
#define COMMANDS_H

#include <definations/namespaces.h>
#include <definations/discofeaturehandlerorders.h>
#include <definations/resources.h>
#include <definations/menuicons.h>
#include <definations/xmppurihandlerorders.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/icommands.h>
#include <interfaces/idataforms.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/iservicediscovery.h>
#include <interfaces/ipresence.h>
#include <interfaces/ixmppuriqueries.h>
#include <utils/errorhandler.h>
#include <utils/menu.h>
#include "commanddialog.h"

class Commands : 
  public QObject,
  public IPlugin,
  public ICommands,
  public IStanzaHandler,
  public IStanzaRequestOwner,
  public IXmppUriHandler,
  public IDiscoHandler,
  public IDiscoFeatureHandler
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin ICommands IStanzaHandler IStanzaRequestOwner IXmppUriHandler IDiscoHandler IDiscoFeatureHandler);
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
  virtual bool stanzaEdit(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept);
  virtual bool stanzaRead(int AHandlerId, const Jid &AStreamJid, const Stanza &AStanza, bool &AAccept);
  //IXmppUriHandler
  virtual bool xmppUriOpen(const Jid &AStreamJid, const Jid &AContactJid, const QString &AAction, const QMultiMap<QString, QString> &AParams);
  //IStanzaRequestOwner
  virtual void stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza);
  virtual void stanzaRequestTimeout(const Jid &AStreamJid, const QString &AStanzaId);
  //IDiscoHandler
  virtual void fillDiscoInfo(IDiscoInfo &ADiscoInfo);
  virtual void fillDiscoItems(IDiscoItems &ADiscoItems);
  //IDiscoFeatureHandler
  virtual bool execDiscoFeature(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo);
  virtual Action *createDiscoFeatureAction(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo, QWidget *AParent);
  //ICommands
  virtual void insertCommand(const QString &ANode, ICommandServer *AServer);
  virtual QList<QString> commandNodes() const;
  virtual ICommandServer *commandServer(const QString &ANode) const;
  virtual void removeCommand(const QString &ANode);
  virtual void insertCommandClient(ICommandClient *AClient);
  virtual void removeCommandClient(ICommandClient *AClient);
  virtual QString sendCommandRequest(const ICommandRequest &ARequest);
  virtual bool sendCommandResult(const ICommandResult &AResult);
  virtual bool executeCommnad(const Jid &AStreamJid, const Jid &ACommandJid, const QString &ANode);
signals:
  virtual void commandInserted(const QString &ANode, ICommandServer *AServer);
  virtual void commandRemoved(const QString &ANode);
  virtual void clientInserted(ICommandClient *AClient);
  virtual void clientRemoved(ICommandClient *AClient);
protected:
  void registerDiscoFeatures();
protected slots:
  void onExecuteActionTriggered(bool);
  void onRequestActionTriggered(bool);
  void onDiscoInfoReceived(const IDiscoInfo &AInfo);
  void onDiscoInfoRemoved(const IDiscoInfo &AInfo);
  void onPresenceAdded(IPresence *APresence);
  void onContactStateChanged(const Jid &AStreamJid, const Jid &AContactJid, bool AStateOnline);
  void onPresenceRemoved(IPresence *APresence);
private:
  IDataForms *FDataForms;
  IStanzaProcessor *FStanzaProcessor;
  IServiceDiscovery *FDiscovery;
  IPresencePlugin *FPresencePlugin;
  IXmppUriQueries *FXmppUriQueries;
private:
  QList<QString> FRequests;
  QHash<int,IPresence*> FSHICommands;
  QList<ICommandClient *> FClients;
  QHash<QString,ICommandServer *> FCommands;
};

#endif // COMMANDS_H
