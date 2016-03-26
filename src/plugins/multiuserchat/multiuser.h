#ifndef MULTIUSER_H
#define MULTIUSER_H

#include <interfaces/imultiuserchat.h>
#include <interfaces/ipresencemanager.h>

class MultiUser :
	public QObject,
	public IMultiUser
{
	Q_OBJECT;
	Q_INTERFACES(IMultiUser);
	friend class MultiUserChat;
public:
	MultiUser(const Jid &AStreamJid, const Jid &AUserJid, const Jid &ARealJid, QObject *AParent);
	~MultiUser();
	virtual QObject *instance() { return this; }
	virtual Jid streamJid() const;
	virtual Jid roomJid() const;
	virtual Jid userJid() const;
	virtual Jid realJid() const;
	virtual QString nick() const;
	virtual QString role() const;
	virtual QString affiliation() const;
	virtual IPresenceItem presence() const;
signals:
	void changed(int AData, const QVariant &ABefore);
protected:
	void setNick(const QString &ANick);
	void setRole(const QString &ARole);
	void setAffiliation(const QString &AAffiliation);
	void setPresence(const IPresenceItem &APresence);
private:
	Jid FStreamJid;
	Jid FRealJid;
	Jid FUserJid;
	QString FNick;
	QString FRole;
	QString FAffiliation;
	IPresenceItem FPresence;
};

#endif // MULTIUSER_H
