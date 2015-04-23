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
	MultiUser(const Jid &ARoomJid, const QString &ANickName, QObject *AParent);
	~MultiUser();
	virtual QObject *instance() { return this; }
	virtual Jid roomJid() const;
	virtual Jid contactJid() const;
	virtual QString nickName() const;
	virtual QString role() const;
	virtual QString affiliation() const;
	virtual QVariant data(int ARole) const;
	virtual void setData(int ARole, const QVariant &AValue);
signals:
	void dataChanged(int ARole, const QVariant &ABefore, const QVariant &AAfter);
protected:
	void setNickName(const QString &ANickName);
private:
	Jid FRoomJid;
	Jid FContactJid;
	QString FNickName;
	QHash<int, QVariant> FData;
};

#endif // MULTIUSER_H
