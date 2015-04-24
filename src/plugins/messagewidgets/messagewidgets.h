#ifndef MESSAGEWIDGETS_H
#define MESSAGEWIDGETS_H

#include <QDesktopServices>
#include <QObjectCleanupHandler>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/imainwindow.h>
#include "address.h"
#include "infowidget.h"
#include "editwidget.h"
#include "viewwidget.h"
#include "receiverswidget.h"
#include "menubarwidget.h"
#include "toolbarwidget.h"
#include "statusbarwidget.h"
#include "normalwindow.h"
#include "chatwindow.h"
#include "tabwindow.h"
#include "tabpagenotifier.h"

class MessageWidgets :
	public QObject,
	public IPlugin,
	public IMessageWidgets,
	public IOptionsDialogHolder,
	public IMessageViewUrlHandler,
	public IMessageEditContentsHandler
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IMessageWidgets IOptionsDialogHolder IMessageViewUrlHandler IMessageEditContentsHandler);
	Q_PLUGIN_METADATA(IID "org.vacuum-im.plugins.MessageWidgets");
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
	virtual QMultiMap<int, IOptionsDialogWidget *> optionsDialogWidgets(const QString &ANodeId, QWidget *AParent);
	//IMessageViewUrlHandler
	virtual bool messageViewUrlOpen(int AOrder, IMessageViewWidget *AWidget, const QUrl &AUrl);
	//IMessageEditContentsHandler
	virtual bool messageEditContentsCreate(int AOrder, IMessageEditWidget *AWidget, QMimeData *AData);
	virtual bool messageEditContentsCanInsert(int AOrder, IMessageEditWidget *AWidget, const QMimeData *AData);
	virtual bool messageEditContentsInsert(int AOrder, IMessageEditWidget *AWidget, const QMimeData *AData, QTextDocument *ADocument);
	virtual bool messageEditContentsChanged(int AOrder, IMessageEditWidget *AWidget, int &APosition, int &ARemoved, int &AAdded);
	//IMessageWidgets
	virtual IMessageAddress *newAddress(const Jid &AStreamJid, const Jid &AContactJid, QObject *AParent);
	virtual IMessageInfoWidget *newInfoWidget(IMessageWindow *AWindow, QWidget *AParent);
	virtual IMessageViewWidget *newViewWidget(IMessageWindow *AWindow, QWidget *AParent);
	virtual IMessageEditWidget *newEditWidget(IMessageWindow *AWindow, QWidget *AParent);
	virtual IMessageReceiversWidget *newReceiversWidget(IMessageWindow *AWindow, QWidget *AParent);
	virtual IMessageMenuBarWidget *newMenuBarWidget(IMessageWindow *AWindow, QWidget *AParent);
	virtual IMessageToolBarWidget *newToolBarWidget(IMessageWindow *AWindow, QWidget *AParent);
	virtual IMessageStatusBarWidget *newStatusBarWidget(IMessageWindow *AWindow, QWidget *AParent);
	virtual IMessageTabPageNotifier *newTabPageNotifier(IMessageTabPage *ATabPage);
	virtual QList<IMessageNormalWindow *> normalWindows() const;
	virtual IMessageNormalWindow *getNormalWindow(const Jid &AStreamJid, const Jid &AContactJid, IMessageNormalWindow::Mode AMode);
	virtual IMessageNormalWindow *findNormalWindow(const Jid &AStreamJid, const Jid &AContactJid, bool AExact=false) const;
	virtual QList<IMessageChatWindow *> chatWindows() const;
	virtual IMessageChatWindow *getChatWindow(const Jid &AStreamJid, const Jid &AContactJid);
	virtual IMessageChatWindow *findChatWindow(const Jid &AStreamJid, const Jid &AContactJid, bool AExact=false) const;
	virtual QList<QUuid> tabWindowList() const;
	virtual QUuid appendTabWindow(const QString &AName);
	virtual void deleteTabWindow(const QUuid &AWindowId);
	virtual QString tabWindowName(const QUuid &AWindowId) const;
	virtual void setTabWindowName(const QUuid &AWindowId, const QString &AName);
	virtual QList<IMessageTabWindow *> tabWindows() const;
	virtual IMessageTabWindow *getTabWindow(const QUuid &AWindowId);
	virtual IMessageTabWindow *findTabWindow(const QUuid &AWindowId) const;
	virtual void assignTabWindowPage(IMessageTabPage *APage);
	virtual QList<IMessageViewDropHandler *> viewDropHandlers() const;
	virtual void insertViewDropHandler(IMessageViewDropHandler *AHandler);
	virtual void removeViewDropHandler(IMessageViewDropHandler *AHandler);
	virtual QMultiMap<int, IMessageViewUrlHandler *> viewUrlHandlers() const;
	virtual void insertViewUrlHandler(int AOrder, IMessageViewUrlHandler *AHandler);
	virtual void removeViewUrlHandler(int AOrder, IMessageViewUrlHandler *AHandler);
	virtual QMultiMap<int, IMessageEditSendHandler *> editSendHandlers() const;
	virtual void insertEditSendHandler(int AOrder, IMessageEditSendHandler *AHandler);
	virtual void removeEditSendHandler(int AOrder, IMessageEditSendHandler *AHandler);
	virtual QMultiMap<int, IMessageEditContentsHandler *> editContentsHandlers() const;
	virtual void insertEditContentsHandler(int AOrder, IMessageEditContentsHandler *AHandler);
	virtual void removeEditContentsHandler(int AOrder, IMessageEditContentsHandler *AHandler);
