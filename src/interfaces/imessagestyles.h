#ifndef IMESSAGESTYLES_H
#define IMESSAGESTYLES_H

#include <QUrl>
#include <QString>
#include <QVariant>
#include <QDateTime>
#include <QStringList>
#include <QTextDocumentFragment>
#include <interfaces/ioptionsmanager.h>
#include <utils/jid.h>
#include <utils/options.h>

#define MESSAGESTYLES_UUID  "{e3ab1bc7-35a6-431a-9b91-c778451b1eb1}"

struct IMessageStyleOptions
{
	QString pluginId;
	QMap<QString, QVariant> extended;
};

struct IMessageContentOptions
{
	enum ContentKind {
		KindMessage,
		KindStatus,
		KindTopic,
		KindMeCommand
	};
	enum ContentType {
		TypeEmpty           =0x00,
		TypeGroupchat       =0x01,
		TypeHistory         =0x02,
		TypeEvent           =0x04,
		TypeMention         =0x08,
		TypeNotification    =0x10
	};
	enum ContentStatus {
		StatusEmpty,
		StatusOnline,
		StatusOffline,
		StatusAway,
		StatusAwayMessage,
		StatusReturnAway,
		StatusIdle,
		StatusReturnIdle,
		StatusDateSeparator,
		StatusJoined,
		StatusLeft,
		StatusError,
		StatusTimeout,
		StatusEncryption,
		StatusFileTransferBegan,
		StatusFileTransferComplete
	};
	enum ContentDirection {
		DirectionIn,
		DirectionOut
	};
	IMessageContentOptions() { 
		kind = KindMessage;
		type = TypeEmpty;
		status = StatusEmpty;
		direction = DirectionIn;
		noScroll = false;
	}
	int kind;
	int type;
	int status;
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

class IMessageStyle
{
public:
	virtual QObject *instance() =0;
	virtual bool isValid() const =0;
	virtual QString styleId() const =0;
	virtual QList<QWidget *> styleWidgets() const =0;
	virtual QWidget *createWidget(const IMessageStyleOptions &AOptions, QWidget *AParent) =0;
	virtual QString senderColor(const QString &ASenderId) const =0;
	virtual QTextDocumentFragment selection(QWidget *AWidget) const =0;
	virtual QTextDocumentFragment textUnderPosition(const QPoint &APosition, QWidget *AWidget) const =0;
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
	virtual QString pluginId() const =0;
	virtual QString pluginName() const =0;
	virtual QList<QString> styles() const =0;
	virtual IMessageStyle *styleForOptions(const IMessageStyleOptions &AOptions) =0;
	virtual IMessageStyleOptions styleOptions(const OptionsNode &ANode, int AMessageType) const =0;
	virtual IOptionsWidget *styleSettingsWidget(const OptionsNode &ANode, int AMessageType, QWidget *AParent) =0;
	virtual void saveStyleSettings(IOptionsWidget *AWidget, OptionsNode ANode = OptionsNode::null) =0;
	virtual void saveStyleSettings(IOptionsWidget *AWidget, IMessageStyleOptions &AOptions) = 0;
protected:
	virtual void styleCreated(IMessageStyle *AStyle) const =0;
	virtual void styleDestroyed(IMessageStyle *AStyle) const =0;
	virtual void styleWidgetAdded(IMessageStyle *AStyle, QWidget *AWidget) const =0;
	virtual void styleWidgetRemoved(IMessageStyle *AStyle, QWidget *AWidget) const =0;
};

class IMessageStyles
{
public:
	virtual QObject *instance() = 0;
	virtual QList<QString> pluginList() const =0;
	virtual IMessageStylePlugin *pluginById(const QString &APluginId) const =0;
	virtual IMessageStyle *styleForOptions(const IMessageStyleOptions &AOptions) const =0;
	virtual IMessageStyleOptions styleOptions(const OptionsNode &ANode, int AMessageType) const =0;
	virtual IMessageStyleOptions styleOptions(int AMessageType, const QString &AContext = QString::null) const =0;
	virtual QString contactAvatar(const Jid &AContactJid) const =0;
	virtual QString contactName(const Jid &AStreamJid, const Jid &AContactJid = Jid::null) const =0;
	virtual QString contactIcon(const Jid &AStreamJid, const Jid &AContactJid = Jid::null) const =0;
	virtual QString contactIcon(const Jid &AContactJid, int AShow, const QString &ASubscription, bool AAsk) const =0;
	virtual QString dateSeparator(const QDate &ADate, const QDate &ACurDate = QDate::currentDate()) const =0;
	virtual QString timeFormat(const QDateTime &ATime, const QDateTime &ACurTime = QDateTime::currentDateTime()) const =0;
protected:
	virtual void styleOptionsChanged(const IMessageStyleOptions &AOptions, int AMessageType, const QString &AContext) const =0;
};

Q_DECLARE_INTERFACE(IMessageStyle,"Vacuum.Plugin.IMessageStyle/1.1")
Q_DECLARE_INTERFACE(IMessageStylePlugin,"Vacuum.Plugin.IMessageStylePlugin/1.1")
Q_DECLARE_INTERFACE(IMessageStyles,"Vacuum.Plugin.IMessageStyles/1.2")

#endif
