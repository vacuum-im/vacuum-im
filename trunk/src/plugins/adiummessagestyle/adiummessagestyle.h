#ifndef ADIUMMESSAGESTYLE_H
#define ADIUMMESSAGESTYLE_H

#include <QList>
#include <QWebView>
#include "../../definations/resources.h"
#include "../../interfaces/imessagestyles.h"
#include "../../utils/filestorage.h"

struct IAdiumMessageStyleOptions 
{
  enum HeaderType {
    HeaderNone,
    HeaderNormal,
    HeaderTopic
  };
  enum BGImageLayout {
    ImageNormal,
    ImageCenter,
    ImageTitle,
    ImageTitleCenter,
    ImageScale
  };
  IAdiumMessageStyleOptions() { headerType=HeaderNone; bgImageLayout=ImageScale; }
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

struct IMessageStyleOptions 
{
  IMessageStyleOptions() { options = NULL; }
  QString variant;
  void *options;
};

struct IMessageContentOptions 
{
  enum ContentType {
    ContentMessage,
    ContentArchive,
    ContentTopic,
    ContentStatus
  };
  bool isSameSender;
  bool isDirectionIn;
  ContentType contentType;
  QStringList messageClasses;
  QString statusKeyword;
  QDateTime sendTime;
  QString sendTimeFormat;
  QString senderName;
  QString senderAvatar;
  QString senderColor;
  QString senderStatusIcon;
};

class AdiumMessageStyle : 
  public QObject
{
  Q_OBJECT;
public:
  AdiumMessageStyle(const QString &AStylePath, QObject *AParent);
  ~AdiumMessageStyle();
  //IMessageStyle
  virtual QObject *instance() { return this; }
  virtual bool isValid() const;
  virtual QString styleId() const;
  virtual int version() const;
  virtual QList<QString> variants() const;
  virtual QWidget *styleWidget(const IMessageStyleOptions &AOptions, QWidget *AParent);
  virtual void setVariant(QWebView *AView, const QString  &AVariant) const;
  virtual void appendContent(QWebView *AView, const QString &AMessage, const IMessageStyle::ContentOptions &AOptions) const;
public:
  //AdiumMessageStyle
  QMap<QString, QVariant> infoValues() const;
signals:
  virtual void styleSeted(QWebView *AView, const IMessageStyleOptions &AOptions) const;
  virtual void variantSeted(QWebView *AView, const QString  &AVariant) const;
  virtual void contentAppended(QWebView *AView, const QString &AMessage, const IMessageStyle::ContentOptions &AOptions) const;
private:
  QString makeStyleTemplate(const IMessageStyleOptions &AOptions) const;
  QString makeContentTemplate(const IMessageStyle::ContentOptions &AOptions) const;
  void fillStyleKeywords(QString &AHtml, const IMessageStyleOptions &AOptions) const;
  void fillContentKeywords(QString &AHtml, const IMessageStyle::ContentOptions &AOptions) const;
  void escapeStringForScript(QString &AText) const;
  QString scriptForAppendContent(const IMessageStyle::ContentOptions &AOptions) const;
  QString loadFileData(const QString &AFileName, const QString &DefValue) const;
  void loadTemplates();
  void loadSenderColors();
  void initStyleSettings();
public:
  static QList<QString> styleVariants(const QString &AStylePath);
  static QMap<QString, QVariant> styleInfo(const QString &AStylePath);
private:
  QString FTopicHTML;
  QString FStatusHTML;
  QString FIn_ContentHTML;
  QString FIn_NextContentHTML;
  QString FIn_ContextHTML;
  QString FIn_NextContextHTML;
  QString FOut_ContentHTML;
  QString FOut_NextContentHTML;
  QString FOut_ContextHTML;
  QString FOut_NextContextHTML;
private:
  bool FCombineConsecutive;
  bool FAllowCustomBackground;
private:
  QString FResourcePath;
  QList<QString> FVariants;
  QList<QString> FSenderColors;
  QMap<QString, QVariant> FInfo;
};

#endif // ADIUMMESSAGESTYLE_H
