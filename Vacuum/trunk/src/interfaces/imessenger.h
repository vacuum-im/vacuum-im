#ifndef IMESSENGER_H
#define IMESSENGER_H

#include <QVariant>
#include <QTextDocument>
#include <QVBoxLayout>
#include <QToolBar>
#include <QTextEdit>
#include <QMainWindow>
#include "../../interfaces/ipluginmanager.h"
#include "../../utils/jid.h"
#include "../../utils/message.h"
#include "../../utils/action.h"
#include "../../utils/toolbarchanger.h"

#define MESSENGER_UUID "{153A4638-B468-496f-B57C-9F30CEDFCC2E}"

class IInfoWidget : 
  virtual public QWidget
{
public:
  enum InfoField {
    AccountName,
    ContactAvatar,
    ContactName,
    ContactShow,
    ContactStatus,
    ContactEmail,
    ContactClient
  };
public:
  virtual const Jid &streamJid() const =0;
  virtual void setStreamJid(const Jid &AStreamJid) =0;
  virtual const Jid &contactJid() const =0;
  virtual void setContactJid(const Jid &AContactJid) =0;
  virtual void autoSetFields() =0;
  virtual void autoSetField(InfoField AField) =0;
  virtual QVariant field(InfoField AField) const =0;
  virtual void setField(InfoField AField, const QVariant &AValue) =0;
signals:
  virtual void streamJidChanged(const Jid &ABefour) =0;
  virtual void contactJidChanged(const Jid &ABefour) =0;
  virtual void fieldChanged(IInfoWidget::InfoField AField, const QVariant &AValue) =0;
};

