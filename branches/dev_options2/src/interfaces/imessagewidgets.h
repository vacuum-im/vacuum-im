#ifndef IMESSAGEWIDGETS_H
#define IMESSAGEWIDGETS_H

#include <QUuid>
#include <QMultiMap>
#include <QTreeView>
#include <QTextBrowser>
#include <QTextDocument>
#include <interfaces/imainwindow.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imessagestylemanager.h>
#include <utils/jid.h>
#include <utils/menu.h>
#include <utils/action.h>
#include <utils/message.h>
#include <utils/splitterwidget.h>
#include <utils/menubarchanger.h>
#include <utils/toolbarchanger.h>
#include <utils/statusbarchanger.h>
#include <utils/advanceditemmodel.h>

#define MESSAGEWIDGETS_UUID "{89de35ee-bd44-49fc-8495-edd2cfebb685}"

class IMessageAddress
{
public:
	virtual QObject *instance() = 0;
	virtual Jid streamJid() const =0;
	virtual Jid contactJid() const =0;
	virtual bool isAutoAddresses() const =0;
	virtual void setAutoAddresses(bool AEnabled) =0;
	virtual QMultiMap<Jid, Jid> availAddresses(bool AUnique=false) const =0;
	virtual void setAddress(const Jid &AStreamJid, const Jid &AContactJid) =0;
	virtual void appendAddress(const Jid &AStreamJid, const Jid &AContactJid) =0;
	virtual void removeAddress(const Jid &AStreamJid, const Jid &AContactJid = Jid::null) =0;
protected:
	virtual void availAddressesChanged() =0;
	virtual void autoAddressesChanged(bool AEnabled) =0;
	virtual void addressChanged(const Jid &AStreamBefore, const Jid &AContactBefore) =0;
	virtual void streamJidChanged(const Jid &ABefore, const Jid &AAfter) =0;
};

class IMessageWindow;
class IMessageWidget
{
public:
	virtual QWidget *instance() = 0;
	virtual bool isVisibleOnWindow() const =0;
	virtual IMessageWindow *messageWindow() const =0;
};

class IMessageInfoWidget : 
	public IMessageWidget
{
public:
	enum Field {
		Avatar,
		Name,
		StatusIcon,
		StatusText,
		UserField = 16
	};
public:
	virtual QWidget *instance() = 0;
	virtual Menu *addressMenu() const =0;
	virtual bool isAddressMenuVisible() const =0;
	virtual void setAddressMenuVisible(bool AVisible) =0;
	virtual QVariant fieldValue(int AField) const =0;
	virtual void setFieldValue(int AField, const QVariant &AValue) =0;
	virtual ToolBarChanger *infoToolBarChanger() const =0;
protected:
	virtual void fieldValueChanged(int AField) =0;
	virtual void addressMenuVisibleChanged(bool AVisible) =0;
	virtual void addressMenuRequested(Menu *AMenu) =0;
	virtual void contextMenuRequested(Menu *AMenu) =0;
	virtual void toolTipsRequested(QMap<int,QString> &AToolTips) =0;
};

class IMessageViewWidget :
	public IMessageWidget
{
public:
	virtual QWidget *instance() = 0;
	virtual void clearContent() =0;
	virtual QWidget *styleWidget() const =0;
	virtual IMessageStyle *messageStyle() const =0;
	virtual void setMessageStyle(IMessageStyle *AStyle, const IMessageStyleOptions &AOptions) =0;
	virtual void appendHtml(const QString &AHtml, const IMessageStyleContentOptions &AOptions) =0;
	virtual void appendText(const QString &AText, const IMessageStyleContentOptions &AOptions) =0;
	virtual void appendMessage(const Message &AMessage, const IMessageStyleContentOptions &AOptions) =0;
	virtual void contextMenuForView(const QPoint &APosition, Menu *AMenu) =0;
	virtual QTextDocumentFragment selection() const =0;
	virtual QTextCharFormat textFormatAt(const QPoint &APosition) const =0;
	virtual QTextDocumentFragment textFragmentAt(const QPoint &APosition) const =0;
protected:
	virtual void urlClicked(const QUrl &AUrl) =0;
	virtual void viewContextMenu(const QPoint &APosition, Menu *AMenu) =0;
	virtual void contentAppended(const QString &AHtml, const IMessageStyleContentOptions &AOptions) =0;
	virtual void messageStyleOptionsChanged(const IMessageStyleOptions &AOptions, bool ACleared) =0;
	virtual void messageStyleChanged(IMessageStyle *ABefore, const IMessageStyleOptions &AOptions) =0;
};