signals:
	void addressCreated(IMessageAddress *AAddress);
	void infoWidgetCreated(IMessageInfoWidget *AInfoWidget);
	void viewWidgetCreated(IMessageViewWidget *AViewWidget);
	void editWidgetCreated(IMessageEditWidget *AEditWidget);
	void receiversWidgetCreated(IMessageReceiversWidget *AReceiversWidget);
	void menuBarWidgetCreated(IMessageMenuBarWidget *AMenuBarWidget);
	void toolBarWidgetCreated(IMessageToolBarWidget *AToolBarWidget);
	void statusBarWidgetCreated(IMessageStatusBarWidget *AStatusBarWidget);
	void tabPageNotifierCreated(IMessageTabPageNotifier *ANotifier);
	void normalWindowCreated(IMessageNormalWindow *AWindow);
	void normalWindowDestroyed(IMessageNormalWindow *AWindow);
	void chatWindowCreated(IMessageChatWindow *AWindow);
	void chatWindowDestroyed(IMessageChatWindow *AWindow);
	void tabWindowAppended(const QUuid &AWindowId, const QString &AName);
	void tabWindowNameChanged(const QUuid &AWindowId, const QString &AName);
	void tabWindowDeleted(const QUuid &AWindowId);
	void tabWindowCreated(IMessageTabWindow *AWindow);
	void tabWindowDestroyed(IMessageTabWindow *AWindow);
protected:
	void deleteTabWindows();
	void insertToolBarQuoteAction(IMessageToolBarWidget *AWidget);
	Action *createQuouteAction(IMessageWindow *AWindow, QObject *AParent);
protected slots:
	void onViewWidgetContextMenu(const QPoint &APosition, Menu *AMenu);
	void onViewContextCopyActionTriggered(bool);
	void onViewContextUrlActionTriggered(bool);
	void onViewContextSearchActionTriggered(bool);
	void onMessageWindowWidgetLayoutChanged();
	void onQuoteActionTriggered(bool);
	void onAssignedTabPageDestroyed();
	void onNormalWindowDestroyed();
	void onChatWindowDestroyed();
	void onTabWindowPageAdded(IMessageTabPage *APage);
	void onTabWindowCurrentPageChanged(IMessageTabPage *APage);
	void onTabWindowDestroyed();
	void onOptionsOpened();
	void onOptionsClosed();
	void onOptionsChanged(const OptionsNode &ANode);
private:
	IMainWindow *FMainWindow;
	IOptionsManager *FOptionsManager;
private:
	QList<IMessageTabWindow *> FTabWindows;
	QList<IMessageChatWindow *> FChatWindows;
	QList<IMessageNormalWindow *> FNormalWindows;
	QObjectCleanupHandler FCleanupHandler;
private:
	QList<IMessageTabPage *> FAssignedPages;
	QMap<QString, QUuid> FPageWindows;
	QList<IMessageViewDropHandler *> FViewDropHandlers;
	QMultiMap<int,IMessageViewUrlHandler *> FViewUrlHandlers;
	QMultiMap<int,IMessageEditSendHandler *> FEditSendHandlers;
	QMultiMap<int,IMessageEditContentsHandler *> FEditContentsHandlers;
};

#endif // MESSAGEWIDGETS_H
