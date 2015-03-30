#ifndef MESSAGESTYLEMANAGER_H
#define MESSAGESTYLEMANAGER_H

#include <interfaces/ipluginmanager.h>
#include <interfaces/imessagestylemanager.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/iavatars.h>
#include <interfaces/ivcard.h>
#include <interfaces/iroster.h>
#include <interfaces/ipresence.h>
#include <interfaces/istatusicons.h>

class MessageStyleManager :
	public QObject,
	public IPlugin,
	public IMessageStyleManager,
	public IOptionsDialogHolder
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IMessageStyleManager IOptionsDialogHolder);
public:
	MessageStyleManager();
	~MessageStyleManager();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return MESSAGESTYLES_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects() {return true; }
	virtual bool initSettings();
	virtual bool startPlugin() { return true; }
	//IOptionsHolder
	virtual QMultiMap<int, IOptionsDialogWidget *> optionsDialogWidgets(const QString &ANodeId, QWidget *AParent);
	//IMessageStyleManager
	virtual QList<QString> styleEngines() const;
	virtual IMessageStyleEngine *findStyleEngine(const QString &AEngineId) const;
	virtual void registerStyleEngine(IMessageStyleEngine *AEngine);
	virtual IMessageStyle *styleForOptions(const IMessageStyleOptions &AOptions) const;
	virtual IMessageStyleOptions styleOptions(int AMessageType, const QString &AContext = QString::null) const;
	//Other functions
	virtual QString contactAvatar(const Jid &AContactJid) const;
	virtual QString contactName(const Jid &AStreamJid, const Jid &AContactJid = Jid::null) const;
	virtual QString contactIcon(const Jid &AStreamJid, const Jid &AContactJid = Jid::null) const;
	virtual QString contactIcon(const Jid &AContactJid, int AShow, const QString &ASubscription, bool AAsk) const;
	virtual QString dateSeparator(const QDate &ADate, const QDate &ACurDate = QDate::currentDate()) const;
	virtual QString timeFormat(const QDateTime &ATime, const QDateTime &ACurTime = QDateTime::currentDateTime()) const;
signals:
	void styleEngineRegistered(IMessageStyleEngine *AEngine);
	void styleOptionsChanged(const IMessageStyleOptions &AOptions, int AMessageType, const QString &AContext) const;
protected:
	void appendPendingChanges(int AMessageType, const QString &AContext);
protected slots:
	void onVCardChanged(const Jid &AContactJid);
	void onOptionsChanged(const OptionsNode &ANode);
	void onApplyPendingChanges();
private:
	IAvatars *FAvatars;
	IStatusIcons *FStatusIcons;
	IVCardPlugin *FVCardPlugin;
	IRosterPlugin *FRosterPlugin;
	IOptionsManager *FOptionsManager;
private:
	mutable QMap<Jid, QString> FStreamNames;
	QList< QPair<int,QString> > FPendingChages;
	QMap<QString, IMessageStyleEngine *> FStyleEngines;
};

#endif // MESSAGESTYLEMANAGER_H
