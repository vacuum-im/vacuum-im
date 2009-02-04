#ifndef MESSENGER_H
#define MESSENGER_H

#include <QObjectCleanupHandler>
#include "../../definations/messagewriterorders.h"
#include "../../definations/messagedataroles.h"
#include "../../definations/messagehandlerorders.h"
#include "../../definations/rosterclickhookerorders.h"
#include "../../definations/rosterindextyperole.h"
#include "../../definations/optionnodes.h"
#include "../../definations/actiongroups.h"
#include "../../definations/stanzahandlerpriority.h"
#include "../../interfaces/imessenger.h"
#include "../../interfaces/ixmppstreams.h"
#include "../../interfaces/istanzaprocessor.h"
#include "../../interfaces/irostersview.h"
#include "../../interfaces/itraymanager.h"
#include "../../interfaces/inotifications.h"
#include "../../interfaces/isettings.h"
#include "../../utils/skin.h"
#include "messagewindow.h"
#include "chatwindow.h"
#include "infowidget.h"
#include "editwidget.h"
#include "viewwidget.h"
#include "receiverswidget.h"
#include "toolbarwidget.h"
#include "tabwindow.h"
#include "messengeroptions.h"
#include "messagehandler.h"

class Messenger : 
  public QObject,
  public IPlugin,
  public IMessenger,
  public IStanzaHandler,
  public IRostersClickHooker,
  public IOptionsHolder,
  public IMessageWriter
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IMessenger IStanzaHandler IRostersClickHooker IOptionsHolder IMessageWriter);
public:
  Messenger();
  ~Messenger();
  virtual QObject *instance() { return this; }
  //IPlugin
  virtual QUuid pluginUuid() const { return MESSENGER_UUID; }
  virtual void pluginInfo(IPluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects();
  virtual bool initSettings() { return true; }
  virtual bool startPlugin() { return true; }
  //IStanzaHandler
  virtual bool editStanza(int, const Jid &, Stanza *, bool &) { return false; }
  virtual bool readStanza(int AHandlerId, const Jid &AStreamJid, const Stanza &AStanza, bool &AAccept);
  //IRostersClickHooker
  virtual bool rosterIndexClicked(IRosterIndex *AIndex, int AOrder);
  //IOptionsHolder
  virtual QWidget *optionsWidget(const QString &ANode, int &AOrder);
  //IMessageWriter
  virtual void writeMessage(Message &AMessage, QTextDocument *ADocument, const QString &ALang, int AOrder);
  virtual void writeText(Message &AMessage, QTextDocument *ADocument, const QString &ALang, int AOrder);
  //IMessenger
  virtual IPluginManager *pluginManager() const { return FPluginManager; }
  virtual void insertMessageHandler(IMessageHandler *AHandler, int AOrder);
  virtual void removeMessageHandler(IMessageHandler *AHandler, int AOrder);
  virtual void insertMessageWriter(IMessageWriter *AWriter, int AOrder);
  virtual void removeMessageWriter(IMessageWriter *AWriter, int AOrder);
  virtual void insertResourceLoader(IResourceLoader *ALoader, int AOrder);
  virtual void removeResourceLoader(IResourceLoader *ALoader, int AOrder);
  virtual void textToMessage(Message &AMessage, const QTextDocument *ADocument, const QString &ALang = "") const;
  virtual void messageToText(QTextDocument *ADocument, const Message &AMessage, const QString &ALang = "") const;
  virtual bool openWindow(const Jid &AStreamJid, const Jid &AContactJid, Message::MessageType AType) const;
  virtual bool sendMessage(const Message &AMessage, const Jid &AStreamJid);
  virtual int receiveMessage(const Message &AMessage);
  virtual void showMessage(int AMessageId);
  virtual void removeMessage(int AMessageId);
  virtual Message messageById(int AMessageId) const { return FMessages.value(AMessageId); }
  virtual QList<int> messages(const Jid &AStreamJid, const Jid &AFromJid = Jid(), int AMesTypes = Message::AnyType);
  virtual bool checkOption(IMessenger::Option AOption) const;
  virtual void setOption(IMessenger::Option AOption, bool AValue);
  //MessageWindows
  virtual QFont defaultChatFont() const { return FChatFont; }
  virtual void setDefaultChatFont(const QFont &AFont);
  virtual QFont defaultMessageFont() const { return FMessageFont; }
  virtual void setDefaultMessageFont(const QFont &AFont);
  virtual QKeySequence sendMessageKey() const;
  virtual void setSendMessageKey(const QKeySequence &AKey);
  virtual IInfoWidget *newInfoWidget(const Jid &AStreamJid, const Jid &AContactJid);
  virtual IViewWidget *newViewWidget(const Jid &AStreamJid, const Jid &AContactJid);
  virtual IEditWidget *newEditWidget(const Jid &AStreamJid, const Jid &AContactJid);
  virtual IReceiversWidget *newReceiversWidget(const Jid &AStreamJid);
  virtual IToolBarWidget *newToolBarWidget(IInfoWidget *AInfo, IViewWidget *AView, IEditWidget *AEdit, IReceiversWidget *AReceivers);
  virtual QList<IMessageWindow *> messageWindows() const { return FMessageWindows; }
  virtual IMessageWindow *newMessageWindow(const Jid &AStreamJid, const Jid &AContactJid, IMessageWindow::Mode AMode);
  virtual IMessageWindow *findMessageWindow(const Jid &AStreamJid, const Jid &AContactJid);
  virtual QList<IChatWindow *> chatWindows() const { return FChatWindows; }
  virtual IChatWindow *newChatWindow(const Jid &AStreamJid, const Jid &AContactJid);
  virtual IChatWindow *findChatWindow(const Jid &AStreamJid, const Jid &AContactJid);
  virtual QList<int> tabWindows() const { return FTabWindows.keys(); }
  virtual ITabWindow *openTabWindow(int AWindowId = 0);
  virtual ITabWindow *findTabWindow(int AWindowId = 0);
signals:
  virtual void messageHandlerInserted(IMessageHandler *AHandler, int AOrder);
  virtual void messageHandlerRemoved(IMessageHandler *AHandler, int AOrder);
  virtual void messageWriterInserted(IMessageWriter *AWriter, int AOrder);
  virtual void messageWriterRemoved(IMessageWriter *AWriter, int AOrder);
  virtual void resourceLoaderInserted(IResourceLoader *ALoader, int AOrder);
  virtual void resourceLoaderRemoved(IResourceLoader *ALoader, int AOrder);
  virtual void messageNotified(int AMessageId);
  virtual void messageUnNotified(int AMessageId);
  virtual void messageReceive(Message &AMessage);
  virtual void messageReceived(const Message &AMessage);
  virtual void messageRemoved(const Message &AMessage);
  virtual void messageSend(Message &AMessage);
  virtual void messageSent(const Message &AMessage);
  virtual void optionChanged(IMessenger::Option AOption, bool AValue);
  virtual void optionsAccepted();
  virtual void optionsRejected();
  //MessageWindows
  virtual void defaultChatFontChanged(const QFont &AFont);
  virtual void defaultMessageFontChanged(const QFont &AFont);
  virtual void sendMessageKeyChanged(const QKeySequence &AKey);
  virtual void infoWidgetCreated(IInfoWidget *AInfoWidget);
  virtual void viewWidgetCreated(IViewWidget *AViewWidget);
  virtual void editWidgetCreated(IEditWidget *AEditWidget);
  virtual void receiversWidgetCreated(IReceiversWidget *AReceiversWidget);
  virtual void toolBarWidgetCreated(IToolBarWidget *AToolBarWidget);
  virtual void messageWindowCreated(IMessageWindow *AWindow);
  virtual void messageWindowDestroyed(IMessageWindow *AWindow);
  virtual void chatWindowCreated(IChatWindow *AWindow);
  virtual void chatWindowDestroyed(IChatWindow *AWindow);
  virtual void tabWindowCreated(ITabWindow *AWindow);
  virtual void tabWindowDestroyed(ITabWindow *AWindow);
protected:
  int newMessageId() { FMessageId++; return FMessageId; }
  IMessageHandler *getMessageHandler(const Message &AMessage);
  void notifyMessage(int AMessageId);
  void unNotifyMessage(int AMessageId);
  void removeStreamMessages(const Jid &AStreamJid);
  void deleteStreamWindows(const Jid &AStreamJid);
  QString prepareBodyForSend(const QString &AString) const;
  QString prepareBodyForReceive(const QString &AString) const;
protected slots:
  void onStreamAdded(IXmppStream *AXmppStream);
  void onStreamJidAboutToBeChanged(IXmppStream *AXmppStream, const Jid &AAfter);
  void onStreamJidChanged(IXmppStream *AXmppStream, const Jid &ABefour);
  void onStreamRemoved(IXmppStream *AXmppStream);
  void onRostersViewContextMenu(IRosterIndex *AIndex, Menu *AMenu);
  void onNotificationActivated(int ANotifyId);
  void onMessageWindowDestroyed();
  void onChatWindowDestroyed();
  void onTabWindowDestroyed();
  void onSettingsOpened();
  void onSettingsClosed();
  void onShowWindowAction(bool);
  void onTextLoadResource(int AType, const QUrl &AName, QVariant &AValue);
private:
  IPluginManager *FPluginManager;
  IXmppStreams *FXmppStreams;
  IStanzaProcessor *FStanzaProcessor;
  IRostersModel *FRostersModel;
  IRostersView *FRostersView;
  IRostersViewPlugin *FRostersViewPlugin;
  INotifications *FNotifications;
  ISettingsPlugin *FSettingsPlugin;
private:
  QHash<int,ITabWindow *> FTabWindows;
  QList<IChatWindow *> FChatWindows;
  QList<IMessageWindow *> FMessageWindows;
  MessengerOptions *FMessengerOptions;
  QObjectCleanupHandler FCleanupHandler;
private:
  int FOptions;
  int FMessageId;
  QFont FChatFont;
  QFont FMessageFont;
  QKeySequence FSendKey;
  MessageHandler *FMessageHandler;
  QMap<int,Message> FMessages;
  QHash<int,int> FNotifyId2MessageId;
  QHash<IXmppStream *,int> FStanzaHandlers;
  QHash<int, IMessageHandler *> FHandlerForMessage;
  QMultiMap<int,IMessageHandler *> FMessageHandlers;
  QMultiMap<int,IMessageWriter *> FMessageWriters;
  QMultiMap<int,IResourceLoader *> FResourceLoaders;
};

#endif // MESSENGER_H
