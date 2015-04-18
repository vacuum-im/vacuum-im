#ifndef ACCOUNT_H
#define ACCOUNT_H

#include <QInputDialog>
#include <interfaces/iaccountmanager.h>
#include <interfaces/ixmppstreammanager.h>
#include <interfaces/ioptionsmanager.h>
#include <utils/passworddialog.h>

class Account :
	public QObject,
	public IAccount
{
	Q_OBJECT;
	Q_INTERFACES(IAccount);
public:
	Account(IXmppStreamManager *AXmppStreamManager, const OptionsNode &AOptionsNode, QObject *AParent);
	virtual QObject *instance() { return this; }
	virtual QUuid accountId() const;
	virtual Jid accountJid() const;
	virtual Jid streamJid() const;
	virtual bool isActive() const;
	virtual void setActive(bool AActive);
	virtual QString name() const;
	virtual void setName(const QString &AName);
	virtual QString resource() const;
	virtual void setResource(const QString &AResource);
	virtual QString password() const;
	virtual void setPassword(const QString &APassword);
	virtual OptionsNode optionsNode() const;
	virtual IXmppStream *xmppStream() const;
signals:
	void activeChanged(bool AActive);
	void optionsChanged(const OptionsNode &ANode);
protected slots:
	void onXmppStreamClosed();
	void onXmppStreamError(const XmppError &AError);
	void onXmppStreamPasswordRequested(bool &AWait);
protected slots:
	void onPasswordDialogAccepted();
	void onPasswordDialogRejected();
protected slots:
	void onOptionsChanged(const OptionsNode &ANode);
private:
	IXmppStream *FXmppStream;
	IXmppStreamManager *FXmppStreamManager;
private:
	OptionsNode FOptionsNode;
private:
	bool FInvalidPassword;
	PasswordDialog *FPasswordDialog;
};

#endif // ACCOUNT_H
