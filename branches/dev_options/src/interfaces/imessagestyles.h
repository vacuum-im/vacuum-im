#ifndef IMESSAGESTYLES_H
#define IMESSAGESTYLES_H

#include <QUrl>
#include <QString>
#include <QVariant>
#include <QDateTime>
#include <QStringList>
#include <utils/jid.h>

#define MESSAGESTYLES_UUID  "{e3ab1bc7-35a6-431a-9b91-c778451b1eb1}"

struct IMessageStyleOptions 
{
  QString pluginId;
  QMap<QString, QVariant> extended;
};

struct IMessageContentOptions 
{
  enum ContentKind {
    Message,
    Status,
    Topic
  };
  enum ContentType {
    Groupchat       =0x01,
    History         =0x02,
    Event           =0x04,
    Mention         =0x08,
    Notification    =0x10
  };
  enum ContentDirection {
    DirectionIn,
    DirectionOut
  };
  IMessageContentOptions() { kind=Message; type=0; direction=DirectionIn; noScroll=false; }
  int kind;
  int type;
  int direction;
  bool noScroll;
  QDateTime time;
  QString timeFormat;
  QString senderId;
  QString senderName;
  QString senderAvatar;
  QString senderColor;
  QString senderIcon;
  QString textBGColor;
};

class IMessageStyleSettings
{
public:
  virtual QWidget *instance() =0;
  virtual int messageType() const =0;
  virtual QString context() const =0;
  virtual bool isModified(int AMessageType, const QString &AContext) const =0;
  virtual void setModified(bool AModified, int AMessageType, const QString &AContext) =0;
  virtual IMessageStyleOptions styleOptions(int AMessageType, const QString &AContext) const =0;
  virtual void loadSettings(int AMessageType, const QString &AContext) =0;
protected:
  virtual void settingsChanged() =0;
};

class IMessageStyle
{
public:
  virtual QObject *instance() =0;
  virtual bool isValid() const =0;
  virtual QString styleId() const =0;
  virtual QList<QWidget *> styleWidgets() const =0;
  virtual QWidget *createWidget(const IMessageStyleOptions &AOptions, QWidget *AParent) =0;
  virtual QString senderColor(const QString &ASenderId) const =0;
  virtual QString selectedText(QWidget *AWidget) const =0;
  virtual bool changeOptions(QWidget *AWidget, const IMessageStyleOptions &AOptions, bool AClean = true) =0;
  virtual bool appendContent(QWidget *AWidget, const QString &AHtml, const IMessageContentOptions &AOptions) =0;
protected:
  virtual void widgetAdded(QWidget *AWidget) const =0;
  virtual void widgetRemoved(QWidget *AWidget) const =0;
  virtual void optionsChanged(QWidget *AWidget, const IMessageStyleOptions &AOptions, bool AClean) const =0;
  virtual void contentAppended(QWidget *AWidget, const QString &AHtml, const IMessageContentOptions &AOptions) const =0;
  virtual void urlClicked(QWidget *AWidget, const QUrl &AUrl) const =0;
};

class IMessageStylePlugin 
{
public:
  virtual QObject *instance() = 0;
  virtual QString stylePluginId() const =0;
  virtual QList<QString> styles() const =0;
  virtual IMessageStyle *styleForOptions(const IMessageStyleOptions &AOptions) =0;
  virtual IMessageStyleSettings *styleSettings(int AMessageType, const QString &AContext, QWidget *AParent = NULL) =0;
  virtual IMessageStyleOptions styleOptions(int AMessageType, const QString &AContext = QString::null) const =0;
  virtual void setStyleOptions(const IMessageStyleOptions &AOptions, int AMessageType, const QString &AContext = QString::null) =0;
protected:
  virtual void styleCreated(IMessageStyle *AStyle) const =0;
  virtual void styleDestroyed(IMessageStyle *AStyle) const =0;
  virtual void styleWidgetAdded(IMessageStyle *AStyle, QWidget *AWidget) const =0;
  virtual void styleWidgetRemoved(IMessageStyle *AStyle, QWidget *AWidget) const =0;
  virtual void styleOptionsChanged(const IMessageStyleOptions &AOptions, int AMessageType, const QString &AContext) const =0;
};

class IMessageStyles 
{
public:
  virtual QObject *instance() = 0;
  virtual QList<QString> stylePlugins() const =0;
  virtual IMessageStylePlugin *stylePluginById(const QString &APluginId) const =0;
  virtual IMessageStyle *styleForOptions(const IMessageStyleOptions &AOptions) const =0;
  virtual IMessageStyleOptions styleOptions(int AMessageType, const QString &AContext = QString::null) const =0;
  virtual void setStyleOptions(const IMessageStyleOptions &AOptions, int AMessageType, const QString &AContext = QString::null) =0;
  virtual QString userAvatar(const Jid &AContactJid) const =0;
  virtual QString userName(const Jid &AStreamJid, const Jid &AContactJid = Jid()) const =0;
  virtual QString userIcon(const Jid &AStreamJid, const Jid &AContactJid = Jid()) const =0;
  virtual QString userIcon(const Jid &AContactJid, int AShow, const QString &ASubscription, bool AAsk) const =0;
  virtual QString timeFormat(const QDateTime &AMessageTime, const QDateTime &ACurTime = QDateTime::currentDateTime()) const =0;
protected:
  virtual void styleOptionsChanged(const IMessageStyleOptions &AOptions, int AMessageType, const QString &AContext) const =0;
};

Q_DECLARE_INTERFACE(IMessageStyleSettings,"Vacuum.Plugin.IMessageStyleSettings/1.0")
Q_DECLARE_INTERFACE(IMessageStyle,"Vacuum.Plugin.IMessageStyle/1.0")
Q_DECLARE_INTERFACE(IMessageStylePlugin,"Vacuum.Plugin.IMessageStylePlugin/1.0")
Q_DECLARE_INTERFACE(IMessageStyles,"Vacuum.Plugin.IMessageStyles/1.0")

#endif