class IMessageEditWidget :
	public IMessageWidget
{
public:
	virtual QWidget *instance() = 0;
	virtual QTextEdit *textEdit() const =0;
	virtual QTextDocument *document() const =0;
	virtual bool sendMessage() =0;
	virtual bool isSendEnabled() const =0;
	virtual void setSendEnabled(bool AEnabled) =0;
	virtual bool isEditEnabled() const =0;
	virtual void setEditEnabled(bool AEnabled) =0;
	virtual bool isAutoResize() const =0;
	virtual void setAutoResize(bool AAuto) =0;
	virtual int minimumHeightLines() const =0;
	virtual void setMinimumHeightLines(int ALines) =0;
	virtual bool isRichTextEnabled() const =0;
	virtual void setRichTextEnabled(bool AEnabled) =0;
	virtual bool isEditToolBarVisible() const =0;
	virtual void setEditToolBarVisible(bool AVisible) =0;
	virtual ToolBarChanger *editToolBarChanger() const =0;
	virtual QString sendShortcutId() const =0;
	virtual void setSendShortcutId(const QString &AShortcutId) =0;
	virtual void contextMenuForEdit(const QPoint &APosition, Menu *AMenu) =0;
	virtual void insertTextFragment(const QTextDocumentFragment &AFragment) =0;
	virtual QTextDocumentFragment prepareTextFragment(const QTextDocumentFragment &AFragment) =0;
protected:
	virtual void sendEnableChanged(bool AEnabled) =0;
	virtual void editEnableChanged(bool AEnabled) =0;
	virtual void autoResizeChanged(bool AAuto) =0;
	virtual void minimumHeightLinesChanged(int ALines) =0;
	virtual void richTextEnableChanged(bool AEnabled) =0;
	virtual void sendShortcutIdChanged(const QString &AShortcutId) =0;
	virtual void keyEventReceived(QKeyEvent *AKeyEvent, bool &AHook) =0;
	virtual void contextMenuRequested(const QPoint &APosition, Menu *AMenu) =0;
};

class IMessageReceiversWidget :
	public IMessageWidget
{
public:
	virtual QWidget *instance() = 0;
	virtual QList<Jid> availStreams() const =0;
	virtual QTreeView *receiversView() const =0;
	virtual AdvancedItemModel *receiversModel() const =0;
	virtual QModelIndex mapModelToView(QStandardItem *AItem) =0;
	virtual QStandardItem *mapViewToModel(const QModelIndex &AIndex) =0;
	virtual void contextMenuForItems(QList<QStandardItem *> AItems, Menu *AMenu) =0;
	virtual QMultiMap<Jid, Jid> selectedAddresses() const =0;
	virtual void setGroupSelection(const Jid &AStreamJid, const QString &AGroup, bool ASelected) =0;
	virtual void setAddressSelection(const Jid &AStreamJid, const Jid &AContactJid, bool ASelected) =0;
	virtual void clearSelection() =0;
protected:
	virtual void availStreamsChanged() =0;
	virtual void addressSelectionChanged() =0;
	virtual void contextMenuForItemsRequested(QList<QStandardItem *> AItems, Menu *AMenu) =0;
};

class IMessageMenuBarWidget :
	public IMessageWidget
{
public:
	virtual QMenuBar *instance() = 0;
	virtual MenuBarChanger *menuBarChanger() const =0;
};

