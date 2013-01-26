#ifndef CAPTCHAFORMS_H
#define CAPTCHAFORMS_H

#include <definitions/namespaces.h>
#include <definitions/stanzahandlerorders.h>
#include <definitions/notificationtypes.h>
#include <definitions/notificationdataroles.h>
#include <definitions/notificationtypeorders.h>
#include <definitions/menuicons.h>
#include <definitions/resources.h>
#include <definitions/soundfiles.h>
#include <definitions/dataformtypes.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/icaptchaforms.h>
#include <interfaces/idataforms.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/inotifications.h>
#include <interfaces/istanzaprocessor.h>
#include <utils/xmpperror.h>
#include <utils/iconstorage.h>
#include <utils/widgetmanager.h>

struct ChallengeItem
{
	Jid streamJid;
	Jid challenger;
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
	bool isSupportedChallenge(IDataForm &AForm) const;
	bool isValidChallenge(const Jid &AStreamJid, const Stanza &AStanza, IDataForm &AForm) const;
	void notifyChallenge(const ChallengeItem &AChallenge);
	QString findChallenge(IDataDialogWidget *ADialog) const;
	QString findChallenge(const Jid &AStreamJid, const Jid &AContactJid) const;
protected:
	bool eventFilter(QObject *AObject, QEvent *AEvent);
protected slots:
	void onStreamOpened(IXmppStream *AXmppStream);
	void onStreamClosed(IXmppStream *AXmppStream);
	void onChallengeDialogAccepted();
	void onChallengeDialogRejected();
	void onNotificationActivated(int ANotifyId);
	void onNotificationRemoved(int ANotifyId);
private:
	IDataForms *FDataForms;
	IXmppStreams *FXmppStreams;
	INotifications *FNotifications;
	IStanzaProcessor *FStanzaProcessor;
private:
	QMap<Jid, int> FSHIChallenge;
private:
	QMap<int, QString> FChallengeNotify;
	QMap<QString, QString> FChallengeRequest;
	QMap<QString, ChallengeItem> FChallenges;
};

#endif // CAPTCHAFORMS_H
