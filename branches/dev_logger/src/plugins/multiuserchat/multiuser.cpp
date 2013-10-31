#include "multiuser.h"

MultiUser::MultiUser(const Jid &ARoomJid, const QString &ANickName, QObject *AParent) : QObject(AParent)
{
	FRoomJid = ARoomJid;
	FContactJid = ARoomJid;
	FContactJid.setResource(ANickName);
	FNickName = ANickName;
	setData(MUDR_ROOM_JID,FRoomJid.bare());
	setData(MUDR_NICK_NAME,FNickName);
	setData(MUDR_CONTACT_JID,FContactJid.full());
	setData(MUDR_SHOW,IPresence::Offline);
	setData(MUDR_ROLE,MUC_ROLE_NONE);
	setData(MUDR_AFFILIATION,MUC_AFFIL_NONE);
}

MultiUser::~MultiUser()
{

}

Jid MultiUser::roomJid() const
{
	return FRoomJid;
}

Jid MultiUser::contactJid() const
{
	return FContactJid;
}

QString MultiUser::nickName() const
{
	return FNickName;
}

QString MultiUser::role() const
{
	return data(MUDR_ROLE).toString();
}

QString MultiUser::affiliation() const
{
	return data(MUDR_AFFILIATION).toString();
}

QVariant MultiUser::data(int ARole) const
{
	return FData.value(ARole);
}

void MultiUser::setData(int ARole, const QVariant &AValue)
{
	QVariant before = data(ARole);
	if (before != AValue)
	{
		if (AValue.isValid())
			FData.insert(ARole,AValue);
		else
			FData.remove(ARole);
		emit dataChanged(ARole,before,AValue);
	}
}

void MultiUser::setNickName(const QString &ANickName)
{
	FNickName = ANickName;
	FContactJid.setResource(ANickName);
	setData(MUDR_NICK_NAME,ANickName);
	setData(MUDR_CONTACT_JID,FContactJid.full());
}