class IMessageToolBarWidget :
	public IMessageWidget
{
public:
	virtual QToolBar *instance() = 0;
	virtual ToolBarChanger *toolBarChanger() const =0;
};

class IMessageStatusBarWidget :
	public IMessageWidget
{
public:
	virtual QStatusBar *instance() = 0;
	virtual StatusBarChanger *statusBarChanger() const =0;
};

struct IMessageTabPageNotify
{
	IMessageTabPageNotify() {
		priority = -1;
		blink = true;
	}
	int priority;
	bool blink;
	QIcon icon;
	QString caption;
	QString toolTip;
};

class IMessageTabPage;
class IMessageTabPageNotifier
{
public:
	virtual QObject *instance() =0;
	virtual IMessageTabPage *tabPage() const =0;
	virtual int activeNotify() const =0;
	virtual QList<int> notifies() const =0;
	virtual IMessageTabPageNotify notifyById(int ANotifyId) const =0;
	virtual int insertNotify(const IMessageTabPageNotify &ANotify) =0;
	virtual void removeNotify(int ANotifyId) =0;
protected:
	virtual void notifyInserted(int ANotifyId) =0;
	virtual void notifyRemoved(int ANotifyId) =0;
	virtual void activeNotifyChanged(int ANotifyId) =0;
};

class IMessageTabPage
{
public:
	virtual QWidget *instance() =0;
	virtual QString tabPageId() const =0;
	virtual bool isVisibleTabPage() const =0;
	virtual bool isActiveTabPage() const =0;
	virtual void assignTabPage() =0;
	virtual void showTabPage() =0;
	virtual void showMinimizedTabPage() =0;
	virtual void closeTabPage() =0;
	virtual QIcon tabPageIcon() const =0;
	virtual QString tabPageCaption() const =0;
	virtual QString tabPageToolTip() const =0;
	virtual IMessageTabPageNotifier *tabPageNotifier() const =0;
	virtual void setTabPageNotifier(IMessageTabPageNotifier *ANotifier) =0;
protected:
	virtual void tabPageAssign() =0;
	virtual void tabPageShow() =0;
	virtual void tabPageShowMinimized() =0;
	virtual void tabPageClose() =0;
	virtual void tabPageClosed() =0;
	virtual void tabPageChanged() =0;
	virtual void tabPageActivated() =0;
	virtual void tabPageDeactivated() =0;
	virtual void tabPageDestroyed() =0;
	virtual void tabPageNotifierChanged() =0;
};

class IMessageTabWindow :
	public IMainCentralPage
{
public:
	virtual QMainWindow *instance() = 0;
	virtual void showWindow() =0;
	virtual void showMinimizedWindow() =0;
	virtual QUuid windowId() const =0;
	virtual QString windowName() const =0;
	virtual Menu *windowMenu() const =0;
	virtual bool isTabBarVisible() const =0;
	virtual void setTabBarVisible(bool AVisible) =0;
	virtual bool isAutoCloseEnabled() const =0;
	virtual void setAutoCloseEnabled(bool AEnabled) =0;
	virtual int tabPageCount() const =0;
	virtual IMessageTabPage *tabPage(int AIndex) const =0;
	virtual void addTabPage(IMessageTabPage *APage) =0;
	virtual bool hasTabPage(IMessageTabPage *APage) const=0;
	virtual IMessageTabPage *currentTabPage() const =0;
	virtual void setCurrentTabPage(IMessageTabPage *APage) =0;
	virtual void detachTabPage(IMessageTabPage *APage) =0;
	virtual void removeTabPage(IMessageTabPage *APage) =0;
protected:
	virtual void currentTabPageChanged(IMessageTabPage *APage) =0;
	virtual void tabPageMenuRequested(IMessageTabPage *APage, Menu *AMenu) =0;
	virtual void tabPageAdded(IMessageTabPage *APage) =0;
	virtual void tabPageRemoved(IMessageTabPage *APage) =0;
	virtual void tabPageDetached(IMessageTabPage *APage) =0;
	virtual void windowChanged() =0;
	virtual void windowDestroyed() =0;
};

