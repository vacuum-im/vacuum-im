#ifndef IROSTERCHANGER_H
#define IROSTERCHANGER_H

#include <QDialog>
#include <QVBoxLayout>
#include <utils/jid.h>
#include <utils/toolbarchanger.h>

#define ROSTERCHANGER_UUID "{018E7891-2743-4155-8A70-EAB430573500}"

class IAddContactDialog 
{
public:
	virtual QDialog *instance() =0;
	virtual Jid streamJid() const =0;
	virtual Jid contactJid() const =0;
	virtual void setContactJid(const Jid &AContactJid) =0;
	virtual QString nickName() const =0;
	virtual void setNickName(const QString &ANick) =0;
	virtual QString group() const =0;
	virtual void setGroup(const QString &AGroup) =0;
	virtual bool subscribeContact() const =0;
	virtual void setSubscribeContact(bool ASubscribe) =0;
	virtual QString subscriptionMessage() const =0;
	virtual void setSubscriptionMessage(const QString &AMessage) =0;
	virtual ToolBarChanger *toolBarChanger() const =0;
protected:
	virtual void dialogDestroyed() =0;
};

class ISubscriptionDialog
{
public:
	virtual QDialog *instance() =0;
	virtual const Jid &streamJid() const =0;
	virtual const Jid &contactJid() const =0;
	virtual QVBoxLayout *actionsLayout() const =0;
	virtual ToolBarChanger *toolBarChanger() const =0;
protected:
	virtual void dialogDestroyed() =0;
};

class IRosterChanger
{
public:
	virtual QObject *instance() =0;
	virtual bool isAutoSubscribe(const Jid &AStreamJid, const Jid &AContactJid) const =0;
	virtual bool isAutoUnsubscribe(const Jid &AStreamJid, const Jid &AContactJid) const =0;
	virtual bool isSilentSubsctiption(const Jid &AStreamJid, const Jid &AContactJid) const =0;
	virtual void insertAutoSubscribe(const Jid &AStreamJid, const Jid &AContactJid, bool ASilently, bool ASubscr, bool AUnsubscr) =0;
	virtual void removeAutoSubscribe(const Jid &AStreamJid, const Jid &AContactJid) =0;
	virtual void subscribeContact(const Jid &AStreamJid, const Jid &AContactJid, const QString &AMessage = "", bool ASilently = false) =0;
	virtual void unsubscribeContact(const Jid &AStreamJid, const Jid &AContactJid, const QString &AMessage = "", bool ASilently = false) =0;
	virtual IAddContactDialog *showAddContactDialog(const Jid &AStreamJid) =0;
protected:
	virtual void addContactDialogCreated(IAddContactDialog *ADialog) =0;
	virtual void subscriptionDialogCreated(ISubscriptionDialog *ADialog) =0;
};

Q_DECLARE_INTERFACE(IAddContactDialog,"Vacuum.Plugin.IAddContactDialog/1.0")
Q_DECLARE_INTERFACE(ISubscriptionDialog,"Vacuum.Plugin.ISubscriptionDialog/1.0")
Q_DECLARE_INTERFACE(IRosterChanger,"Vacuum.Plugin.IRosterChanger/1.0")

#endif
