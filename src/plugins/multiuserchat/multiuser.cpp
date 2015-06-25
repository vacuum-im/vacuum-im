#include "multiuser.h"

#include <definitions/multiuserdataroles.h>

MultiUser::MultiUser(const Jid &AStreamJid, const Jid &AUserJid, const Jid &ARealJid, QObject *AParent) : QObject(AParent)
{
	FStreamJid = AStreamJid;
	FUserJid = AUserJid;
	FRealJid = ARealJid;
	FRole = MUC_ROLE_NONE;
	FAffiliation = MUC_AFFIL_NONE;
}

MultiUser::~MultiUser()
{

}

Jid MultiUser::streamJid() const
{
	return FStreamJid;
}

Jid MultiUser::roomJid() const
{
	return FUserJid.bare();
}

Jid MultiUser::userJid() const
{
	return FUserJid;
}

Jid MultiUser::realJid() const
{
	return FRealJid;
}

QString MultiUser::nick() const
{
	return FUserJid.resource();
}

QString MultiUser::role() const
{
	return FRole;
}

QString MultiUser::affiliation() const
{
	return FAffiliation;
}

IPresenceItem MultiUser::presence() const
{
	return FPresence;
}

void MultiUser::setNick(const QString &ANick)
{
	if (FUserJid.resource() != ANick)
	{
		QVariant before = FUserJid.resource();
		FUserJid.setResource(ANick);
		emit changed(MUDR_NICK,before);
	}
}

void MultiUser::setRole(const QString &ARole)
{
	if (FRole != ARole)
	{
		QVariant before = FRole;
		FRole = ARole;
		emit changed(MUDR_ROLE,before);
	}
}

void MultiUser::setAffiliation(const QString &AAffiliation)
{
	if (FAffiliation != AAffiliation)
	{
		QVariant before = FAffiliation;
		FAffiliation = AAffiliation;
		emit changed(MUDR_AFFILIATION,before);
	}
}

void MultiUser::setPresence(const IPresenceItem &APresence)
{
	if (FPresence != APresence)
	{
		FPresence = APresence;
		emit changed(MUDR_PRESENCE,QVariant());
	}
}
