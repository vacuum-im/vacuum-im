#ifndef IMESSAGESTYLES_H
#define IMESSAGESTYLES_H

#include <QMap>
#include <QColor>
#include <QString>
#include <QVariant>
#include <QDateTime>
#include <QStringList>
#include "../../utils/jid.h"

class QWebView;

#define MESSAGESTYLES_UUID  "{e3ab1bc7-35a6-431a-9b91-c778451b1eb1}"

//Message Style Info Values
#define MSIV_VERSION                        "MessageViewVersion"
#define MSIV_NAME                           "CFBundleName"
#define MSIV_IDENTIFIER                     "CFBundleIdentifier"
#define MSIV_INFO_STRING                    "CFBundleGetInfoString"
#define MSIV_DEFAULT_VARIANT                "DefaultVariant"
#define MSIV_NO_VARIANT_NAME                "DisplayNameForNoVariant"
#define MSIV_DEFAULT_FONT_FAMILY            "DefaultFontFamily"
#define MSIV_DEFAULT_FONT_SIZE              "DefaultFontSize"
#define MSIV_SHOW_USER_ICONS                "ShowsUserIcons"
#define MSIV_ALLOW_TEXT_COLORS              "AllowTextColors"
#define MSIV_DISABLE_COMBINE_CONSECUTIVE    "DisableCombineConsecutive"
#define MSIV_DEFAULT_BACKGROUND_COLOR       "DefaultBackgroundColor"
#define MSIV_DISABLE_CUSTOM_BACKGROUND      "DisableCustomBackground"
#define MSIV_BACKGROUND_TRANSPARENT         "DefaultBackgroundIsTransparent"
#define MSIV_IMAGE_MASK                     "ImageMask"

//Message Status Message Classes
#define MSMC_MESSAGE                        "message"
#define MSMC_EVENT                          "event"
#define MSMC_STATUS                         "status"
#define MSMC_NOTIFICATION                   "notification"
#define MSMC_HISTORY                        "history"
#define MSMC_CONSECUTIVE                    "consecutive"
#define MSMC_OUTGOING                       "outgoing"
#define MSMC_INCOMING                       "incoming"
#define MSMC_GROUPCHAT                      "groupchat"
#define MSMC_MENTION                        "mention"
#define MSMC_AUTOREPLY                      "autoreply"

//Message Style Status Keywords
#define MSSK_ONLINE                         "online"
#define MSSK_OFFLINE                        "offline"
#define MSSK_AWAY                           "away"
#define MSSK_AWAY_MESSAGE                   "away_message"
#define MSSK_RETURN_AWAY                    "return_away"
#define MSSK_IDLE                           "idle"
#define MSSK_RETURN_IDLE                    "return_idle"
#define MSSK_DATE_SEPARATOR                 "date_separator"
#define MSSK_CONTACT_JOINED                 "contact_joined"
#define MSSK_CONTACT_LEFT                   "contact_left"
#define MSSK_ERROR                          "error"
#define MSSK_TIMED_OUT                      "timed_out"
#define MSSK_ENCRYPTION                     "encryption"
#define MSSK_FILETRANSFER_BEGAN             "fileTransferBegan"
#define MSSK_FILETRANSFER_CONPLETE          "fileTransferComplete"

class IMessageStyle
{
public:
  enum HeaderType {
    HeaderNone,
    HeaderNormal,
    HeaderTopic
  };
  enum ContentType {
    ContentMessage,
    ContentArchive,
    ContentTopic,
    ContentStatus
  };
  enum BGImageLayout {
    ImageNormal,
    ImageCenter,
    ImageTitle,
    ImageTitleCenter,
    ImageScale
  };
  struct StyleOptions {
    QString variant;
    int headerType;
    QDateTime startTime;
    QString chatName;
    QString accountName;
    QString selfName;
    QString selfAvatar;
    QString contactName;
    QString contactAvatar;
    QColor bgColor;
    QString bgImageFile;
    int bgImageLayout;
  };
  struct ContentOptions {
    bool isAlignLTR;
    bool isSameSender;
    bool isDirectionIn;
    bool replaceLastContent;
    bool willAppendMoreContent;
    ContentType contentType;
    QStringList messageClasses;
    QString statusKeyword;
    QDateTime sendTime;
    QString sendTimeFormat;
    QString senderName;
    QString senderAvatar;
    QString senderColor;
    QString senderStatusIcon;
    QString textBGColor;
  };
public:
  virtual QObject *instance() = 0;
  virtual bool isValid() const =0;
  virtual int version() const =0;
  virtual QString styleId() const =0;
  virtual QList<QString> variants() const =0;
  virtual QMap<QString, QVariant> infoValues() const =0;
  virtual void setStyle(QWebView *AView, const IMessageStyle::StyleOptions &AOptions) const =0;
  virtual void setVariant(QWebView *AView, const QString  &AVariant) const =0;
  virtual void appendContent(QWebView *AView, const QString &AMessage, const IMessageStyle::ContentOptions &AOptions) const =0;
signals:
  virtual void styleSeted(QWebView *AView, const IMessageStyle::StyleOptions &AOptions) const =0;
  virtual void variantSeted(QWebView *AView, const QString  &AVariant) const =0;
  virtual void contentAppended(QWebView *AView, const QString &AMessage, const IMessageStyle::ContentOptions &AOptions) const =0;
};

class IMessageStyles 
{
public:
  struct ContentSettings {
    bool showAvatars;
    bool showStatusIcons;
  };
  struct StyleSettings {
    QString styleId;
    QString variant;
    QString bgColor;
    QString bgImageFile;
    int bgImageLayout;
    ContentSettings content;
  };
public:
  virtual QObject *instance() = 0;
  virtual IMessageStyle *styleById(const QString &AStyleId) =0;
  virtual QList<QString> styles() const =0;
  virtual QList<QString> styleVariants(const QString &AStyleId) const =0;
  virtual QMap<QString, QVariant> styleInfo(const QString &AStyleId) const =0;
  virtual IMessageStyles::StyleSettings styleSettings(int AMessageType) const =0;
  virtual void setStyleSettings(int AMessageType, const IMessageStyles::StyleSettings &ASettings) =0;
  virtual QString userAvatar(const Jid &AContactJid) const =0;
  virtual QString userName(const Jid &AStreamJid, const Jid &AContactJid = Jid()) const =0;
  virtual QString userIcon(const Jid &AStreamJid, const Jid &AContactJid = Jid()) const =0;
  virtual QString userIcon(const Jid &AContactJid, int AShow, const QString &ASubscription, bool AAsk) const =0;
  virtual QString messageTimeFormat(const QDateTime &AMessageTime, const QDateTime &ACurTime = QDateTime::currentDateTime()) const =0;
signals:
  virtual void styleCreated(IMessageStyle *AStyle) const =0;
  virtual void styleSettingsChanged(int AMessageType, const IMessageStyles::StyleSettings &ASettings) const =0;
};

Q_DECLARE_INTERFACE(IMessageStyle,"Vacuum.Plugin.IMessageStyle/1.0")
Q_DECLARE_INTERFACE(IMessageStyles,"Vacuum.Plugin.IMessageStyles/1.0")

#endif
