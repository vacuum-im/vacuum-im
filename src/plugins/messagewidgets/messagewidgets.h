#ifndef MESSAGEWIDGETS_H
#define MESSAGEWIDGETS_H

#include <QMultiMap>
#include <QDesktopServices>
#include <QObjectCleanupHandler>
#include <definations/optionnodes.h>
#include <definations/optionnodeorders.h>
#include <definations/optionwidgetorders.h>
#include <definations/widgeturlhandlerorders.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/isettings.h>
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
#include "messengeroptions.h"

class MessageWidgets : 
  public QObject,
  public IPlugin,
  public IMessageWidgets,
  public IOptionsHolder,
  public IWidgetUrlHandler
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IMessageWidgets IOptionsHolder IWidgetUrlHandler);
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
  //IWidgetUrlHandler
  virtual bool widgetUrlOpen(IViewWidget *AWidget, const QUrl &AUrl, int AOrder);
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
  virtual bool tabWindowsEnabled() const;
  virtual void setTabWindowsEnabled(bool AEnabled);
  virtual QUuid defaultTabWindow() const;
  virtual void setDefaultTabWindow(const QUuid &AWindowId);
  virtual bool chatWindowShowStatus() const;
  virtual void setChatWindowShowStatus(bool AShow);
  virtual bool editorAutoResize() const;
  virtual void setEditorAutoResize(bool AResize);
  virtual bool showInfoWidgetInChatWindow() const;
  virtual void setShowInfoWidgetInChatWindow(bool AShow);
  virtual int editorMinimumLines() const;
  virtual void setEditorMinimumLines(int ALines);
  virtual QKeySequence editorSendKey() const;
  virtual void setEditorSendKey(const QKeySequence &AKey);
  virtual void insertUrlHandler(IWidgetUrlHandler *AHandler, int AOrder);
  virtual void removeUrlHandler(IWidgetUrlHandler *AHandler, int AOrder);
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
  void tabWindowsEnabledChanged(bool AEnabled);
  void defaultTabWindowChanged(const QUuid &AWindowId);
  void chatWindowShowStatusChanged(bool AShow);
  void editorAutoResizeChanged(bool AResize);
  void showInfoWidgetInChatWindowChanged(bool AShow);
  void editorMinimumLinesChanged(int ALines);
  void editorSendKeyChanged(const QKeySequence &AKey);
  void urlHandlerInserted(IWidgetUrlHandler *AHandler, int AOrder);
  void urlHandlerRemoved(IWidgetUrlHandler *AHandler, int AOrder);
signals:
  void optionsAccepted();
  void optionsRejected();
protected:
  void deleteWindows();
  void deleteStreamWindows(const Jid &AStreamJid);
protected slots:
  void onViewWidgetUrlClicked(const QUrl &AUrl);
  void onMessageWindowDestroyed();
  void onChatWindowDestroyed();
  void onTabWindowPageAdded(ITabWindowPage *APage);
  void onTabWindowDestroyed();
  void onStreamJidAboutToBeChanged(IXmppStream *AXmppStream, const Jid &AAfter);
  void onStreamRemoved(IXmppStream *AXmppStream);
  void onSettingsOpened();
  void onSettingsClosed();
private:
  IPluginManager *FPluginManager;
  IXmppStreams *FXmppStreams;
  ISettingsPlugin *FSettingsPlugin;
private:
  QList<ITabWindow *> FTabWindows;
  QList<IChatWindow *> FChatWindows;
  QList<IMessageWindow *> FMessageWindows;
  QObjectCleanupHandler FCleanupHandler;
private:
  QUuid FDefaultTabWindow;
  bool FTabWindowsEnabled;
  bool FChatWindowShowStatus;
  bool FEditorAutoResize;
  bool FShowInfoWidgetInChatWindow;
  int FEditorMinimumLines;
  QKeySequence FEditorSendKey;
private:
  QMap<QUuid, QString> FAvailTabWindows;
  QMultiMap<int,IWidgetUrlHandler *> FUrlHandlers;
};

#endif // MESSAGEWIDGETS_H
