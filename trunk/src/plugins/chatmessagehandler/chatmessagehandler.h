#ifndef CHATMESSAGEHANDLER_H
#define CHATMESSAGEHANDLER_H

#define CHATMESSAGEHANDLER_UUID "{b60cc0e4-8006-4909-b926-fcb3cbc506f0}"

#include "../../definations/messagehandlerorders.h"
#include "../../definations/rosterindextyperole.h"
#include "../../definations/rosterlabelorders.h"
#include "../../definations/rosterclickhookerorders.h"
#include "../../definations/notificationdataroles.h"
#include "../../definations/vcardvaluenames.h"
#include "../../definations/actiongroups.h"
#include "../../definations/resources.h"
#include "../../definations/menuicons.h"
#include "../../definations/soundfiles.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/imessageprocessor.h"
#include "../../interfaces/imessagewidgets.h"
#include "../../interfaces/imessagearchiver.h"
#include "../../interfaces/inotifications.h"
#include "../../interfaces/istatusicons.h"
#include "../../interfaces/irostersview.h"
#include "../../interfaces/ipresence.h"
#include "../../interfaces/ivcard.h"

class ChatMessageHandler : 
  public QObject,
  public IPlugin,
  public IMessageHandler,
  public IRostersClickHooker
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IMessageHandler IRostersClickHooker);
public:
  ChatMessageHandler();
  ~ChatMessageHandler();
  //IPlugin
  virtual QObject *instance() { return this; }
  virtual QUuid pluginUuid() const { return CHATMESSAGEHANDLER_UUID; }
  virtual void pluginInfo(IPluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects();
  virtual bool initSettings() { return true; }
  virtual bool startPlugin() { return true; }
  //IRostersClickHooker
  virtual bool rosterIndexClicked(IRosterIndex *AIndex, int AOrder);
  //IMessageHandler
  virtual bool checkMessage(const Message &AMessage);
  virtual void showMessage(int AMessageId);
  virtual void receiveMessage(int AMessageId);
  virtual bool openWindow(const Jid &AStreamJid, const Jid &AContactJid, Message::MessageType AType);
  virtual INotification notification(INotifications *ANotifications, const Message &AMessage);
protected:
  IChatWindow *getChatWindow(const Jid &AStreamJid, const Jid &AContactJid);
  IChatWindow *findChatWindow(const Jid &AStreamJid, const Jid &AContactJid);
  void showChatHistory(IChatWindow *AWindow);
  void showChatWindow(IChatWindow *AWindow);
  void removeActiveChatMessages(IChatWindow *AWindow);
  void updateChatWindow(IChatWindow *AWindow);
protected slots:
  void onChatMessageSend();
  void onChatWindowActivated();
  void onChatInfoFieldChanged(IInfoWidget::InfoField AField, const QVariant &AValue);
  void onChatWindowDestroyed();
  void onPresenceReceived(IPresence *APresence, const IPresenceItem &APresenceItem);
  void onStatusIconsChanged();
  void onVCardChanged(const Jid &AContactJid);
  void onRostersViewContextMenu(IRosterIndex *AIndex, Menu *AMenu);
  void onShowWindowAction(bool);
private:
  IMessageWidgets *FMessageWidgets; 
  IMessageProcessor *FMessageProcessor;
  IStatusIcons *FStatusIcons;
  IPresencePlugin *FPresencePlugin;
  IMessageArchiver *FMessageArchiver;
  IVCardPlugin *FVCardPlugin;
  IRostersView *FRostersView;
private:
  QList<IChatWindow *> FChatWindows;
  QMultiHash<IChatWindow *,int> FActiveChatMessages;
};

#endif // CHATMESSAGEHANDLER_H
