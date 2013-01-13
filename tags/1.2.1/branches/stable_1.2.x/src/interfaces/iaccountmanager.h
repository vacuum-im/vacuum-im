#ifndef IACCOUNTMANAGER_H
#define IACCOUNTMANAGER_H

#include <QUuid>
#include <QString>
#include <QVariant>
#include <interfaces/ixmppstreams.h>
#include <utils/jid.h>
#include <utils/options.h>

#define ACCOUNTMANAGER_UUID "{56F1AA4C-37A6-4007-ACFE-557EEBD86AF8}"

class IAccount
{
public:
	virtual QObject *instance() = 0;
	virtual bool isValid() const =0;
	virtual QUuid accountId() const =0;
	virtual bool isActive() const =0;
	virtual void setActive(bool AActive) =0;
	virtual QString name() const =0;
	virtual void setName(const QString &AName) =0;
	virtual Jid streamJid() const =0;
	virtual void setStreamJid(const Jid &AStreamJid) =0;
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
	virtual IAccount *accountById(const QUuid &AAcoountId) const =0;
	virtual IAccount *accountByStream(const Jid &AStreamJid) const =0;
	virtual IAccount *appendAccount(const QUuid &AAccountId) =0;
	virtual void showAccount(const QUuid &AAccountId) =0;
	virtual void hideAccount(const QUuid &AAccountId) =0;
	virtual void removeAccount(const QUuid &AAccountId) =0;
	virtual void destroyAccount(const QUuid &AAccountId) =0;
protected:
	virtual void appended(IAccount *AAccount) =0;
	virtual void shown(IAccount *AAccount) =0;
	virtual void hidden(IAccount *AAccount) =0;
	virtual void removed(IAccount *AAccount) =0;
	virtual void changed(IAccount *AAcount, const OptionsNode &ANode) =0;
	virtual void destroyed(const QUuid &AAccountId) =0;
};

Q_DECLARE_INTERFACE(IAccount,"Vacuum.Plugin.IAccount/1.0")
Q_DECLARE_INTERFACE(IAccountManager,"Vacuum.Plugin.IAccountManager/1.0")

#endif
