#ifndef IMESSAGESTYLEMANAGER_H
#define IMESSAGESTYLEMANAGER_H

#include <QUrl>
#include <QString>
#include <QVariant>
#include <QDateTime>
#include <QStringList>
#include <QTextCharFormat>
#include <QTextDocumentFragment>
#include <interfaces/ioptionsmanager.h>
#include <utils/jid.h>
#include <utils/options.h>

#define MESSAGESTYLES_UUID  "{e3ab1bc7-35a6-431a-9b91-c778451b1eb1}"

struct IMessageStyleOptions
{
	QString engineId;
	QString styleId;
	QMap<QString, QVariant> extended;
};

struct IMessageStyleContentOptions
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
	IMessageStyleContentOptions() { 
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
	virtual QTextCharFormat textFormatAt(QWidget *AWidget, const QPoint &APosition) const =0;
	virtual QTextDocumentFragment textFragmentAt(QWidget *AWidget, const QPoint &APosition) const =0;
	virtual bool changeOptions(QWidget *AWidget, const IMessageStyleOptions &AOptions, bool AClear = true) =0;
	virtual bool appendContent(QWidget *AWidget, const QString &AHtml, const IMessageStyleContentOptions &AOptions) =0;
protected:
	virtual void widgetAdded(QWidget *AWidget) const =0;
	virtual void widgetRemoved(QWidget *AWidget) const =0;
	virtual void optionsChanged(QWidget *AWidget, const IMessageStyleOptions &AOptions, bool ACleared) const =0;
	virtual void contentAppended(QWidget *AWidget, const QString &AHtml, const IMessageStyleContentOptions &AOptions) const =0;
	virtual void urlClicked(QWidget *AWidget, const QUrl &AUrl) const =0;
};

class IMessageStyleEngine
{
public:
	virtual QObject *instance() = 0;
	virtual QString engineId() const =0;
	virtual QString engineName() const =0;
	virtual QList<QString> styles() const =0;
	virtual QList<int> supportedMessageTypes() const =0;
	virtual IMessageStyle *styleForOptions(const IMessageStyleOptions &AOptions) =0;
	virtual IMessageStyleOptions styleOptions(const OptionsNode &AEngineNode, const QString &AStyleId=QString::null) const =0;
	virtual IOptionsDialogWidget *styleSettingsWidget(const OptionsNode &AStyleNode, QWidget *AParent) =0;
	virtual IMessageStyleOptions styleSettinsOptions(IOptionsDialogWidget *AWidget) const =0;
protected:
	virtual void styleCreated(IMessageStyle *AStyle) const =0;
	virtual void styleDestroyed(IMessageStyle *AStyle) const =0;
	virtual void styleWidgetAdded(IMessageStyle *AStyle, QWidget *AWidget) const =0;
	virtual void styleWidgetRemoved(IMessageStyle *AStyle, QWidget *AWidget) const =0;
};

class IMessageStyleManager
{
public:
	virtual QObject *instance() = 0;
	virtual QList<QString> styleEngines() const =0;
	virtual IMessageStyleEngine *findStyleEngine(const QString &AEngineId) const =0;
	virtual void registerStyleEngine(IMessageStyleEngine *AEngine) =0;
	virtual IMessageStyle *styleForOptions(const IMessageStyleOptions &AOptions) const =0;
	virtual IMessageStyleOptions styleOptions(int AMessageType, const QString &AContext=QString::null) const =0;
	virtual QString contactAvatar(const Jid &AContactJid) const =0;
	virtual QString contactName(const Jid &AStreamJid, const Jid &AContactJid=Jid::null) const =0;
	virtual QString contactIcon(const Jid &AStreamJid, const Jid &AContactJid=Jid::null) const =0;
	virtual QString contactIcon(const Jid &AContactJid, int AShow, const QString &ASubscription, bool AAsk) const =0;
	virtual QString dateSeparator(const QDate &ADate, const QDate &ACurDate=QDate::currentDate()) const =0;
	virtual QString timeFormat(const QDateTime &ATime, const QDateTime &ACurTime=QDateTime::currentDateTime()) const =0;
protected:
	virtual void styleEngineRegistered(IMessageStyleEngine *AEngine) =0;
	virtual void styleOptionsChanged(const IMessageStyleOptions &AOptions, int AMessageType, const QString &AContext) const =0;
};

Q_DECLARE_INTERFACE(IMessageStyle,"Vacuum.Plugin.IMessageStyle/1.3")
Q_DECLARE_INTERFACE(IMessageStyleEngine,"Vacuum.Plugin.IMessageStyleEngine/1.2")
Q_DECLARE_INTERFACE(IMessageStyleManager,"Vacuum.Plugin.IMessageStyleManager/1.4")

#endif // IMESSAGESTYLEMANAGER_H
