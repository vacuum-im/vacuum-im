#ifndef SIMPLEMESSAGESTYLE_H
#define SIMPLEMESSAGESTYLE_H

#include <QList>
#include "../../definations/resources.h"
#include "../../interfaces/imessagestyles.h"
#include "../../utils/filestorage.h"
#include "styleviewer.h"

//Message Style Info Values
#define MSIV_NAME                           "Name"
#define MSIV_DEFAULT_VARIANT                "DefaultVariant"
#define MSIV_DEFAULT_FONT_FAMILY            "DefaultFontFamily"
#define MSIV_DEFAULT_FONT_SIZE              "DefaultFontSize"
#define MSIV_DISABLE_COMBINE_CONSECUTIVE    "DisableCombineConsecutive"
#define MSIV_DEFAULT_BACKGROUND_COLOR       "DefaultBackgroundColor"
#define MSIV_DISABLE_CUSTOM_BACKGROUND      "DisableCustomBackground"

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

//Message Style Options
#define MSO_STYLE_ID                        "styleId"
#define MSO_VARIANT                         "variant"
#define MSO_FONT_FAMILY                     "fontFamily"
#define MSO_FONT_SIZE                       "fontSize"
#define MSO_BG_COLOR                        "bgColor"
#define MSO_BG_IMAGE_FILE                   "bgImageFile"

class SimpleMessageStyle : 
  public QObject,
  public IMessageStyle
{
  Q_OBJECT;
  Q_INTERFACES(IMessageStyle);
public:
  struct WidgetStatus {
    int lastKind;
    QString lastId;
    QDateTime lastTime;
    bool scrollStarted;
  };
public:
  SimpleMessageStyle(const QString &AStylePath, QObject *AParent);
  ~SimpleMessageStyle();
  //IMessageStyle
  virtual QObject *instance() { return this; }
  virtual bool isValid() const;
  virtual QString styleId() const;
  virtual QList<QWidget *> styleWidgets() const;
  virtual QWidget *createWidget(const IMessageStyleOptions &AOptions, QWidget *AParent);
  virtual QString senderColor(const QString &ASenderId) const;
  virtual void changeOptions(QWidget *AWidget, const IMessageStyleOptions &AOptions, bool AClean = true);
  virtual void appendContent(QWidget *AWidget, const QString &AHtml, const IMessageContentOptions &AOptions);
  //ISimpleMessageStyle
  virtual QMap<QString, QVariant> infoValues() const;
  virtual QList<QString> variants() const;
signals:
  virtual void widgetAdded(QWidget *AWidget) const;
  virtual void widgetRemoved(QWidget *AWidget) const;
  virtual void optionsChanged(QWidget *AWidget, const IMessageStyleOptions &AOptions, bool AClean) const;
  virtual void contentAppended(QWidget *AWidget, const QString &AHtml, const IMessageContentOptions &AOptions) const;
  virtual void urlClicked(QWidget *AWidget, const QUrl &AUrl) const;
public:
  static QList<QString> styleVariants(const QString &AStylePath);
  static QMap<QString, QVariant> styleInfo(const QString &AStylePath);
protected:
  bool isSameSender(QWidget *AWidget, const IMessageContentOptions &AOptions) const;
  void setVariant(QWidget *AWidget, const QString  &AVariant);
  QString makeStyleTemplate() const;
  void fillStyleKeywords(QString &AHtml, const IMessageStyleOptions &AOptions) const;
  QString makeContentTemplate(const IMessageContentOptions &AOptions, bool ASameSender) const;
  void fillContentKeywords(QString &AHtml, const IMessageContentOptions &AOptions, bool ASameSender) const;
  QString loadFileData(const QString &AFileName, const QString &DefValue) const;
  void loadTemplates();
  void initStyleSettings();
protected:
  virtual bool eventFilter(QObject *AWatched, QEvent *AEvent);
protected slots:
  void onLinkClicked(const QUrl &AUrl);
  void onScrollAfterResize();
  void onStyleWidgetAdded(IMessageStyle *AStyle, QWidget *AWidget);
  void onStyleWidgetDestroyed(QObject *AObject);
private:
  bool FCombineConsecutive;
  bool FAllowCustomBackground;
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
  QString FStylePath;
  QList<QString> FVariants;
  QMap<QString, QVariant> FInfo;
  QMap<QWidget *, WidgetStatus> FWidgetStatus;
};

#endif // SIMPLEMESSAGESTYLE_H
