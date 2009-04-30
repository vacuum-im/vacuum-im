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
#include "../../interfaces/imessagestyles.h"
#include "../../interfaces/imessagearchiver.h"
#include "../../interfaces/inotifications.h"
#include "../../interfaces/istatusicons.h"
#include "../../interfaces/irostersview.h"
#include "../../interfaces/ipresence.h"
#include "../../interfaces/ivcard.h"
#include "../../interfaces/iavatars.h"
#include "../../interfaces/istatuschanger.h"

struct WindowStatus {
  int lastContent;
  QString lastSender;
  QDateTime lastTime;
  QString lastStatusShow;
};

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
  IChatWindow *getWindow(const Jid &AStreamJid, const Jid &AContactJid);
  IChatWindow *findWindow(const Jid &AStreamJid, const Jid &AContactJid);
  void showWindow(IChatWindow *AWindow);
  void updateWindow(IChatWindow *AWindow);
  void removeActiveMessages(IChatWindow *AWindow);
  void showHistory(IChatWindow *AWindow);
  void setMessageStyle(IChatWindow *AWindow);
  void fillContentOptions(IChatWindow *AWindow, IMessageStyle::ContentOptions &AOptions) const;
  void showStyledStatus(IChatWindow *AWindow, const QString &AMessage, const QString &AStatusKeyword);
  void showStyledMessage(IChatWindow *AWindow, const Message &AMessage, bool ANoScroll = false);
protected slots:
  void onMessageReady();
  void onWindowActivated();
  void onInfoFieldChanged(IInfoWidget::InfoField AField, const QVariant &AValue);
  void onMessageStyleSettingsChanged(int AMessageType, const IMessageStyles::StyleSettings &ASettings);
  void onViewWidgetContentAppended(const QString &AMessage, const IMessageStyle::ContentOptions &AOptions);
  void onWindowDestroyed();
  void onStatusIconsChanged();
  void onShowWindowAction(bool);
  void onRostersViewContextMenu(IRosterIndex *AIndex, Menu *AMenu);
  void onPresenceReceived(IPresence *APresence, const IPresenceItem &APresenceItem);
private:
  IMessageWidgets *FMessageWidgets;
  IMessageProcessor *FMessageProcessor;
  IMessageStyles *FMessageStyles;
  IPresencePlugin *FPresencePlugin;
  IMessageArchiver *FMessageArchiver;
  IRostersView *FRostersView;
  IStatusIcons *FStatusIcons;
  IStatusChanger *FStatusChanger;
private:
  QList<IChatWindow *> FWindows;
  QMultiMap<IChatWindow *,int> FActiveMessages;
  QMap<IViewWidget *, WindowStatus> FWindowStatus;
};

#endif // CHATMESSAGEHANDLER_H
