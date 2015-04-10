#ifndef IACCOUNTMANAGER_H
#define IACCOUNTMANAGER_H

#include <QUuid>
#include <QString>
#include <QVariant>
#include <interfaces/ixmppstreammanager.h>
#include <utils/jid.h>
#include <utils/options.h>

#define ACCOUNTMANAGER_UUID "{56F1AA4C-37A6-4007-ACFE-557EEBD86AF8}"

class IAccount
{
public:
	virtual QObject *instance() = 0;
	virtual QUuid accountId() const =0;
	virtual Jid accountJid() const =0;
	virtual Jid streamJid() const =0;
	virtual bool isActive() const =0;
	virtual void setActive(bool AActive) =0;
	virtual QString name() const =0;
	virtual void setName(const QString &AName) =0;
	virtual QString resource() const =0;
	virtual void setResource(const QString &AResource) = 0;
	virtual QString password() const =0;
	virtual void setPassword(const QString &APassword) =0;
	virtual OptionsNode optionsNode() const =0;
	virtual IXmppStream *xmppStream() const =0;
protected:
	virtual void activeChanged(bool AActive) =0;
	virtual void optionsChanged(const OptionsNode &ANode) =0;
};

class IAccountManager
{
public:
	virtual QObject *instance() = 0;
	virtual QList<IAccount *> accounts() const =0;
	virtual IAccount *findAccountById(const QUuid &AAcoountId) const =0;
	virtual IAccount *findAccountByStream(const Jid &AStreamJid) const =0;
	virtual IAccount *createAccount(const Jid &AAccountJid, const QString &AName) =0;
	virtual void destroyAccount(const QUuid &AAccountId) =0;
protected:
	virtual void accountInserted(IAccount *AAccount) =0;
	virtual void accountRemoved(IAccount *AAccount) =0;
	virtual void accountDestroyed(const QUuid &AAccountId) =0;
	virtual void accountActiveChanged(IAccount *AAccount, bool AActive) =0;
	virtual void accountOptionsChanged(IAccount *AAcount, const OptionsNode &ANode) =0;
};

Q_DECLARE_INTERFACE(IAccount,"Vacuum.Plugin.IAccount/1.0")
Q_DECLARE_INTERFACE(IAccountManager,"Vacuum.Plugin.IAccountManager/1.0")

#endif // IACCOUNTMANAGER_H