class IViewWidget : 
  virtual public QWidget
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
  virtual QTextEdit *textEdit() const =0;
  virtual QTextDocument *document() const =0;
  virtual ToolBarChanger *toolBarChanger() const =0;
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
  virtual public QWidget
{
public:
  virtual const Jid &streamJid() const =0;
  virtual void setStreamJid(const Jid &AStreamJid) =0;
  virtual const Jid &contactJid() const =0;
  virtual void setContactJid(const Jid &AContactJid) =0;
  virtual ToolBarChanger *toolBarChanger() const =0;
  virtual void addToolBar(QToolBar *AToolBar) =0;
  virtual void removeToolBar(QToolBar *AToolBar) =0;
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
  virtual public QWidget
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
signals:
  virtual void streamJidChanged(const Jid &ABefour) =0;
  virtual void receiverAdded(const Jid &AReceiver) =0;
  virtual void receiverRemoved(const Jid &AReceiver) =0;
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
  virtual public QMainWindow
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
  virtual public QMainWindow,
  public ITabWidget
{
  Q_INTERFACES(ITabWidget);
public:
  virtual const Jid &streamJid() const =0;
  virtual const Jid &contactJid() const =0;
  virtual IInfoWidget *infoWidget() const =0;
  virtual IViewWidget *viewWidget() const =0;
  virtual IEditWidget *editWidget() const =0;
  virtual void showWindow() =0;
  virtual void closeWindow() =0;
signals:
  virtual void streamJidChanged(const Jid &ABefour) =0;
  virtual void contactJidChanged(const Jid &ABefour) =0;
  virtual void windowChanged() =0;
  virtual void windowClosed() =0;
  virtual void windowDestroyed() =0;
};

class IMessageWindow : 
  virtual public QMainWindow,
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
  virtual void addTabWidget(QWidget *AWidget) =0;
  virtual void removeTabWidget(QWidget *AWidget) =0;
  virtual IInfoWidget *infoWidget() const =0;
  virtual IViewWidget *viewWidget() const =0;
  virtual IEditWidget *editWidget() const =0;
  virtual IReceiversWidget *receiversWidget() const =0;
  virtual Mode mode() const =0;
  virtual void setMode(Mode AMode) =0;
  virtual void showWindow() =0;
  virtual void closeWindow() =0;
signals:
  virtual void streamJidChanged(const Jid &ABefour) =0;
  virtual void contactJidChanged(const Jid &ABefour) =0;
  virtual void windowChanged() =0;
  virtual void windowClosed() =0;
  virtual void windowDestroyed() =0;
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
  virtual int newMessageId() =0;
  virtual void textToMessage(Message &AMessage, const QTextDocument *ADocument, const QString &ALang = "") const =0;
  virtual void messageToText(QTextDocument *ADocument, const Message &AMessage, const QString &ALang = "") const =0;
  virtual bool sendMessage(const Message &AMessage, const Jid &AStreamJid) =0;
  virtual int receiveMessage(const Message &AMessage) =0;
  virtual void showMessage(int AMessageId) =0;
  virtual void removeMessage(int AMessageId) =0;
  virtual Message messageById(int AMessageId) const =0;
  virtual QList<int> messages(const Jid &AStreamJid, const Jid &AFromJid = Jid(), Message::MessageType AMesType = Message::AnyType) =0;
  virtual bool checkOption(IMessenger::Option AOption) const =0;
  virtual void setOption(IMessenger::Option AOption, bool AValue) =0;
  //MessageWindows
  virtual IInfoWidget *newInfoWidget(const Jid &AStreamJid, const Jid &AContactJid) =0;
  virtual IViewWidget *newViewWidget(const Jid &AStreamJid, const Jid &AContactJid) =0;
  virtual IEditWidget *newEditWidget(const Jid &AStreamJid, const Jid &AContactJid) =0;
  virtual IReceiversWidget *newReceiversWidget(const Jid &AStreamJid) =0;
  virtual IChatWindow *openChatWindow(const Jid &AStreamJid, const Jid &AContactJid) =0;
  virtual IChatWindow *findChatWindow(const Jid &AStreamJid, const Jid &AContactJid) =0;
  virtual QList<int> tabWindows() const =0;
  virtual ITabWindow *openTabWindow(int AWindowId = 0) =0;
  virtual ITabWindow *findTabWindow(int AWindowId = 0) =0;
signals:
  virtual void messageReceived(const Message &AMessage) =0;
  virtual void messageNotified(int AMessageId) =0;
  virtual void messageUnNotified(int AMessageId) =0;
  virtual void messageRemoved(const Message &AMessage) =0;
  virtual void messageSent(const Message &AMessage) =0;
  virtual void optionChanged(IMessenger::Option AOption, bool AValue) =0;
  virtual void optionsAccepted() =0;
  virtual void optionsRejected() =0;
  //MessageWindows
  virtual void tabWindowCreated(ITabWindow *AWindow) =0;
  virtual void tabWindowDestroyed(ITabWindow *AWindow) =0;
  virtual void chatWindowCreated(IChatWindow *AWindow) =0;
  virtual void chatWindowDestroyed(IChatWindow *AWindow) =0;
};

Q_DECLARE_INTERFACE(IInfoWidget,"Vacuum.Plugin.IInfoWidget/1.0")
Q_DECLARE_INTERFACE(IViewWidget,"Vacuum.Plugin.IViewWidget/1.0")
Q_DECLARE_INTERFACE(IEditWidget,"Vacuum.Plugin.IEditWidget/1.0")
Q_DECLARE_INTERFACE(IReceiversWidget,"Vacuum.Plugin.IReceiversWidget/1.0")
Q_DECLARE_INTERFACE(ITabWidget,"Vacuum.Plugin.ITabWidget/1.0")
Q_DECLARE_INTERFACE(ITabWindow,"Vacuum.Plugin.ITabWindow/1.0")
Q_DECLARE_INTERFACE(IChatWindow,"Vacuum.Plugin.IChatWindow/1.0")
Q_DECLARE_INTERFACE(IMessageWindow,"Vacuum.Plugin.IMessageWindow/1.0")
Q_DECLARE_INTERFACE(IMessenger,"Vacuum.Plugin.IMessenger/1.0")

#endif //IMESSENGER_H