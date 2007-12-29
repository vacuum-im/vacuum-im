#ifndef IMESSENGER_H
#define IMESSENGER_H

#include <QVariant>
#include <QFont>
#include <QTextDocument>
#include <QVBoxLayout>
#include <QToolBar>
#include <QTextEdit>
#include <QTextBrowser>
#include <QMainWindow>
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/irostersmodel.h"
#include "../../utils/jid.h"
#include "../../utils/message.h"
#include "../../utils/action.h"
#include "../../utils/toolbarchanger.h"

#define MESSENGER_UUID "{153A4638-B468-496f-B57C-9F30CEDFCC2E}"

class IInfoWidget : 
  public QWidget
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

class IViewWidget : 
  public QWidget
{
public:
  enum ShowKind {
    SingleMessage,
    ChatMessage
  };
public:
  virtual const Jid &streamJid() const =0;
  virtual void setStreamJid(const Jid &AStreamJid) =0;
  virtual const Jid &contactJid() const =0;
  virtual void setContactJid(const Jid &AContactJid) =0;
  virtual QTextBrowser *textBrowser() const =0;
  virtual QTextDocument *document() const =0;
  virtual ShowKind showKind() const =0;
  virtual void setShowKind(ShowKind AKind) =0;
  virtual void showMessage(const Message &AMessage) =0;
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

class IEditWidget : 
  public QWidget
{
public:
  virtual const Jid &streamJid() const =0;
  virtual void setStreamJid(const Jid &AStreamJid) =0;
  virtual const Jid &contactJid() const =0;
  virtual void setContactJid(const Jid &AContactJid) =0;
  virtual QTextEdit *textEdit() const =0;
  virtual QTextDocument *document() const =0;
  virtual void sendMessage() =0;
  virtual int sendMessageKey() const =0;
  virtual void setSendMessageKey(int AKey) =0;
  virtual void clearEditor() =0;
signals:
  virtual void messageAboutToBeSend() =0;
  virtual void messageReady() =0;
  virtual void streamJidChanged(const Jid &ABefour) =0;
  virtual void contactJidChanged(const Jid &ABefour) =0;
  virtual void sendMessageKeyChanged(int AKey) =0;
  virtual void editorCleared() =0;
};

class IReceiversWidget : 
  public QWidget
{
public:
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

class IToolBarWidget :
  public QToolBar
{
public:
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

class ITabWindow : 
  public QMainWindow
{
public:
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
  public QMainWindow,
  public ITabWidget
{
  Q_INTERFACES(ITabWidget);
public:
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
  public QMainWindow,
  public ITabWidget
{
  Q_INTERFACES(ITabWidget);
public:
  enum Mode {
    ReadMode                  =1,
    WriteMode                 =2
  };
public:
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

class IMessageHandler
{
public:
  virtual bool openWindow(IRosterIndex *AIndex) =0;
  virtual bool openWindow(const Jid &AStreamJid, const Jid &AContactJid, Message::MessageType AType) =0;
  virtual bool checkMessage(const Message &AMessage) =0;
  virtual bool notifyOptions(const Message &AMessage, QIcon &AIcon, QString &AToolTip, int &AFlags) =0;
  virtual void receiveMessage(int AMessageId) =0;
  virtual void showMessage(int AMessageId) =0;
};

class IMessageWriter
{
public:
  virtual void writeMessage(Message &AMessage, QTextDocument *ADocument, const QString &ALang, int AOrder) =0;
  virtual void writeText(Message &AMessage, QTextDocument *ADocument, const QString &ALang, int AOrder) =0;
};

class IResourceLoader
{
public:
  virtual void loadResource(int AType, const QUrl &AName, QVariant &AValue) =0;
};

class IMessenger
{
public:
  enum Option {
    OpenChatInTabWindow         =1,
    ShowHTML                    =2,
    ShowDateTime                =4,
    ShowStatus                  =16,
    OpenMessageWindow           =32,
    OpenChatWindow              =64
  };
public:
  virtual QObject *instance() = 0;
  virtual IPluginManager *pluginManager() const =0;
  virtual void insertMessageHandler(IMessageHandler *AHandler, int AOrder) =0;
  virtual void removeMessageHandler(IMessageHandler *AHandler, int AOrder) =0;
  virtual void insertMessageWriter(IMessageWriter *AWriter, int AOrder) =0;
  virtual void removeMessageWriter(IMessageWriter *AWriter, int AOrder) =0;
  virtual void insertResourceLoader(IResourceLoader *ALoader, int AOrder) =0;
  virtual void removeResourceLoader(IResourceLoader *ALoader, int AOrder) =0;
  virtual void textToMessage(Message &AMessage, const QTextDocument *ADocument, const QString &ALang = "") const =0;
  virtual void messageToText(QTextDocument *ADocument, const Message &AMessage, const QString &ALang = "") const =0;
  virtual bool openWindow(const Jid &AStreamJid, const Jid &AContactJid, Message::MessageType AType) const =0;
  virtual bool sendMessage(const Message &AMessage, const Jid &AStreamJid) =0;
  virtual int receiveMessage(const Message &AMessage) =0;
  virtual void showMessage(int AMessageId) =0;
  virtual void removeMessage(int AMessageId) =0;
  virtual Message messageById(int AMessageId) const =0;
  virtual QList<int> messages(const Jid &AStreamJid, const Jid &AFromJid = Jid(), int AMesTypes = Message::AnyType) =0;
  virtual bool checkOption(IMessenger::Option AOption) const =0;
  virtual void setOption(IMessenger::Option AOption, bool AValue) =0;
  //MessageWindows
  virtual QFont defaultChatFont() const =0;
  virtual void setDefaultChatFont(const QFont &AFont) =0;
  virtual QFont defaultMessageFont() const =0;
  virtual void setDefaultMessageFont(const QFont &AFont) =0;
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
signals:
  virtual void messageHandlerInserted(IMessageHandler *AHandler, int AOrder) =0;
  virtual void messageHandlerRemoved(IMessageHandler *AHandler, int AOrder) =0;
  virtual void messageWriterInserted(IMessageWriter *AWriter, int AOrder) =0;
  virtual void messageWriterRemoved(IMessageWriter *AWriter, int AOrder) =0;
  virtual void resourceLoaderInserted(IResourceLoader *ALoader, int AOrder) =0;
  virtual void resourceLoaderRemoved(IResourceLoader *ALoader, int AOrder) =0;
  virtual void messageNotified(int AMessageId) =0;
  virtual void messageUnNotified(int AMessageId) =0;
  virtual void messageReceive(Message &AMessage) =0;
  virtual void messageReceived(const Message &AMessage) =0;
  virtual void messageRemoved(const Message &AMessage) =0;
  virtual void messageSend(Message &AMessage) =0;
  virtual void messageSent(const Message &AMessage) =0;
  virtual void optionChanged(IMessenger::Option AOption, bool AValue) =0;
  virtual void optionsAccepted() =0;
  virtual void optionsRejected() =0;
  //MessageWindows
  virtual void defaultChatFontChanged(const QFont &AFont) =0;
  virtual void defaultMessageFontChanged(const QFont &AFont) =0;
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
Q_DECLARE_INTERFACE(IMessageHandler,"Vacuum.Plugin.IMessageHandler/1.0")
Q_DECLARE_INTERFACE(IMessageWriter,"Vacuum.Plugin.IMessageWriter/1.0")
Q_DECLARE_INTERFACE(IResourceLoader,"Vacuum.Plugin.IResourceLoader/1.0")
Q_DECLARE_INTERFACE(IMessenger,"Vacuum.Plugin.IMessenger/1.0")

#endif //IMESSENGER_H