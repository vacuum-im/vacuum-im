#ifndef IMESSAGEWIDGETS_H
#define IMESSAGEWIDGETS_H

#include <QTextBrowser>
#include <QTextDocument>
#include "../../interfaces/ipluginmanager.h"
#include "../../utils/jid.h"
#include "../../utils/action.h"
#include "../../utils/message.h"
#include "../../utils/toolbarchanger.h"

#define MESSAGEWIDGETS_UUID "{89de35ee-bd44-49fc-8495-edd2cfebb685}"

class IInfoWidget
{
public:
  enum InfoField {
    AccountName       =1,
    ContactAvatar     =2,
    ContactName       =4,
    ContactShow       =8,
    ContactStatus     =16,
    ContactEmail      =32,
    ContactClient     =64
  };
public:
  virtual QWidget *instance() = 0;
  virtual const Jid &streamJid() const =0;
  virtual void setStreamJid(const Jid &AStreamJid) =0;
  virtual const Jid &contactJid() const =0;
  virtual void setContactJid(const Jid &AContactJid) =0;
  virtual void autoUpdateFields() =0;
  virtual void autoUpdateField(InfoField AField) =0;
  virtual QVariant field(InfoField AField) const =0;
  virtual void setField(InfoField AField, const QVariant &AValue) =0;
  virtual bool isFiledAutoUpdated(IInfoWidget::InfoField AField) const =0;
  virtual int autoUpdatedFields() const =0;
  virtual void setFieldAutoUpdated(IInfoWidget::InfoField AField, bool AAuto) =0;
  virtual bool isFieldVisible(IInfoWidget::InfoField AField) const =0;
  virtual int visibleFields() const =0;
  virtual void setFieldVisible(IInfoWidget::InfoField AField, bool AVisible) =0;
signals:
  virtual void streamJidChanged(const Jid &ABefour) =0;
  virtual void contactJidChanged(const Jid &ABefour) =0;
  virtual void fieldChanged(IInfoWidget::InfoField AField, const QVariant &AValue) =0;
};

class IViewWidget
{
public:
  enum ShowKind {
    NormalMessage,
    ChatMessage,
    GroupChatMessage
  };
public:
  virtual QWidget *instance() = 0;
  virtual const Jid &streamJid() const =0;
  virtual void setStreamJid(const Jid &AStreamJid) =0;
  virtual const Jid &contactJid() const =0;
  virtual void setContactJid(const Jid &AContactJid) =0;
  virtual QTextBrowser *textBrowser() const =0;
  virtual QTextDocument *document() const =0;
  virtual ShowKind showKind() const =0;
  virtual void setShowKind(ShowKind AKind) =0;
  virtual void showMessage(const Message &AMessage) =0;
  virtual void showCustomMessage(const QString &AHtml, const QDateTime &ATime=QDateTime(),
    const QString &ANick="", const QColor &ANickColor=Qt::blue) =0;
  virtual void showCustomHtml(const QString &AHtml) =0;
  virtual QColor colorForJid(const Jid &AJid) const =0;
  virtual void setColorForJid(const Jid &AJid, const QColor &AColor) =0;
  virtual QString nickForJid(const Jid &AJid) const =0;
  virtual void setNickForJid(const Jid &AJid, const QString &ANick) =0;
signals:
  virtual void messageShown(const Message &AMessage) =0;
  virtual void customHtmlShown(const QString &AHtml) =0;
  virtual void streamJidChanged(const Jid &ABefour) =0;
  virtual void contactJidChanged(const Jid &ABefour) =0;
  virtual void colorForJidChanged(const Jid &AJid, const QColor &AColor) =0;
  virtual void nickForJidChanged(const Jid &AJid, const QString &ANick) =0;
};

class IEditWidget
{
public:
  virtual QWidget *instance() = 0;
  virtual const Jid &streamJid() const =0;
  virtual void setStreamJid(const Jid &AStreamJid) =0;
  virtual const Jid &contactJid() const =0;
  virtual void setContactJid(const Jid &AContactJid) =0;
  virtual QTextEdit *textEdit() const =0;
  virtual QTextDocument *document() const =0;
  virtual void sendMessage() =0;
  virtual QKeySequence sendMessageKey() const =0;
  virtual void setSendMessageKey(const QKeySequence &AKey) =0;
  virtual void clearEditor() =0;
signals:
  virtual void keyEventReceived(QKeyEvent *AKeyEvent, bool &AHook) =0;
  virtual void messageAboutToBeSend() =0;
  virtual void messageReady() =0;
  virtual void streamJidChanged(const Jid &ABefour) =0;
  virtual void contactJidChanged(const Jid &ABefour) =0;
  virtual void sendMessageKeyChanged(const QKeySequence &AKey) =0;
  virtual void editorCleared() =0;
};

