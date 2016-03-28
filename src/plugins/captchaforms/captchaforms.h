#ifndef CAPTCHAFORMS_H
#define CAPTCHAFORMS_H

#include <interfaces/ipluginmanager.h>
#include <interfaces/icaptchaforms.h>
#include <interfaces/idataforms.h>
#include <interfaces/ixmppstreammanager.h>
#include <interfaces/inotifications.h>
#include <interfaces/istanzaprocessor.h>

struct TriggerItem {
	QString id;
	QDateTime sent;
};

struct ChallengeItem {
	Jid streamJid;
	Jid challenger;
	QString challengeId;
	IDataDialogWidget *dialog;
};

class CaptchaForms :
	public QObject,
	public IPlugin,
	public ICaptchaForms,
	public IStanzaHandler,
	public IStanzaRequestOwner,
	public IDataLocalizer
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin ICaptchaForms IStanzaHandler IStanzaRequestOwner IDataLocalizer);
	Q_PLUGIN_METADATA(IID "org.vacuum-im.plugins.CaptchaForms");
public:
	CaptchaForms();
	~CaptchaForms();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return CAPTCHAFORMS_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings() { return true; }
	virtual bool startPlugin() { return true; }
	//IStanzaHandler
	virtual bool stanzaReadWrite(int AHandleId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept);
	//IStanzaRequestOwner
	virtual void stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza);
	//IDataLocalizer
	virtual IDataFormLocale dataFormLocale(const QString &AFormType);
	//ICaptchaForms
	virtual bool submitChallenge(const QString &AChallengeId, const IDataForm &ASubmit);
	virtual bool cancelChallenge(const QString &AChallengeId);
signals:
	void challengeReceived(const QString &AChallengeId, const IDataForm &AForm);
	void challengeSubmited(const QString &AChallengeId, const IDataForm &ASubmit);
	void challengeAccepted(const QString &AChallengeId);
	void challengeRejected(const QString &AChallengeId, const XmppError &AError);
	void challengeCanceled(const QString &AChallengeId);
protected:
	void appendTrigger(const Jid &AStreamJid, const Stanza &AStanza);
	bool hasTrigger(const Jid &AStreamJid, const IDataForm &AForm) const;
protected:
	IDataForm getChallengeForm(const Stanza &AStanza) const;
	bool isValidChallenge(const Stanza &AStanza, const IDataForm &AForm) const;
	bool isSupportedChallenge(IDataForm &AForm) const;
	void notifyChallenge(const ChallengeItem &AChallenge);
	QString findChallenge(IDataDialogWidget *ADialog) const;
	QString findChallenge(const Jid &AStreamJid, const Jid &AContactJid) const;
protected:
	bool setFocusToEditableField(IDataDialogWidget *ADialog);
protected:
	bool eventFilter(QObject *AObject, QEvent *AEvent);
protected slots:
	void onXmppStreamOpened(IXmppStream *AXmppStream);
	void onXmppStreamClosed(IXmppStream *AXmppStream);
	void onChallengeDialogAccepted();
	void onChallengeDialogRejected();
	void onNotificationActivated(int ANotifyId);
	void onNotificationRemoved(int ANotifyId);
private:
	IDataForms *FDataForms;
	INotifications *FNotifications;
	IStanzaProcessor *FStanzaProcessor;
	IXmppStreamManager *FXmppStreamManager;
private:
	QMap<Jid, int> FSHITrigger;
	QMap<Jid, int> FSHIChallenge;
private:
	QMap<int, QString> FChallengeNotify;
	QMap<QString, QString> FChallengeRequest;
	QMap<QString, ChallengeItem> FChallenges;
	QMap<Jid, QHash<Jid, QList<TriggerItem> > > FTriggers;
};

#endif // CAPTCHAFORMS_H
