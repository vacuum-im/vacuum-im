#ifndef MESSAGEWIDGETS_H
#define MESSAGEWIDGETS_H

#include <QDesktopServices>
#include <QObjectCleanupHandler>
#include <definations/optionvalues.h>
#include <definations/optionnodes.h>
#include <definations/optionnodeorders.h>
#include <definations/optionwidgetorders.h>
#include <definations/viewurlhandlerorders.h>
#include <definations/toolbargroups.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/ioptionsmanager.h>
#include <utils/options.h>
#include "infowidget.h"
#include "editwidget.h"
#include "viewwidget.h"
#include "receiverswidget.h"
#include "menubarwidget.h"
#include "toolbarwidget.h"
#include "statusbarwidget.h"
#include "messagewindow.h"
#include "chatwindow.h"
#include "tabwindow.h"

class MessageWidgets : 
  public QObject,
  public IPlugin,
  public IMessageWidgets,
  public IOptionsHolder,
  public IViewUrlHandler
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IMessageWidgets IOptionsHolder IViewUrlHandler);
public:
  MessageWidgets();
  ~MessageWidgets();
  //IPlugin
  virtual QObject *instance() { return this; }
  virtual QUuid pluginUuid() const { return MESSAGEWIDGETS_UUID; }
  virtual void pluginInfo(IPluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects();
  virtual bool initSettings();
  virtual bool startPlugin() { return true; }
  //IOptionsHolder
  virtual IOptionsWidget *optionsWidget(const QString &ANodeId, int &AOrder, QWidget *AParent);
  //IViewUrlHandler
  virtual bool viewUrlOpen(IViewWidget *AWidget, const QUrl &AUrl, int AOrder);
  //IMessageWidgets
  virtual IPluginManager *pluginManager() const { return FPluginManager; }
  virtual IInfoWidget *newInfoWidget(const Jid &AStreamJid, const Jid &AContactJid);
  virtual IViewWidget *newViewWidget(const Jid &AStreamJid, const Jid &AContactJid);
  virtual IEditWidget *newEditWidget(const Jid &AStreamJid, const Jid &AContactJid);
  virtual IReceiversWidget *newReceiversWidget(const Jid &AStreamJid);
  virtual IMenuBarWidget *newMenuBarWidget(IInfoWidget *AInfo, IViewWidget *AView, IEditWidget *AEdit, IReceiversWidget *AReceivers);
  virtual IToolBarWidget *newToolBarWidget(IInfoWidget *AInfo, IViewWidget *AView, IEditWidget *AEdit, IReceiversWidget *AReceivers);
  virtual IStatusBarWidget *newStatusBarWidget(IInfoWidget *AInfo, IViewWidget *AView, IEditWidget *AEdit, IReceiversWidget *AReceivers);
  virtual QList<IMessageWindow *> messageWindows() const;
  virtual IMessageWindow *newMessageWindow(const Jid &AStreamJid, const Jid &AContactJid, IMessageWindow::Mode AMode);
  virtual IMessageWindow *findMessageWindow(const Jid &AStreamJid, const Jid &AContactJid) const;
  virtual QList<IChatWindow *> chatWindows() const;
  virtual IChatWindow *newChatWindow(const Jid &AStreamJid, const Jid &AContactJid);
  virtual IChatWindow *findChatWindow(const Jid &AStreamJid, const Jid &AContactJid) const;
  virtual QList<QUuid> tabWindowList() const;
  virtual QUuid appendTabWindow(const QString &AName);
  virtual void deleteTabWindow(const QUuid &AWindowId);
  virtual QString tabWindowName(const QUuid &AWindowId) const;
  virtual void setTabWindowName(const QUuid &AWindowId, const QString &AName);
  virtual QList<ITabWindow *> tabWindows() const;
  virtual ITabWindow *openTabWindow(const QUuid &AWindowId);
  virtual ITabWindow *findTabWindow(const QUuid &AWindowId) const;
  virtual void assignTabWindowPage(ITabWindowPage *APage);
  virtual QList<IViewDropHandler *> viewDropHandlers() const;
  virtual void insertViewDropHandler(IViewDropHandler *AHandler);
  virtual void removeViewDropHandler(IViewDropHandler *AHandler);
  virtual QMultiMap<int, IViewUrlHandler *> viewUrlHandlers() const;
  virtual void insertViewUrlHandler(IViewUrlHandler *AHandler, int AOrder);
  virtual void removeViewUrlHandler(IViewUrlHandler *AHandler, int AOrder);
signals:
  void infoWidgetCreated(IInfoWidget *AInfoWidget);
  void viewWidgetCreated(IViewWidget *AViewWidget);
  void editWidgetCreated(IEditWidget *AEditWidget);
  void receiversWidgetCreated(IReceiversWidget *AReceiversWidget);
  void menuBarWidgetCreated(IMenuBarWidget *AMenuBarWidget);
  void toolBarWidgetCreated(IToolBarWidget *AToolBarWidget);
  void statusBarWidgetCreated(IStatusBarWidget *AStatusBarWidget);
  void messageWindowCreated(IMessageWindow *AWindow);
  void messageWindowDestroyed(IMessageWindow *AWindow);
  void chatWindowCreated(IChatWindow *AWindow);
  void chatWindowDestroyed(IChatWindow *AWindow);
  void tabWindowAppended(const QUuid &AWindowId, const QString &AName);
  void tabWindowNameChanged(const QUuid &AWindowId, const QString &AName);
  void tabWindowDeleted(const QUuid &AWindowId);
  void tabWindowCreated(ITabWindow *AWindow);
  void tabWindowDestroyed(ITabWindow *AWindow);
  void viewDropHandlerInserted(IViewDropHandler *AHandler);
  void viewDropHandlerRemoved(IViewDropHandler *AHandler);
  void viewUrlHandlerInserted(IViewUrlHandler *AHandler, int AOrder);
  void viewUrlHandlerRemoved(IViewUrlHandler *AHandler, int AOrder);
protected:
  void insertQuoteAction(IToolBarWidget *AWidget);
  void deleteWindows();
  void deleteStreamWindows(const Jid &AStreamJid);
protected slots:
  void onViewWidgetUrlClicked(const QUrl &AUrl);
  void onQuoteActionTriggered(bool);
  void onMessageWindowDestroyed();
  void onChatWindowDestroyed();
  void onTabWindowPageAdded(ITabWindowPage *APage);
  void onTabWindowDestroyed();
  void onStreamJidAboutToBeChanged(IXmppStream *AXmppStream, const Jid &AAfter);
  void onStreamRemoved(IXmppStream *AXmppStream);
  void onOptionsOpened();
  void onOptionsClosed();
private:
  IPluginManager *FPluginManager;
  IXmppStreams *FXmppStreams;
  IOptionsManager *FOptionsManager;
private:
  QList<ITabWindow *> FTabWindows;
  QList<IChatWindow *> FChatWindows;
  QList<IMessageWindow *> FMessageWindows;
  QObjectCleanupHandler FCleanupHandler;
private:
  QMap<QString, QUuid> FPageWindows;
  QList<IViewDropHandler *> FViewDropHandlers;
  QMultiMap<int,IViewUrlHandler *> FViewUrlHandlers;
};

#endif // MESSAGEWIDGETS_H
