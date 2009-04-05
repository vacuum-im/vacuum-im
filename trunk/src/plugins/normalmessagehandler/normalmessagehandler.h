#ifndef NORMALMESSAGEHANDLER_H
#define NORMALMESSAGEHANDLER_H

#define NORMALMESSAGEHANDLER_UUID "{8592e3c3-ef5e-42a9-91c9-faf1ed9a91cc}"

#include "../../definations/messagehandlerorders.h"
#include "../../definations/rosterindextyperole.h"
#include "../../definations/rosterlabelorders.h"
#include "../../definations/notificationdataroles.h"
#include "../../definations/actiongroups.h"
#include "../../definations/resources.h"
#include "../../definations/menuicons.h"
#include "../../definations/soundfiles.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/imessageprocessor.h"
#include "../../interfaces/imessagewidgets.h"
#include "../../interfaces/inotifications.h"
#include "../../interfaces/istatusicons.h"
#include "../../interfaces/irostersview.h"
#include "../../interfaces/ipresence.h"

class NormalMessageHandler : 
  public QObject,
  public IPlugin,
  public IMessageHandler
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IMessageHandler);
public:
  NormalMessageHandler();
  ~NormalMessageHandler();
  //IPlugin
  virtual QObject *instance() { return this; }
  virtual QUuid pluginUuid() const { return NORMALMESSAGEHANDLER_UUID; }
  virtual void pluginInfo(IPluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects();
  virtual bool initSettings() { return true; }
  virtual bool startPlugin() { return true; }
  //IMessageHandler
  virtual bool checkMessage(const Message &AMessage);
  virtual void showMessage(int AMessageId);
  virtual void receiveMessage(int AMessageId);
  virtual bool openWindow(const Jid &AStreamJid, const Jid &AContactJid, Message::MessageType AType);
  virtual INotification notification(INotifications *ANotifications, const Message &AMessage);
protected:
  IMessageWindow *getMessageWindow(const Jid &AStreamJid, const Jid &AContactJid, IMessageWindow::Mode AMode);
  IMessageWindow *findMessageWindow(const Jid &AStreamJid, const Jid &AContactJid);
  void showMessageWindow(IMessageWindow *AWindow);
  void showNextNormalMessage(IMessageWindow *AWindow);
  void loadActiveNormalMessages(IMessageWindow *AWindow);
  void updateMessageWindow(IMessageWindow *AWindow);
protected slots:
  void onMessageWindowSend();
  void onMessageWindowShowNext();
  void onMessageWindowReply();
  void onMessageWindowForward();
  void onMessageWindowShowChat();
  void onMessageWindowDestroyed();
  void onPresenceReceived(IPresence *APresence, const IPresenceItem &APresenceItem);
  void onStatusIconsChanged();
  void onRostersViewContextMenu(IRosterIndex *AIndex, Menu *AMenu);
  void onShowWindowAction(bool);
private:
  IMessageWidgets *FMessageWidgets; 
  IMessageProcessor *FMessageProcessor;
  IStatusIcons *FStatusIcons;
  IPresencePlugin *FPresencePlugin;
  IRostersView *FRostersView;
private:
  QList<IMessageWindow *> FMessageWindows;
  QMultiHash<IMessageWindow *, int> FActiveNormalMessages;
};

#endif // NORMALMESSAGEHANDLER_H