class IMessageWindow :
	public IMessageTabPage
{
public:
	virtual QWidget *instance() =0;
	virtual Jid streamJid() const =0;
	virtual Jid contactJid() const =0;
	virtual IMessageAddress *address() const =0;
	virtual IMessageInfoWidget *infoWidget() const =0;
	virtual IMessageViewWidget *viewWidget() const =0;
	virtual IMessageEditWidget *editWidget() const =0;
	virtual IMessageMenuBarWidget *menuBarWidget() const =0;
	virtual IMessageToolBarWidget *toolBarWidget() const =0;
	virtual IMessageStatusBarWidget *statusBarWidget() const =0;
	virtual IMessageReceiversWidget *receiversWidget() const =0;
protected:
	virtual void widgetLayoutChanged() =0;
};

class IMessageNormalWindow :
	public IMessageWindow
{
public:
	enum Mode {
		ReadMode    =0x01,
		WriteMode   =0x02
	};
public:
	virtual QMainWindow *instance() =0;
	virtual Mode mode() const =0;
	virtual void setMode(Mode AMode) =0;
	virtual QString subject() const =0;
	virtual void setSubject(const QString &ASubject) =0;
	virtual QString threadId() const =0;
	virtual void setThreadId(const QString &AThreadId) =0;
	virtual SplitterWidget *messageWidgetsBox() const =0;
	virtual void updateWindow(const QIcon &AIcon, const QString &ACaption, const QString &ATitle, const QString &AToolTip) =0;
protected:
	virtual void modeChanged(int AMode) =0;
};

class IMessageChatWindow :
	public IMessageWindow
{
public:
	virtual QMainWindow *instance() =0;
	virtual SplitterWidget *messageWidgetsBox() const =0;
	virtual void updateWindow(const QIcon &AIcon, const QString &ACaption, const QString &ATitle, const QString &AToolTip) =0;
};

class IMessageViewDropHandler
{
public:
	virtual bool messagaeViewDragEnter(IMessageViewWidget *AWidget, const QDragEnterEvent *AEvent) =0;
	virtual bool messageViewDragMove(IMessageViewWidget *AWidget, const QDragMoveEvent *AEvent) =0;
	virtual void messageViewDragLeave(IMessageViewWidget *AWidget, const QDragLeaveEvent *AEvent) =0;
	virtual bool messageViewDropAction(IMessageViewWidget *AWidget, const QDropEvent *AEvent, Menu *AMenu) =0;
};

class IMessageViewUrlHandler
{
public:
	virtual bool messageViewUrlOpen(int AOrder, IMessageViewWidget *AWidget, const QUrl &AUrl) =0;
};

class IMessageEditSendHandler
{
public:
	virtual bool messageEditSendPrepare(int AOrder, IMessageEditWidget *AWidget) =0;
	virtual bool messageEditSendProcesse(int AOrder, IMessageEditWidget *AWidget) =0;
};

class IMessageEditContentsHandler
{
public:
	virtual bool messageEditContentsCreate(int AOrder, IMessageEditWidget *AWidget, QMimeData *AData) =0;
	virtual bool messageEditContentsCanInsert(int AOrder, IMessageEditWidget *AWidget, const QMimeData *AData) =0;
	virtual bool messageEditContentsInsert(int AOrder, IMessageEditWidget *AWidget, const QMimeData *AData, QTextDocument *ADocument) =0;
	virtual bool messageEditContentsChanged(int AOrder, IMessageEditWidget *AWidget, int &APosition, int &ARemoved, int &AAdded) =0;
};