class IReceiversWidget
{
public:
  virtual QWidget *instance() = 0;
  virtual const Jid &streamJid() const =0;
  virtual void setStreamJid(const Jid &AStreamJid) =0;
  virtual QList<Jid> receivers() const =0;
  virtual QString receiverName(const Jid &AReceiver) const =0;
  virtual void addReceiversGroup(const QString &AGroup) =0;
  virtual void removeReceiversGroup(const QString &AGroup) =0;
  virtual void addReceiver(const Jid &AReceiver) =0;
  virtual void removeReceiver(const Jid &AReceiver) =0;
  virtual void clear() =0;
signals:
  virtual void streamJidChanged(const Jid &ABefour) =0;
  virtual void receiverAdded(const Jid &AReceiver) =0;
  virtual void receiverRemoved(const Jid &AReceiver) =0;
};

class IToolBarWidget
{
public:
  virtual QToolBar *instance() = 0;
  virtual ToolBarChanger *toolBarChanger() const =0;
  virtual IInfoWidget *infoWidget() const =0;
  virtual IViewWidget *viewWidget() const =0;
  virtual IEditWidget *editWidget() const =0;
  virtual IReceiversWidget *receiversWidget() const =0;
};

class ITabWidget
{
public:
  virtual QWidget *instance() =0;
  virtual void showWindow() =0;
  virtual void closeWindow() =0;
signals:
  virtual void windowShow() =0;
  virtual void windowClose() =0;
  virtual void windowChanged() =0;
  virtual void windowDestroyed() =0;
};

class ITabWindow
{
public:
  virtual QMainWindow *instance() = 0;
  virtual void showWindow() =0;
  virtual int windowId() const =0;
  virtual void addWidget(ITabWidget *AWidget) =0;
  virtual bool hasWidget(ITabWidget *AWidget) const=0;
  virtual ITabWidget *currentWidget() const =0;
  virtual void setCurrentWidget(ITabWidget *AWidget) =0;
  virtual void removeWidget(ITabWidget *AWidget) =0;
signals:
  virtual void widgetAdded(ITabWidget *AWidget) =0;
  virtual void currentChanged(ITabWidget *AWidget) =0;
  virtual void widgetRemoved(ITabWidget *AWidget) =0;
  virtual void windowChanged() =0;
  virtual void windowDestroyed() =0;
};

class IChatWindow :
  public ITabWidget
{
public:
  //virtual QMainWindow *instance() = 0;
  virtual const Jid &streamJid() const =0;
  virtual const Jid &contactJid() const =0;
  virtual void setContactJid(const Jid &AContactJid) =0;
  virtual IInfoWidget *infoWidget() const =0;
  virtual IViewWidget *viewWidget() const =0;
  virtual IEditWidget *editWidget() const =0;
  virtual IToolBarWidget *toolBarWidget() const =0;
  virtual bool isActive() const =0;
  virtual void showMessage(const Message &AMessage) =0;
  virtual void updateWindow(const QIcon &AIcon, const QString &AIconText, const QString &ATitle) =0;
signals:
  virtual void messageReady() =0;
  virtual void streamJidChanged(const Jid &ABefour) =0;
  virtual void contactJidChanged(const Jid &ABefour) =0;
  virtual void windowActivated() =0;
  virtual void windowClosed() =0;
};

class IMessageWindow :
  public ITabWidget
{
public:
  enum Mode {
    ReadMode    =1,
    WriteMode   =2
  };
public:
  //virtual QMainWindow *instance() = 0;
  virtual const Jid &streamJid() const =0;
  virtual const Jid &contactJid() const =0;
  virtual void setContactJid(const Jid &AContactJid) =0;
  virtual void addTabWidget(QWidget *AWidget) =0;
  virtual void setCurrentTabWidget(QWidget *AWidget) =0;
  virtual void removeTabWidget(QWidget *AWidget) =0;
  virtual IInfoWidget *infoWidget() const =0;
  virtual IViewWidget *viewWidget() const =0;
  virtual IEditWidget *editWidget() const =0;
  virtual IReceiversWidget *receiversWidget() const =0;
  virtual IToolBarWidget *viewToolBarWidget() const =0;
  virtual IToolBarWidget *editToolBarWidget() const =0;
  virtual Mode mode() const =0;
  virtual void setMode(Mode AMode) =0;
  virtual Message currentMessage() const =0;
  virtual QString subject() const =0;
  virtual void setSubject(const QString &ASubject) =0;
  virtual QString threadId() const =0;
  virtual void setThreadId(const QString &AThreadId) =0;
  virtual int nextCount() const =0;
  virtual void setNextCount(int ACount) =0;
  virtual void showMessage(const Message &AMessage) =0;
  virtual void updateWindow(const QIcon &AIcon, const QString &AIconText, const QString &ATitle) =0;
signals:
  virtual void messageReady() =0;
  virtual void showNextMessage() =0;
  virtual void replyMessage() =0;
  virtual void forwardMessage() =0;
  virtual void showChatWindow() =0;
  virtual void streamJidChanged(const Jid &ABefour) =0;
  virtual void contactJidChanged(const Jid &ABefour) =0;
  virtual void windowClosed() =0;
};

class IMessageResource
{
public:
  virtual void loadTextResource(int AType, const QUrl &AName, QVariant &AValue) =0;
};

