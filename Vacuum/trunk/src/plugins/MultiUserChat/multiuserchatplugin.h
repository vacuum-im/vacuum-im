#ifndef MULTIUSERCHATPLUGIN_H
#define MULTIUSERCHATPLUGIN_H

#include "../../definations/actiongroups.h"
#include "../../definations/rosterindextyperole.h"
#include "../../interfaces/imultiuserchat.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/ixmppstreams.h"
#include "../../interfaces/istanzaprocessor.h"
#include "../../interfaces/ipresence.h"
#include "../../interfaces/imainwindow.h"
#include "../../interfaces/itraymanager.h"
#include "../../utils/action.h"
#include "multiuserchat.h"
#include "multiuserchatwindow.h"
#include "joinmultichatdialog.h"

class MultiUserChatPlugin : 
  public QObject,
  public IPlugin,
  public IMultiUserChatPlugin
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IMultiUserChatPlugin);
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
  //IMultiUserChatPlugin
  virtual IPluginManager *pluginManager() const { return FPluginManager; }
  virtual IMultiUserChat *getMultiUserChat(const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANick, 
    const QString &APassword, bool ADedicated = false);
  virtual QList<IMultiUserChat *> multiUserChats() const { return FChats; }
  virtual IMultiUserChat *multiUserChat(const Jid &AStreamJid, const Jid &ARoomJid) const;
  virtual IMultiUserChatWindow *getMultiChatWindow(const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANick, 
    const QString &APassword);
  virtual QList<IMultiUserChatWindow *> multiChatWindows() const { return FChatWindows; }
  virtual IMultiUserChatWindow *multiChatWindow(const Jid &AStreamJid, const Jid &ARoomJid) const;
signals:
  virtual void multiUserChatCreated(IMultiUserChat *AMultiChat);
  virtual void multiUserChatDestroyed(IMultiUserChat *AMultiChat);
  virtual void multiChatWindowCreated(IMultiUserChatWindow *AWindow);
  virtual void multiChatWindowDestroyed(IMultiUserChatWindow *AWindow);
  virtual void multiUserContextMenu(IMultiUserChatWindow *AWindow, IMultiUser *AUser, Menu *AMenu);
protected:
  void insertChatAction(IMultiUserChatWindow *AWindow);
  void removeChatAction(IMultiUserChatWindow *AWindow);
protected slots:
  void onMultiUserContextMenu(IMultiUser *AUser, Menu *AMenu);
  void onMultiUserChatDestroyed();
  void onMultiChatWindowDestroyed();
  void onStreamRemoved(IXmppStream *AXmppStream);
  void onJoinActionTriggered(bool);
  void onRostersViewContextMenu(IRosterIndex *AIndex, Menu *AMenu);
  void onChatActionTriggered(bool);
private:
  IPluginManager *FPluginManager;
  IMessenger *FMessenger;
  IRostersViewPlugin *FRostersViewPlugin;
  IMainWindowPlugin *FMainWindowPlugin;
  ITrayManager *FTrayManager;
  IXmppStreams *FXmppStreams;
private:
  Menu *FChatMenu;
  Action *FJoinAction;
private:
  QList<IMultiUserChat *> FChats;
  QList<IMultiUserChatWindow *> FChatWindows;
  QHash<IMultiUserChatWindow *, Action *> FChatActions;
};

#endif // MULTIUSERCHATPLUGIN_H
