#include "multiuserchatplugin.h"

#define IN_GROUPCHAT          "psi/groupChat"

#define ADR_HOST              Action::DR_Parametr1
#define ADR_ROOM              Action::DR_Parametr2
#define ADR_NICK              Action::DR_Parametr3
#define ADR_PASSWORD          Action::DR_Parametr4

MultiUserChatPlugin::MultiUserChatPlugin()
{
  FPluginManager = NULL;
  FMessenger = NULL;
  FRostersViewPlugin = NULL;
  FMainWindowPlugin = NULL;
  FTrayManager = NULL;
  FXmppStreams = NULL;

  FChatMenu = NULL;
  FJoinAction = NULL;
}

MultiUserChatPlugin::~MultiUserChatPlugin()
{
  delete FChatMenu;
}

void MultiUserChatPlugin::pluginInfo(PluginInfo *APluginInfo)
{
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->description = tr("Implements multi-user text conferencing");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = tr("Multi-User Chat"); 
  APluginInfo->uid = MULTIUSERCHAT_UUID;
  APluginInfo->version = "0.1";
  APluginInfo->dependences.append(MESSENGER_UUID);
  APluginInfo->dependences.append(XMPPSTREAMS_UUID);
  APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool MultiUserChatPlugin::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  FPluginManager = APluginManager;

  IPlugin *plugin = APluginManager->getPlugins("IMessenger").value(0,NULL);
  if (plugin)
  {
    FMessenger = qobject_cast<IMessenger *>(plugin->instance());
  }

  plugin = APluginManager->getPlugins("IXmppStreams").value(0,NULL);
  if (plugin)
  {
    FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
    if (FXmppStreams)
    {
      connect(FXmppStreams->instance(),SIGNAL(removed(IXmppStream *)),SLOT(onStreamRemoved(IXmppStream *)));
    }
  }

  plugin = APluginManager->getPlugins("IRostersViewPlugin").value(0,NULL);
  if (plugin)
  {
    FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
  }

  plugin = APluginManager->getPlugins("IMainWindowPlugin").value(0,NULL);
  if (plugin)
  {
    FMainWindowPlugin = qobject_cast<IMainWindowPlugin *>(plugin->instance());
  }

  plugin = APluginManager->getPlugins("ITrayManager").value(0,NULL);
  if (plugin)
  {
    FTrayManager = qobject_cast<ITrayManager *>(plugin->instance());
  }

  return FMessenger!=NULL;
}

bool MultiUserChatPlugin::initObjects()
{
  FChatMenu = new Menu(NULL);
  FChatMenu->setIcon(SYSTEM_ICONSETFILE,IN_GROUPCHAT);
  FChatMenu->setTitle(tr("Groupchats"));
  connect(FChatMenu->menuAction(),SIGNAL(triggered(bool)),SLOT(onJoinActionTriggered(bool)));

  FJoinAction = new Action(FChatMenu);
  FJoinAction->setIcon(SYSTEM_ICONSETFILE,IN_GROUPCHAT);
  FJoinAction->setText(tr("Join groupchat"));
  connect(FJoinAction,SIGNAL(triggered(bool)),SLOT(onJoinActionTriggered(bool)));
  FChatMenu->addAction(FJoinAction,AG_DEFAULT+100,true);

  if (FRostersViewPlugin && FRostersViewPlugin->rostersView())
  {
    connect(FRostersViewPlugin->rostersView(),SIGNAL(contextMenu(IRosterIndex *, Menu *)),
      SLOT(onRostersViewContextMenu(IRosterIndex *, Menu *)));
  }
  if (FMainWindowPlugin && FMainWindowPlugin->mainWindow())
  {
    FMainWindowPlugin->mainWindow()->topToolBarChanger()->addAction(FChatMenu->menuAction(),AG_MULTIUSERCHAT_MWTTB);
  }
  if (FTrayManager)
  {
    FTrayManager->addAction(FChatMenu->menuAction(),AG_MULTIUSERCHAT_TRAY,true);
  }
  return true;
}

IMultiUserChat *MultiUserChatPlugin::getMultiUserChat(const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANick, 
                                                      const QString &APassword, bool ADedicated)
{
  IMultiUserChat *chat = multiUserChat(AStreamJid,ARoomJid);
  if (!chat)
  {
    chat = new MultiUserChat(this,ADedicated ? NULL: FMessenger,AStreamJid,ARoomJid,ANick,APassword,this);
    connect(chat->instance(),SIGNAL(chatDestroyed()),SLOT(onMultiUserChatDestroyed()));
    FChats.append(chat);
  }
  return chat;
}

IMultiUserChat *MultiUserChatPlugin::multiUserChat(const Jid &AStreamJid, const Jid &ARoomJid) const
{
  foreach(IMultiUserChat *chat, FChats)
    if (chat->streamJid() == AStreamJid && chat->roomJid() == ARoomJid)
      return chat;
  return NULL;
}

