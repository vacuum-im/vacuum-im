#ifndef MESSENGER_H
#define MESSENGER_H

#include "../../definations/messagedataroles.h"
#include "../../definations/initorders.h"
#include "../../definations/rosterlabelorders.h"
#include "../../definations/rosterindextyperole.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/imessenger.h"
#include "../../interfaces/ixmppstreams.h"
#include "../../interfaces/istanzaprocessor.h"
#include "../../interfaces/irostersmodel.h"
#include "../../interfaces/irostersview.h"
#include "../../interfaces/itraymanager.h"
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
  Q_INTERFACES(IPlugin IMessenger IStanzaHandler IRostersClickHooker);

public:
  Messenger();
  ~Messenger();

  virtual QObject *instance() { return this; }

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
  virtual int newMessageId() { FMessageId++; return FMessageId; }
  virtual int receiveMessage(const Message &AMessage);
  virtual void showMessage(int AMessageId);
  virtual void removeMessage(int AMessageId);
  virtual QList<int> messages(const Jid &AStreamJid, const Jid &AFromJid = Jid(), const QString &AMesType = "all");
signals:
  virtual void messageReceived(Message &AMessage);
protected:
  void notifyMessage(int AMessageId);
  void unNotifyMessage(int AMessageId);
protected slots:
  void onStreamAdded(IXmppStream *AXmppStream);
  void onStreamJidAboutToBeChanged(IXmppStream *AXmppStream, const Jid &AAfter);
  void onStreamJidChanged(IXmppStream *AXmppStream, const Jid &ABefour);
  void onStreamRemoved(IXmppStream *AXmppStream);
  void onSkinChaged();
  void onRosterLabelDClicked(IRosterIndex *AIndex, int ALabelId, bool &AAccepted);
  void onTrayNotifyActivated(int ANotifyId);
private:
  IXmppStreams *FXmppStreams;
  IStanzaProcessor *FStanzaProcessor;
  IRostersModel *FRostersModel;
  IRostersModelPlugin *FRostersModelPlugin;
  IRostersView *FRostersView;
  IRostersViewPlugin *FRostersViewPlugin;
  ITrayManager *FTrayManager;
private:
  QIcon FMessageIcon;
  SkinIconset FSystemIconset;
private:
  int FMessageId;
  int FIndexClickHooker;
  int FNormalLabelId;
  QMap<int,Message> FMessages;
  QHash<IXmppStream *,HandlerId> FMessageHandlers;
  QHash<int,int> FTrayId2MessageId;
};

#endif // MESSENGER_H
