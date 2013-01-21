#ifndef ICAPTCHAFORMS_H
#define ICAPTCHAFORMS_H

#include <QString>
#include <interfaces/idataforms.h>
#include <utils/xmpperror.h>

#define CAPTCHAFORMS_UUID "{f733885c-2a25-438f-bfdb-dc7d139a222f}"

class ICaptchaForms
{
public:
	virtual QObject *instance() =0;
	virtual bool submitChallenge(const QString &AChallengeId, const IDataForm &ASubmit) =0;
	virtual bool cancelChallenge(const QString &AChallengeId) =0;
protected:
	virtual void challengeReceived(const QString &AChallengeId, const IDataForm &AForm) =0;
	virtual void challengeSubmited(const QString &AChallengeId, const IDataForm &ASubmit) =0;
	virtual void challengeAccepted(const QString &AChallengeId) =0;
	virtual void challengeRejected(const QString &AChallengeId, const XmppError &AError) =0;
	virtual void challengeCanceled(const QString &AChallengeId) =0;
};

Q_DECLARE_INTERFACE(ICaptchaForms,"Vacuum.Plugin.ICaptchaForms/1.1")

#endif