IMultiUserChatWindow *MultiUserChatPlugin::getMultiChatWindow(const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANick, 
                                                              const QString &APassword)
{
  IMultiUserChatWindow *chatWindow = multiChatWindow(AStreamJid,ARoomJid);
  if (!chatWindow)
  {
    IMultiUserChat *chat = getMultiUserChat(AStreamJid,ARoomJid,ANick,APassword,false);
    chatWindow = new MultiUserChatWindow(FMessenger,chat);
    connect(chatWindow,SIGNAL(multiUserContextMenu(IMultiUser *, Menu *)),SLOT(onMultiUserContextMenu(IMultiUser *, Menu *)));
    connect(chatWindow,SIGNAL(windowDestroyed()),SLOT(onMultiChatWindowDestroyed()));
    insertChatAction(chatWindow);
    FChatWindows.append(chatWindow);
    emit multiChatWindowCreated(chatWindow);
  }
  return chatWindow;
}

IMultiUserChatWindow *MultiUserChatPlugin::multiChatWindow(const Jid &AStreamJid, const Jid &ARoomJid) const
{
  foreach(IMultiUserChatWindow *chatWindow,FChatWindows)
    if (chatWindow->streamJid()==AStreamJid && chatWindow->roomJid()==ARoomJid)
      return chatWindow;
  return NULL;
}

void MultiUserChatPlugin::insertChatAction(IMultiUserChatWindow *AWindow)
{
  Action *action = new Action(FChatMenu);
  action->setIcon(SYSTEM_ICONSETFILE,IN_GROUPCHAT);
  action->setText(tr("%1 as %2").arg(AWindow->multiUserChat()->roomJid().bare()).arg(AWindow->multiUserChat()->nickName()));
  connect(action,SIGNAL(triggered(bool)),SLOT(onChatActionTriggered(bool)));
  FChatMenu->addAction(action,AG_DEFAULT,false);
  FChatActions.insert(AWindow,action);
}

void MultiUserChatPlugin::removeChatAction(IMultiUserChatWindow *AWindow)
{
  if (FChatActions.contains(AWindow))
    FChatMenu->removeAction(FChatActions.take(AWindow));
}

void MultiUserChatPlugin::onMultiUserContextMenu(IMultiUser *AUser, Menu *AMenu)
{
  IMultiUserChatWindow *chatWindow = qobject_cast<IMultiUserChatWindow *>(sender());
  if (chatWindow)
    emit multiUserContextMenu(chatWindow,AUser,AMenu);
}

void MultiUserChatPlugin::onMultiUserChatDestroyed()
{
  IMultiUserChat *chat = qobject_cast<IMultiUserChat *>(sender());
  if (FChats.contains(chat))
  {
    FChats.removeAt(FChats.indexOf(chat));
    emit multiUserChatDestroyed(chat);
  }
}

void MultiUserChatPlugin::onMultiChatWindowDestroyed()
{
  IMultiUserChatWindow *chatWindow = qobject_cast<IMultiUserChatWindow *>(sender());
  if (chatWindow)
  {
    removeChatAction(chatWindow);
    FChatWindows.removeAt(FChatWindows.indexOf(chatWindow));
    emit multiChatWindowCreated(chatWindow);
  }
}

void MultiUserChatPlugin::onStreamRemoved(IXmppStream *AXmppStream)
{
  foreach(IMultiUserChatWindow *chatWindow,FChatWindows)
    if (chatWindow->streamJid() == AXmppStream->jid())
      chatWindow->exitMultiUserChat();
}

void MultiUserChatPlugin::onJoinActionTriggered(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    QString host = action->data(ADR_HOST).toString();
    QString room = action->data(ADR_ROOM).toString();
    QString nick = action->data(ADR_NICK).toString();
    QString password = action->data(ADR_PASSWORD).toString();
    Jid streamJid = action->data(Action::DR_StreamJid).toString();
    Jid roomJid(room,host,"");
    JoinMultiChatDialog *dialog = new JoinMultiChatDialog(this,streamJid,roomJid,nick,password);
    dialog->show();
  }
}

void MultiUserChatPlugin::onRostersViewContextMenu(IRosterIndex *AIndex, Menu *AMenu)
{
  int show = AIndex->data(RDR_Show).toInt();
  if (AIndex->type() == RIT_StreamRoot && show!=IPresence::Offline && show!=IPresence::Error)
  {
    Action *action = new Action(AMenu);
    action->setIcon(SYSTEM_ICONSETFILE,IN_GROUPCHAT);
    action->setText(tr("Join groupchat"));
    action->setData(Action::DR_StreamJid,AIndex->data(RDR_Jid));
    connect(action,SIGNAL(triggered(bool)),SLOT(onJoinActionTriggered(bool)));
    AMenu->addAction(action,AG_MULTIUSERCHAT_ROSTER,true);
  }
}

void MultiUserChatPlugin::onChatActionTriggered(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  IMultiUserChatWindow *window = FChatActions.key(action,NULL);
  if (window)
    window->showWindow();
}

Q_EXPORT_PLUGIN2(MultiUserChatPlugin, MultiUserChatPlugin)
