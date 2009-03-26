#ifndef MESSAGEWIDGETS_H
#define MESSAGEWIDGETS_H

#include <QObjectCleanupHandler>
#include "../../definations/optionnodes.h"
#include "../../definations/optionnodeorders.h"
#include "../../definations/optionwidgetorders.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/imessagewidgets.h"
#include "infowidget.h"
#include "editwidget.h"
#include "viewwidget.h"
#include "receiverswidget.h"
#include "toolbarwidget.h"
#include "messagewindow.h"
#include "chatwindow.h"
#include "tabwindow.h"
#include "messengeroptions.h"

class MessageWidgets : 
  public QObject,
  public IPlugin,
  public IMessageWidgets,
  public IOptionsHolder
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IMessageWidgets IOptionsHolder);
public:
  MessageWidgets();
  ~MessageWidgets();
  //IPlugin
  virtual QObject *instance() { return this; }
  virtual QUuid pluginUuid() const { return MESSAGEWIDGETS_UUID; }
  virtual void pluginInfo(IPluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects();
  virtual bool initSettings() { return true; }
  virtual bool startPlugin() { return true; }
  //IOptionsHolder
  virtual QWidget *optionsWidget(const QString &ANode, int &AOrder);
  //IMessageWidgets
  virtual IPluginManager *pluginManager() const { return FPluginManager; }
  virtual QFont defaultChatFont() const;
  virtual void setDefaultChatFont(const QFont &AFont);
  virtual QFont defaultMessageFont() const;
  virtual void setDefaultMessageFont(const QFont &AFont);
  virtual QKeySequence sendMessageKey() const;
  virtual void setSendMessageKey(const QKeySequence &AKey);
  virtual IInfoWidget *newInfoWidget(const Jid &AStreamJid, const Jid &AContactJid);
  virtual IViewWidget *newViewWidget(const Jid &AStreamJid, const Jid &AContactJid);
  virtual IEditWidget *newEditWidget(const Jid &AStreamJid, const Jid &AContactJid);
  virtual IReceiversWidget *newReceiversWidget(const Jid &AStreamJid);
  virtual IToolBarWidget *newToolBarWidget(IInfoWidget *AInfo, IViewWidget *AView, IEditWidget *AEdit, IReceiversWidget *AReceivers);
  virtual QList<IMessageWindow *> messageWindows() const;
  virtual IMessageWindow *newMessageWindow(const Jid &AStreamJid, const Jid &AContactJid, IMessageWindow::Mode AMode);
  virtual IMessageWindow *findMessageWindow(const Jid &AStreamJid, const Jid &AContactJid);
  virtual QList<IChatWindow *> chatWindows() const;
  virtual IChatWindow *newChatWindow(const Jid &AStreamJid, const Jid &AContactJid);
  virtual IChatWindow *findChatWindow(const Jid &AStreamJid, const Jid &AContactJid);
  virtual QList<int> tabWindows() const;
  virtual ITabWindow *openTabWindow(int AWindowId = 0);
  virtual ITabWindow *findTabWindow(int AWindowId = 0);
  virtual bool checkOption(IMessageWidgets::Option AOption) const;
  virtual void setOption(IMessageWidgets::Option AOption, bool AValue);
  virtual void insertResourceLoader(IMessageResource *ALoader, int AOrder);
  virtual void removeResourceLoader(IMessageResource *ALoader, int AOrder);
signals:
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
  virtual void optionChanged(IMessageWidgets::Option AOption, bool AValue);
  virtual void resourceLoaderInserted(IMessageResource *ALoader, int AOrder);
  virtual void resourceLoaderRemoved(IMessageResource *ALoader, int AOrder);
signals:
  virtual void optionsAccepted();
  virtual void optionsRejected();
protected:
  void deleteStreamWindows(const Jid &AStreamJid);
protected slots:
  void onMessageWindowDestroyed();
  void onChatWindowDestroyed();
  void onTabWindowDestroyed();
  void onStreamJidAboutToBeChanged(IXmppStream *AXmppStream, const Jid &AAfter);
  void onStreamRemoved(IXmppStream *AXmppStream);
  void onSettingsOpened();
  void onSettingsClosed();
  void onLoadTextResource(int AType, const QUrl &AName, QVariant &AValue);
private:
  IPluginManager *FPluginManager;
  IXmppStreams *FXmppStreams;
  ISettingsPlugin *FSettingsPlugin;
private:
  QHash<int,ITabWindow *> FTabWindows;
  QList<IChatWindow *> FChatWindows;
  QList<IMessageWindow *> FMessageWindows;
  QObjectCleanupHandler FCleanupHandler;
private:
  int FOptions;
  QFont FChatFont;
  QFont FMessageFont;
  QKeySequence FSendKey;
  QMultiMap<int,IMessageResource *> FResourceLoaders;
};

#endif // MESSAGEWIDGETS_H