class IMessageWidgets
{
public:
	virtual QObject *instance() = 0;
	virtual IPluginManager *pluginManager() const =0;
	virtual IMessageAddress *newAddress(const Jid &AStreamJid, const Jid &AContactJid, QObject *AParent) =0;
	virtual IMessageInfoWidget *newInfoWidget(IMessageWindow *AWindow, QWidget *AParent) =0;
	virtual IMessageViewWidget *newViewWidget(IMessageWindow *AWindow, QWidget *AParent) =0;
	virtual IMessageEditWidget *newEditWidget(IMessageWindow *AWindow, QWidget *AParent) =0;
	virtual IMessageReceiversWidget *newReceiversWidget(IMessageWindow *AWindow, QWidget *AParent) =0;
	virtual IMessageMenuBarWidget *newMenuBarWidget(IMessageWindow *AWindow, QWidget *AParent) =0;
	virtual IMessageToolBarWidget *newToolBarWidget(IMessageWindow *AWindow, QWidget *AParent) =0;
	virtual IMessageStatusBarWidget *newStatusBarWidget(IMessageWindow *AWindow, QWidget *AParent) =0;
	virtual IMessageTabPageNotifier *newTabPageNotifier(IMessageTabPage *ATabPage) = 0;
	virtual QList<IMessageNormalWindow *> normalWindows() const =0;
	virtual IMessageNormalWindow *getNormalWindow(const Jid &AStreamJid, const Jid &AContactJid, IMessageNormalWindow::Mode AMode) =0;
	virtual IMessageNormalWindow *findNormalWindow(const Jid &AStreamJid, const Jid &AContactJid, bool AExact=false) const =0;
	virtual QList<IMessageChatWindow *> chatWindows() const =0;
	virtual IMessageChatWindow *getChatWindow(const Jid &AStreamJid, const Jid &AContactJid) =0;
	virtual IMessageChatWindow *findChatWindow(const Jid &AStreamJid, const Jid &AContactJid, bool AExact=false) const =0;
	virtual QList<QUuid> tabWindowList() const =0;
	virtual QUuid appendTabWindow(const QString &AName) =0;
	virtual void deleteTabWindow(const QUuid &AWindowId) =0;
	virtual QString tabWindowName(const QUuid &AWindowId) const =0;
	virtual void setTabWindowName(const QUuid &AWindowId, const QString &AName) =0;
	virtual QList<IMessageTabWindow *> tabWindows() const =0;
	virtual IMessageTabWindow *getTabWindow(const QUuid &AWindowId) =0;
	virtual IMessageTabWindow *findTabWindow(const QUuid &AWindowId) const =0;
	virtual void assignTabWindowPage(IMessageTabPage *APage) =0;
	virtual QList<IMessageViewDropHandler *> viewDropHandlers() const =0;
	virtual void insertViewDropHandler(IMessageViewDropHandler *AHandler) =0;
	virtual void removeViewDropHandler(IMessageViewDropHandler *AHandler) =0;
	virtual QMultiMap<int, IMessageViewUrlHandler *> viewUrlHandlers() const =0;
	virtual void insertViewUrlHandler(int AOrder, IMessageViewUrlHandler *AHandler) =0;
	virtual void removeViewUrlHandler(int AOrder, IMessageViewUrlHandler *AHandler) =0;
	virtual QMultiMap<int, IMessageEditSendHandler *> editSendHandlers() const =0;
	virtual void insertEditSendHandler(int AOrder, IMessageEditSendHandler *AHandler) =0;
	virtual void removeEditSendHandler(int AOrder, IMessageEditSendHandler *AHandler) =0;
	virtual QMultiMap<int, IMessageEditContentsHandler *> editContentsHandlers() const =0;
	virtual void insertEditContentsHandler(int AOrder, IMessageEditContentsHandler *AHandler) =0;
	virtual void removeEditContentsHandler( int AOrder, IMessageEditContentsHandler *AHandler) =0;
protected:
	virtual void addressCreated(IMessageAddress *AAddress) =0;
	virtual void infoWidgetCreated(IMessageInfoWidget *AInfoWidget) =0;
	virtual void viewWidgetCreated(IMessageViewWidget *AViewWidget) =0;
	virtual void editWidgetCreated(IMessageEditWidget *AEditWidget) =0;
	virtual void receiversWidgetCreated(IMessageReceiversWidget *AReceiversWidget) =0;
	virtual void menuBarWidgetCreated(IMessageMenuBarWidget *AMenuBarWidget) =0;
	virtual void toolBarWidgetCreated(IMessageToolBarWidget *AToolBarWidget) =0;
	virtual void statusBarWidgetCreated(IMessageStatusBarWidget *AStatusBarWidget) =0;
	virtual void tabPageNotifierCreated(IMessageTabPageNotifier *ANotifier) =0;
	virtual void normalWindowCreated(IMessageNormalWindow *AWindow) =0;
	virtual void normalWindowDestroyed(IMessageNormalWindow *AWindow) =0;
	virtual void chatWindowCreated(IMessageChatWindow *AWindow) =0;
	virtual void chatWindowDestroyed(IMessageChatWindow *AWindow) =0;
	virtual void tabWindowAppended(const QUuid &AWindowId, const QString &AName) =0;
	virtual void tabWindowNameChanged(const QUuid &AWindowId, const QString &AName) =0;
	virtual void tabWindowDeleted(const QUuid &AWindowId) =0;
	virtual void tabWindowCreated(IMessageTabWindow *AWindow) =0;
	virtual void tabWindowDestroyed(IMessageTabWindow *AWindow) =0;
};

