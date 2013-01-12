#ifndef MESSAGEWIDGETS_H
#define MESSAGEWIDGETS_H

#include <QDesktopServices>
#include <QObjectCleanupHandler>
#include <definitions/actiongroups.h>
#include <definitions/optionvalues.h>
#include <definitions/optionnodes.h>
#include <definitions/optionnodeorders.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/viewurlhandlerorders.h>
#include <definitions/editcontentshandlerorders.h>
#include <definitions/toolbargroups.h>
#include <definitions/shortcuts.h>
#include <definitions/shortcutgrouporders.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/imainwindow.h>
#include <utils/options.h>
#include <utils/shortcuts.h>
#include <utils/textmanager.h>
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
#include "tabpagenotifier.h"

class MessageWidgets :
	public QObject,
	public IPlugin,
	public IMessageWidgets,
	public IOptionsHolder,
	public IViewUrlHandler,
	public IEditContentsHandler
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IMessageWidgets IOptionsHolder IViewUrlHandler IEditContentsHandler);
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
	virtual QMultiMap<int, IOptionsWidget *> optionsWidgets(const QString &ANodeId, QWidget *AParent);
	//IViewUrlHandler
	virtual bool viewUrlOpen(int AOrder, IViewWidget *AWidget, const QUrl &AUrl);
	//IEditContentsHandler
	virtual bool editContentsCreate(int AOrder, IEditWidget *AWidget, QMimeData *AData);
	virtual bool editContentsCanInsert(int AOrder, IEditWidget *AWidget, const QMimeData *AData);
	virtual bool editContentsInsert(int AOrder, IEditWidget *AWidget, const QMimeData *AData, QTextDocument *ADocument);
	virtual bool editContentsChanged(int AOrder, IEditWidget *AWidget, int &APosition, int &ARemoved, int &AAdded);
	//IMessageWidgets
	virtual IPluginManager *pluginManager() const { return FPluginManager; }
	virtual IInfoWidget *newInfoWidget(const Jid &AStreamJid, const Jid &AContactJid, QWidget *AParent);
	virtual IViewWidget *newViewWidget(const Jid &AStreamJid, const Jid &AContactJid, QWidget *AParent);
	virtual IEditWidget *newEditWidget(const Jid &AStreamJid, const Jid &AContactJid, QWidget *AParent);
	virtual IReceiversWidget *newReceiversWidget(const Jid &AStreamJid, QWidget *AParent);
	virtual IMenuBarWidget *newMenuBarWidget(IInfoWidget *AInfo, IViewWidget *AView, IEditWidget *AEdit, IReceiversWidget *AReceivers, QWidget *AParent);
	virtual IToolBarWidget *newToolBarWidget(IInfoWidget *AInfo, IViewWidget *AView, IEditWidget *AEdit, IReceiversWidget *AReceivers, QWidget *AParent);
	virtual IStatusBarWidget *newStatusBarWidget(IInfoWidget *AInfo, IViewWidget *AView, IEditWidget *AEdit, IReceiversWidget *AReceivers, QWidget *AParent);
	virtual ITabPageNotifier *newTabPageNotifier(ITabPage *ATabPage);
	virtual QList<IMessageWindow *> messageWindows() const;
	virtual IMessageWindow *getMessageWindow(const Jid &AStreamJid, const Jid &AContactJid, IMessageWindow::Mode AMode);
	virtual IMessageWindow *findMessageWindow(const Jid &AStreamJid, const Jid &AContactJid) const;
	virtual QList<IChatWindow *> chatWindows() const;
	virtual IChatWindow *getChatWindow(const Jid &AStreamJid, const Jid &AContactJid);
	virtual IChatWindow *findChatWindow(const Jid &AStreamJid, const Jid &AContactJid) const;
	virtual QList<QUuid> tabWindowList() const;
	virtual QUuid appendTabWindow(const QString &AName);
	virtual void deleteTabWindow(const QUuid &AWindowId);
	virtual QString tabWindowName(const QUuid &AWindowId) const;
	virtual void setTabWindowName(const QUuid &AWindowId, const QString &AName);
	virtual QList<ITabWindow *> tabWindows() const;
	virtual ITabWindow *getTabWindow(const QUuid &AWindowId);
	virtual ITabWindow *findTabWindow(const QUuid &AWindowId) const;
	virtual void assignTabWindowPage(ITabPage *APage);
	virtual QList<IViewDropHandler *> viewDropHandlers() const;
	virtual void insertViewDropHandler(IViewDropHandler *AHandler);
	virtual void removeViewDropHandler(IViewDropHandler *AHandler);
	virtual QMultiMap<int, IViewUrlHandler *> viewUrlHandlers() const;
	virtual void insertViewUrlHandler(int AOrder, IViewUrlHandler *AHandler);
	virtual void removeViewUrlHandler(int AOrder, IViewUrlHandler *AHandler);
	virtual QMultiMap<int, IEditContentsHandler *> editContentsHandlers() const;
	virtual void insertEditContentsHandler(int AOrder, IEditContentsHandler *AHandler);
	virtual void removeEditContentsHandler(int AOrder, IEditContentsHandler *AHandler);
