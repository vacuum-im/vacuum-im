#ifndef SIMPLEMESSAGESTYLE_H
#define SIMPLEMESSAGESTYLE_H

#include <QList>
#include <QTimer>
#include <QNetworkAccessManager>
#include <interfaces/imessagestylemanager.h>
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
#define MSMC_MECOMMAND                      "me_command"

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
#define MSSK_FILETRANSFER_COMPLETE          "fileTransferComplete"

//Message Style Options
#define MSO_VARIANT                         "variant"
#define MSO_FONT_FAMILY                     "fontFamily"
#define MSO_FONT_SIZE                       "fontSize"
#define MSO_BG_COLOR                        "bgColor"
#define MSO_BG_IMAGE_FILE                   "bgImageFile"

struct ContentItem {
	int size;
};

struct WidgetStatus {
	int lastKind;
	QString lastId;
	QDateTime lastTime;
	bool scrollStarted;
	int contentStartPosition;
	QList<ContentItem> content;
};

class SimpleMessageStyle :
	public QObject,
	public IMessageStyle
{
	Q_OBJECT;
	Q_INTERFACES(IMessageStyle);
public:
	SimpleMessageStyle(const QString &AStylePath, QNetworkAccessManager *ANetworkAccessManager, QObject *AParent);
	~SimpleMessageStyle();
	//IMessageStyle
	virtual QObject *instance() { return this; }
	virtual bool isValid() const;
	virtual QString styleId() const;
	virtual QList<QWidget *> styleWidgets() const;
	virtual QWidget *createWidget(const IMessageStyleOptions &AOptions, QWidget *AParent);
	virtual QString senderColor(const QString &ASenderId) const;
	virtual QTextDocumentFragment selection(QWidget *AWidget) const;
	virtual QTextCharFormat textFormatAt(QWidget *AWidget, const QPoint &APosition) const;
	virtual QTextDocumentFragment textFragmentAt(QWidget *AWidget, const QPoint &APosition) const;
	virtual bool changeOptions(QWidget *AWidget, const IMessageStyleOptions &AOptions, bool AClear = true);
	virtual bool appendContent(QWidget *AWidget, const QString &AHtml, const IMessageStyleContentOptions &AOptions);
	//ISimpleMessageStyle
	virtual QMap<QString, QVariant> infoValues() const;
	virtual QList<QString> variants() const;
signals:
	void widgetAdded(QWidget *AWidget) const;
	void widgetRemoved(QWidget *AWidget) const;
	void optionsChanged(QWidget *AWidget, const IMessageStyleOptions &AOptions, bool AClean) const;
	void contentAppended(QWidget *AWidget, const QString &AHtml, const IMessageStyleContentOptions &AOptions) const;
	void urlClicked(QWidget *AWidget, const QUrl &AUrl) const;
public:
	static QList<QString> styleVariants(const QString &AStylePath);
	static QMap<QString, QVariant> styleInfo(const QString &AStylePath);
protected:
	bool isSameSender(QWidget *AWidget, const IMessageStyleContentOptions &AOptions) const;
	void setVariant(QWidget *AWidget, const QString  &AVariant);
	QString makeStyleTemplate() const;
	void fillStyleKeywords(QString &AHtml, const IMessageStyleOptions &AOptions) const;
	QString makeContentTemplate(const IMessageStyleContentOptions &AOptions, bool ASameSender) const;
	void fillContentKeywords(QString &AHtml, const IMessageStyleContentOptions &AOptions, bool ASameSender) const;
	QString prepareMessage(const QString &AHtml, const IMessageStyleContentOptions &AOptions) const;
	QString loadFileData(const QString &AFileName, const QString &DefValue) const;
	void loadTemplates();
	void loadSenderColors();
	void initStyleSettings();
protected:
	bool eventFilter(QObject *AWatched, QEvent *AEvent);
protected slots:
	void onScrollAfterResize();
	void onStyleWidgetLinkClicked(const QUrl &AUrl);
	void onStyleWidgetDestroyed(QObject *AObject);
	void onStyleWidgetAdded(IMessageStyle *AStyle, QWidget *AWidget);
private:
	QTimer FScrollTimer;
	bool FCombineConsecutive;
	bool FAllowCustomBackground;
private:
	QString FTopicHTML;
	QString FStatusHTML;
	QString FMeCommandHTML;
	QString FIn_ContentHTML;
	QString FIn_NextContentHTML;
	QString FOut_ContentHTML;
	QString FOut_NextContentHTML;
private:
	QString FStylePath;
	QList<QString> FVariants;
	QList<QString> FSenderColors;
	QMap<QString, QVariant> FInfo;
	QMap<QWidget *, WidgetStatus> FWidgetStatus;
	QNetworkAccessManager *FNetworkAccessManager;
private:
	static QString FSharedPath;
};

#endif // SIMPLEMESSAGESTYLE_H
