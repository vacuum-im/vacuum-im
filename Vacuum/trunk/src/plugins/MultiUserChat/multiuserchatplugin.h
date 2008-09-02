#ifndef MULTIUSERCHATPLUGIN_H
#define MULTIUSERCHATPLUGIN_H

#include <QMessageBox>
#include "../../definations/actiongroups.h"
#include "../../definations/rosterindextyperole.h"
#include "../../definations/rosterlabelorders.h"
#include "../../definations/discofeatureorder.h"
#include "../../definations/messagehandlerorders.h"
#include "../../interfaces/imultiuserchat.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/ixmppstreams.h"
#include "../../interfaces/istanzaprocessor.h"
#include "../../interfaces/ipresence.h"
#include "../../interfaces/imainwindow.h"
#include "../../interfaces/itraymanager.h"
#include "../../interfaces/iservicediscovery.h"
#include "../../interfaces/inotifications.h"
#include "../../utils/message.h"
#include "../../utils/action.h"
#include "../../utils/skin.h"
#include "multiuserchat.h"
#include "multiuserchatwindow.h"
#include "joinmultichatdialog.h"

class MultiUserChatPlugin : 
  public QObject,
  public IPlugin,
  public IMultiUserChatPlugin,
  public IDiscoFeatureHandler,
  public IMessageHandler
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IMultiUserChatPlugin IDiscoFeatureHandler IMessageHandler);
public:
  MultiUserChatPlugin();
  ~MultiUserChatPlugin();
  //IPlugin
  virtual QObject *instance() { return this; }
  virtual QUuid pluginUuid() const { return MULTIUSERCHAT_UUID; }
  virtual void pluginInfo(PluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects();
  virtual bool initSettings() { return true; }
  virtual bool startPlugin() { return true; }
  //IDiscoFeatureHandler
  virtual bool execDiscoFeature(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo);
  virtual Action *createDiscoFeatureAction(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo, QWidget *AParent);
  //IMessageHandler
  virtual bool openWindow(IRosterIndex * /*AIndex*/) { return false; }
  virtual bool openWindow(const Jid &/*AStreamJid*/, const Jid &/*AContactJid*/, Message::MessageType /*AType*/) { return false; }
  virtual bool checkMessage(const Message &AMessage);
  virtual INotification notification(INotifications *ANotifications, const Message &AMessage);
  virtual void receiveMessage(int AMessageId);
  virtual void showMessage(int AMessageId);
  //IMultiUserChatPlugin
  virtual IPluginManager *pluginManager() const { return FPluginManager; }
  virtual bool requestRoomNick(const Jid &AStreamJid, const Jid &ARoomJid);
  virtual IMultiUserChat *getMultiUserChat(const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANick, 
    const QString &APassword, bool ADedicated = false);
  virtual QList<IMultiUserChat *> multiUserChats() const { return FChats; }
  virtual IMultiUserChat *multiUserChat(const Jid &AStreamJid, const Jid &ARoomJid) const;
  virtual IMultiUserChatWindow *getMultiChatWindow(const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANick, 
    const QString &APassword);
  virtual QList<IMultiUserChatWindow *> multiChatWindows() const { return FChatWindows; }
  virtual IMultiUserChatWindow *multiChatWindow(const Jid &AStreamJid, const Jid &ARoomJid) const;
  virtual void showJoinMultiChatDialog(const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANick, const QString &APassword);
signals:
  virtual void roomNickReceived(const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANick);
  virtual void multiUserChatCreated(IMultiUserChat *AMultiChat);
  virtual void multiUserChatDestroyed(IMultiUserChat *AMultiChat);
  virtual void multiChatWindowCreated(IMultiUserChatWindow *AWindow);
  virtual void multiChatWindowDestroyed(IMultiUserChatWindow *AWindow);
  virtual void multiUserContextMenu(IMultiUserChatWindow *AWindow, IMultiUser *AUser, Menu *AMenu);
protected:
  void insertChatAction(IMultiUserChatWindow *AWindow);
  void removeChatAction(IMultiUserChatWindow *AWindow);
  void registerDiscoFeatures();
  Menu *createInviteMenu(const Jid &AContactJid, QWidget *AParent) const;
  Action *createJoinAction(const Jid &AStreamJid, const Jid &ARoomJid, QObject *AParent) const;
protected slots:
  void onMultiUserContextMenu(IMultiUser *AUser, Menu *AMenu);
  void onMultiUserChatDestroyed();
  void onMultiChatWindowDestroyed();
  void onStreamRemoved(IXmppStream *AXmppStream);
  void onJoinActionTriggered(bool);
  void onRostersViewContextMenu(IRosterIndex *AIndex, Menu *AMenu);
  void onChatActionTriggered(bool);
  void onDiscoInfoReceived(const IDiscoInfo &ADiscoInfo);
  void onInviteDialogFinished(int AResult);
  void onInviteActionTriggered(bool);
private:
  IPluginManager *FPluginManager;
  IMessenger *FMessenger;
  IRostersViewPlugin *FRostersViewPlugin;
  IMainWindowPlugin *FMainWindowPlugin;
  ITrayManager *FTrayManager;
  IXmppStreams *FXmppStreams;
  IServiceDiscovery *FDiscovery;
  INotifications *FNotifications;
private:
  Menu *FChatMenu;
  Action *FJoinAction;
private:
  struct InviteFields {
    Jid streamJid;
    Jid roomJid;
    Jid fromJid;
    QString password;
  };
  QList<IMultiUserChat *> FChats;
  QList<IMultiUserChatWindow *> FChatWindows;
  QHash<IMultiUserChatWindow *, Action *> FChatActions;
  QList<int> FActiveInvites;
  QHash<QMessageBox *,InviteFields> FInviteDialogs;
};

#endif // MULTIUSERCHATPLUGIN_H