signals:
	void infoWidgetCreated(IInfoWidget *AInfoWidget);
	void viewWidgetCreated(IViewWidget *AViewWidget);
	void editWidgetCreated(IEditWidget *AEditWidget);
	void receiversWidgetCreated(IReceiversWidget *AReceiversWidget);
	void menuBarWidgetCreated(IMenuBarWidget *AMenuBarWidget);
	void toolBarWidgetCreated(IToolBarWidget *AToolBarWidget);
	void statusBarWidgetCreated(IStatusBarWidget *AStatusBarWidget);
	void tabPageNotifierCreated(ITabPageNotifier *ANotifier);
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
	void viewUrlHandlerInserted(int AOrder, IViewUrlHandler *AHandler);
	void viewUrlHandlerRemoved(int AOrder, IViewUrlHandler *AHandler);
	void editContentsHandlerInserted(int AOrder, IEditContentsHandler *AHandler);
	void editContentsHandlerRemoved(int AOrder, IEditContentsHandler *AHandler);
protected:
	void insertQuoteAction(IToolBarWidget *AWidget);
	void deleteWindows();
	void deleteStreamWindows(const Jid &AStreamJid);
protected slots:
	void onViewWidgetUrlClicked(const QUrl &AUrl);
	void onViewWidgetContextMenu(const QPoint &APosition, const QTextDocumentFragment &AText, Menu *AMenu);
	void onViewContextCopyActionTriggered(bool);
	void onViewContextUrlActionTriggered(bool);
	void onViewContextSearchActionTriggered(bool);
	void onEditWidgetCreateDataRequest(QMimeData *AData);
	void onEditWidgetCanInsertDataRequest(const QMimeData *AData, bool &ACanInsert);
	void onEditWidgetInsertDataRequest(const QMimeData *AData, QTextDocument *ADocument);
	void onEditWidgetContentsChanged(int APosition, int ARemoved, int AAdded);
	void onQuoteActionTriggered(bool);
	void onAssignedTabPageDestroyed();
	void onMessageWindowDestroyed();
	void onChatWindowDestroyed();
	void onTabWindowPageAdded(ITabPage *APage);
	void onTabWindowCurrentPageChanged(ITabPage *APage);
	void onTabWindowDestroyed();
	void onShortcutActivated(const QString &AId, QWidget *AWidget);
	void onStreamJidAboutToBeChanged(IXmppStream *AXmppStream, const Jid &AAfter);
	void onStreamRemoved(IXmppStream *AXmppStream);
	void onOptionsOpened();
	void onOptionsClosed();
	void onOptionsChanged(const OptionsNode &ANode);
private:
	IPluginManager *FPluginManager;
	IMainWindow *FMainWindow;
	IXmppStreams *FXmppStreams;
	IOptionsManager *FOptionsManager;
private:
	QList<ITabWindow *> FTabWindows;
	QList<IChatWindow *> FChatWindows;
	QList<IMessageWindow *> FMessageWindows;
	QObjectCleanupHandler FCleanupHandler;
private:
	QList<ITabPage *> FAssignedPages;
	QMap<QString, QUuid> FPageWindows;
	QList<IViewDropHandler *> FViewDropHandlers;
	QMultiMap<int,IViewUrlHandler *> FViewUrlHandlers;
	QMultiMap<int,IEditContentsHandler *> FEditContentsHandlers;
};

#endif // MESSAGEWIDGETS_H
