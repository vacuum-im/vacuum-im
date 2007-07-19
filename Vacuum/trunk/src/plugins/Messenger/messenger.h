#ifndef MESSENGER_H
#define MESSENGER_H

#include "../../definations/initorders.h"
#include "../../definations/rosterlabelorders.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/imessenger.h"
#include "../../interfaces/ixmppstreams.h"
#include "../../interfaces/istanzaprocessor.h"
#include "../../interfaces/irostersmodel.h"
#include "../../interfaces/irostersview.h"
#include "../../interfaces/itraymanager.h"
#include "../../utils/message.h"
#include "../../utils/skin.h"
#include "messagewindow.h"

class Messenger : 
  public QObject,
  public IPlugin,
  public IMessenger,
  public IStanzaHandler,
  public IRostersClickHooker
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IMessenger IRostersClickHooker);

public:
  Messenger();
  ~Messenger();

  virtual QObject* instance() { return this; }

  //IPlugin
  virtual QUuid pluginUuid() const { return MESSENGER_UUID; }
  virtual void pluginInfo(PluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects();
  virtual bool initSettings() { return true; }
  virtual bool startPlugin() { return true; }

  //IStanzaHandler
  virtual bool editStanza(HandlerId, const Jid &, Stanza *, bool &) { return false; }
  virtual bool readStanza(HandlerId AHandlerId, const Jid &AStreamJid, const Stanza &AStanza, bool &AAccept);
  
  //IRostersClickHooker
  virtual bool rosterIndexClicked(IRosterIndex *AIndex, int AHookerId);
  
  //IMessenger

protected:
  void notifyMessage(const Message &AMessage);
signals:
  virtual void messageReceived(Message &AMessage);
  virtual void messageWindowCreated(IMessageWindow *AWindow);
protected slots:
  void onStreamAdded(IXmppStream *AXmppStream);
  void onStreamRemoved(IXmppStream *AXmppStream);
  void onSkinChaged();
private:
  IXmppStreams *FXmppStreams;
  IStanzaProcessor *FStanzaProcessor;
  IRostersModel *FRostersModel;
  IRostersModelPlugin *FRostersModelPlugin;
  IRostersView *FRostersView;
  IRostersViewPlugin *FRostersViewPlugin;
  ITrayManager *FTrayManager;
private:
  SkinIconset FSystemIconset;
private:
  int FNormalLabelId;
  int FIndexClickHooker;
  QHash<IXmppStream *,HandlerId> FMessageHandlers;
};

#endif // MESSENGER_H
