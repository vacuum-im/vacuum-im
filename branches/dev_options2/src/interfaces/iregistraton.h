#ifndef IREGISTRATION_H
#define IREGISTRATION_H

#include <QUrl>
#include <interfaces/idataforms.h>
#include <interfaces/ixmppstreams.h>
#include <utils/xmpperror.h>
#include <utils/jid.h>

#define REGISTRATION_UUID "{441F0DD4-C2DF-4417-B2F7-1D180C125EE3}"

struct IRegisterFields 
{
	enum Fields {
		Username  = 0x01,
		Password  = 0x02,
		Email     = 0x04,
		Redirect  = 0x08,
		Form      = 0x10
	};
	int fieldMask;
	bool registered;
	Jid serviceJid;
	QString instructions;
	QString username;
	QString password;
	QString email;
	QString key;
	QUrl redirect;
	IDataForm form;
};

struct IRegisterSubmit 
{
	int fieldMask;
	Jid serviceJid;
	QString username;
	QString password;
	QString email;
	QString key;
	IDataForm form;
};

class IRegistration 
{
public:
	enum Operation {
		Register,
		Unregister,
		ChangePassword
	};
public:
	virtual QObject *instance() =0;
	// Stream Registration
	virtual QString startStreamRegistration(IXmppStream *AXmppStream) =0;
	virtual QString submitStreamRegistration(IXmppStream *AXmppStream, const IRegisterSubmit &ASubmit) =0;
	// Service Registration
	virtual QString sendRegisterRequest(const Jid &AStreamJid, const Jid &AServiceJid) =0;
	virtual QString sendUnregisterRequest(const Jid &AStreamJid, const Jid &AServiceJid) =0;
	virtual QString sendChangePasswordRequest(const Jid &AStreamJid, const Jid &AServiceJid, const QString &AUserName, const QString &APassword) =0;
	virtual QString sendRequestSubmit(const Jid &AStreamJid, const IRegisterSubmit &ASubmit) =0;
	virtual QDialog *showRegisterDialog(const Jid &AStreamJid, const Jid &AServiceJid, int AOperation, QWidget *AParent = NULL) =0;
protected:
	virtual void registerFields(const QString &AId, const IRegisterFields &AFields) =0;
	virtual void registerError(const QString &AId, const XmppError &AError) =0;
	virtual void registerSuccess(const QString &AId) =0;
};

Q_DECLARE_INTERFACE(IRegistration,"Vacuum.Plugin.IRegistration/1.2")

#endif // IREGISTRATION_H