class IMessageWidgets 
{
public:
  enum Option {
    UseTabWindow                =0x01,
    ShowHTML                    =0x02,
    ShowDateTime                =0x04,
    ShowStatus                  =0x08,
  };
public:
  virtual QObject *instance() = 0;
  virtual IPluginManager *pluginManager() const =0;
  virtual QFont defaultChatFont() const =0;
  virtual void setDefaultChatFont(const QFont &AFont) =0;
  virtual QFont defaultMessageFont() const =0;
  virtual void setDefaultMessageFont(const QFont &AFont) =0;
  virtual QKeySequence sendMessageKey() const =0;
  virtual void setSendMessageKey(const QKeySequence &AKey) =0;
  virtual IInfoWidget *newInfoWidget(const Jid &AStreamJid, const Jid &AContactJid) =0;
  virtual IViewWidget *newViewWidget(const Jid &AStreamJid, const Jid &AContactJid) =0;
  virtual IEditWidget *newEditWidget(const Jid &AStreamJid, const Jid &AContactJid) =0;
  virtual IReceiversWidget *newReceiversWidget(const Jid &AStreamJid) =0;
  virtual IToolBarWidget *newToolBarWidget(IInfoWidget *AInfo, IViewWidget *AView, IEditWidget *AEdit, IReceiversWidget *AReceivers) =0;
  virtual QList<IMessageWindow *> messageWindows() const =0;
  virtual IMessageWindow *newMessageWindow(const Jid &AStreamJid, const Jid &AContactJid, IMessageWindow::Mode AMode) =0;
  virtual IMessageWindow *findMessageWindow(const Jid &AStreamJid, const Jid &AContactJid) =0;
  virtual QList<IChatWindow *> chatWindows() const =0;
  virtual IChatWindow *newChatWindow(const Jid &AStreamJid, const Jid &AContactJid) =0;
  virtual IChatWindow *findChatWindow(const Jid &AStreamJid, const Jid &AContactJid) =0;
  virtual QList<int> tabWindows() const =0;
  virtual ITabWindow *openTabWindow(int AWindowId = 0) =0;
  virtual ITabWindow *findTabWindow(int AWindowId = 0) =0;
  virtual bool checkOption(IMessageWidgets::Option AOption) const =0;
  virtual void setOption(IMessageWidgets::Option AOption, bool AValue) =0;
  virtual void insertResourceLoader(IMessageResource *ALoader, int AOrder) =0;
  virtual void removeResourceLoader(IMessageResource *ALoader, int AOrder) =0;
signals:
  virtual void defaultChatFontChanged(const QFont &AFont) =0;
  virtual void defaultMessageFontChanged(const QFont &AFont) =0;
  virtual void sendMessageKeyChanged(const QKeySequence &AKey) =0;
  virtual void infoWidgetCreated(IInfoWidget *AInfoWidget) =0;
  virtual void viewWidgetCreated(IViewWidget *AViewWidget) =0;
  virtual void editWidgetCreated(IEditWidget *AEditWidget) =0;
  virtual void receiversWidgetCreated(IReceiversWidget *AReceiversWidget) =0;
  virtual void toolBarWidgetCreated(IToolBarWidget *AToolBarWidget) =0;
  virtual void messageWindowCreated(IMessageWindow *AWindow) =0;
  virtual void messageWindowDestroyed(IMessageWindow *AWindow) =0;
  virtual void chatWindowCreated(IChatWindow *AWindow) =0;
  virtual void chatWindowDestroyed(IChatWindow *AWindow) =0;
  virtual void tabWindowCreated(ITabWindow *AWindow) =0;
  virtual void tabWindowDestroyed(ITabWindow *AWindow) =0;
  virtual void optionChanged(IMessageWidgets::Option AOption, bool AValue) =0;
  virtual void resourceLoaderInserted(IMessageResource *ALoader, int AOrder) =0;
  virtual void resourceLoaderRemoved(IMessageResource *ALoader, int AOrder) =0;
};

Q_DECLARE_INTERFACE(IInfoWidget,"Vacuum.Plugin.IInfoWidget/1.0")
Q_DECLARE_INTERFACE(IViewWidget,"Vacuum.Plugin.IViewWidget/1.0")
Q_DECLARE_INTERFACE(IEditWidget,"Vacuum.Plugin.IEditWidget/1.0")
Q_DECLARE_INTERFACE(IReceiversWidget,"Vacuum.Plugin.IReceiversWidget/1.0")
Q_DECLARE_INTERFACE(IToolBarWidget,"Vacuum.Plugin.IToolBarWidget/1.0")
Q_DECLARE_INTERFACE(ITabWidget,"Vacuum.Plugin.ITabWidget/1.0")
Q_DECLARE_INTERFACE(ITabWindow,"Vacuum.Plugin.ITabWindow/1.0")
Q_DECLARE_INTERFACE(IChatWindow,"Vacuum.Plugin.IChatWindow/1.0")
Q_DECLARE_INTERFACE(IMessageWindow,"Vacuum.Plugin.IMessageWindow/1.0")
Q_DECLARE_INTERFACE(IMessageResource,"Vacuum.Plugin.IMessageResource/1.0")
Q_DECLARE_INTERFACE(IMessageWidgets,"Vacuum.Plugin.IMessageWidgets/1.0")

#endif
