#ifndef MESSENGER_H
#define MESSENGER_H

#include "../../definations/messagedataroles.h"
#include "../../definations/initorders.h"
#include "../../definations/rosterlabelorders.h"
#include "../../definations/rosterindextyperole.h"
#include "../../definations/optionnodes.h"
#include "../../definations/actiongroups.h"
#include "../../interfaces/imessenger.h"
#include "../../interfaces/ixmppstreams.h"
#include "../../interfaces/istanzaprocessor.h"
#include "../../interfaces/irostersmodel.h"
#include "../../interfaces/irostersview.h"
#include "../../interfaces/itraymanager.h"
#include "../../interfaces/isettings.h"
#include "../../utils/skin.h"
#include "messagewindow.h"
#include "chatwindow.h"
#include "infowidget.h"
#include "editwidget.h"
#include "viewwidget.h"
#include "receiverswidget.h"
#include "tabwindow.h"
#include "messengeroptions.h"

class Messenger : 
  public QObject,
  public IPlugin,
  public IMessenger,
  public IStanzaHandler,
  public IRostersClickHooker,
  public IOptionsHolder
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IMessenger IStanzaHandler IRostersClickHooker IOptionsHolder);
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
  //IOptionsHolder
  virtual QWidget *optionsWidget(const QString &ANode, int &AOrder);
  //IMessenger
  virtual IPluginManager *pluginManager() const { return FPluginManager; }
  virtual void textToMessage(Message &AMessage, const QTextDocument *ADocument, const QString &ALang = "") const;
  virtual void messageToText(QTextDocument *ADocument, const Message &AMessage, const QString &ALang = "") const;
  virtual int newMessageId() { FMessageId++; return FMessageId; }
  virtual bool sendMessage(const Message &AMessage, const Jid &AStreamJid);
  virtual int receiveMessage(const Message &AMessage);
  virtual void showMessage(int AMessageId);
  virtual void removeMessage(int AMessageId);
  virtual Message messageById(int AMessageId) const { return FMessages.value(AMessageId); }
  virtual QList<int> messages(const Jid &AStreamJid, const Jid &AFromJid = Jid(), Message::MessageType AMesType = Message::AnyType);
  virtual bool checkOption(IMessenger::Option AOption) const;
  virtual void setOption(IMessenger::Option AOption, bool AValue);
  //MessageWindows
  virtual IInfoWidget *newInfoWidget(const Jid &AStreamJid, const Jid &AContactJid);
  virtual IViewWidget *newViewWidget(const Jid &AStreamJid, const Jid &AContactJid);
  virtual IEditWidget *newEditWidget(const Jid &AStreamJid, const Jid &AContactJid);
  virtual IReceiversWidget *newReceiversWidget(const Jid &AStreamJid);
  virtual IMessageWindow *openMessageWindow(const Jid &AStreamJid, const Jid &AContactJid, IMessageWindow::Mode AMode);
  virtual IMessageWindow *findMessageWindow(const Jid &AStreamJid, const Jid &AContactJid);
  virtual IChatWindow *openChatWindow(const Jid &AStreamJid, const Jid &AContactJid);
  virtual IChatWindow *findChatWindow(const Jid &AStreamJid, const Jid &AContactJid);
  virtual QList<int> tabWindows() const { return FTabWindows.keys(); }
  virtual ITabWindow *openTabWindow(int AWindowId = 0);
  virtual ITabWindow *findTabWindow(int AWindowId = 0);
signals:
  virtual void messageReceived(const Message &AMessage);
  virtual void messageNotified(int AMessageId);
  virtual void messageUnNotified(int AMessageId);
  virtual void messageRemoved(const Message &AMessage);
  virtual void messageSent(const Message &AMessage);
  virtual void optionChanged(IMessenger::Option AOption, bool AValue);
  virtual void optionsAccepted();
  virtual void optionsRejected();
  //MessageWindows
  virtual void tabWindowCreated(ITabWindow *AWindow);
  virtual void tabWindowDestroyed(ITabWindow *AWindow);
  virtual void chatWindowCreated(IChatWindow *AWindow);
  virtual void chatWindowDestroyed(IChatWindow *AWindow);
  virtual void messageWindowCreated(IMessageWindow *AWindow);
  virtual void messageWindowDestroyed(IMessageWindow *AWindow);
protected:
  void notifyMessage(int AMessageId);
  void unNotifyMessage(int AMessageId);
  void removeStreamMessages(const Jid &AStreamJid);
  void deleteStreamWindows(const Jid &AStreamJid);
protected slots:
  void onStreamAdded(IXmppStream *AXmppStream);
  void onStreamJidAboutToBeChanged(IXmppStream *AXmppStream, const Jid &AAfter);
  void onStreamJidChanged(IXmppStream *AXmppStream, const Jid &ABefour);
  void onStreamRemoved(IXmppStream *AXmppStream);
  void onSystemIconsetChanged();
  void onRostersViewContextMenu(IRosterIndex *AIndex, Menu *AMenu);
  void onRosterLabelDClicked(IRosterIndex *AIndex, int ALabelId, bool &AAccepted);
  void onTrayNotifyActivated(int ANotifyId);
  void onRosterIndexCreated(IRosterIndex *AIndex, IRosterIndex *AParent);
  void onMessageWindowJidChanged(const Jid &ABefour);
  void onMessageWindowDestroyed();
  void onChatWindowJidChanged(const Jid &ABefour);
  void onChatWindowDestroyed();
  void onTabWindowDestroyed();
  void onSettingsOpened();
  void onSettingsClosed();
  void onOptionsDialogAccepted();
  void onOptionsDialogRejected();
  void onShowWindowAction(bool);
private:
  IPluginManager *FPluginManager;
  IXmppStreams *FXmppStreams;
  IStanzaProcessor *FStanzaProcessor;
  IRostersModel *FRostersModel;
  IRostersModelPlugin *FRostersModelPlugin;
  IRostersView *FRostersView;
  IRostersViewPlugin *FRostersViewPlugin;
  ITrayManager *FTrayManager;
  ISettingsPlugin *FSettingsPlugin;
private:
  QIcon FNormalIcon;
  QIcon FChatIcon;
  SkinIconset *FSystemIconset;
private:
  QHash<int,ITabWindow *> FTabWindows;
  QList<IChatWindow *> FChatWindows;
  QList<IMessageWindow *> FMessageWindows;
  MessengerOptions *FMessengerOptions;
private:
  int FOptions;
  int FMessageId;
  int FIndexClickHooker;
  int FNormalLabelId;
  int FChatLabelId;
  QMap<int,Message> FMessages;
  QHash<int,int> FTrayId2MessageId;
  QHash<IXmppStream *,HandlerId> FMessageHandlers;
};

#endif // MESSENGER_H