Q_DECLARE_INTERFACE(IMessageAddress,"Vacuum.Plugin.IMessageAddress/1.0")
Q_DECLARE_INTERFACE(IMessageWidget,"Vacuum.Plugin.IMessageWidget/1.0")
Q_DECLARE_INTERFACE(IMessageInfoWidget,"Vacuum.Plugin.IMessageInfoWidget/1.2")
Q_DECLARE_INTERFACE(IMessageViewWidget,"Vacuum.Plugin.IMessageViewWidget/1.4")
Q_DECLARE_INTERFACE(IMessageEditWidget,"Vacuum.Plugin.IMessageEditWidget/1.4")
Q_DECLARE_INTERFACE(IMessageReceiversWidget,"Vacuum.Plugin.IMessageReceiversWidget/1.3")
Q_DECLARE_INTERFACE(IMessageMenuBarWidget,"Vacuum.Plugin.IMessageMenuBarWidget/1.1")
Q_DECLARE_INTERFACE(IMessageToolBarWidget,"Vacuum.Plugin.IMessageToolBarWidget/1.1")
Q_DECLARE_INTERFACE(IMessageStatusBarWidget,"Vacuum.Plugin.IMessageStatusBarWidget/1.1")
Q_DECLARE_INTERFACE(IMessageTabPageNotifier,"Vacuum.Plugin.IMessageTabPageNotifier/1.1")
Q_DECLARE_INTERFACE(IMessageTabPage,"Vacuum.Plugin.IMessageTabPage/1.4")
Q_DECLARE_INTERFACE(IMessageTabWindow,"Vacuum.Plugin.IMessageTabWindow/1.5")
Q_DECLARE_INTERFACE(IMessageWindow,"Vacuum.Plugin.IMessageWindow/1.3")
Q_DECLARE_INTERFACE(IMessageNormalWindow,"Vacuum.Plugin.IMessageNormalWindow/1.6")
Q_DECLARE_INTERFACE(IMessageChatWindow,"Vacuum.Plugin.IMessageChatWindow/1.6")
Q_DECLARE_INTERFACE(IMessageViewDropHandler,"Vacuum.Plugin.IMessageViewDropHandler/1.2")
Q_DECLARE_INTERFACE(IMessageViewUrlHandler,"Vacuum.Plugin.IMessageViewUrlHandler/1.2")
Q_DECLARE_INTERFACE(IMessageEditSendHandler,"QIP.Plugin.IMessageEditSendHandler/1.0")
Q_DECLARE_INTERFACE(IMessageEditContentsHandler,"Vacuum.Plugin.IMessageEditContentsHandler/1.3")
Q_DECLARE_INTERFACE(IMessageWidgets,"Vacuum.Plugin.IMessageWidgets/1.11")

#endif // IMESSAGEWIDGETS_H
